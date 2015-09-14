/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Ander Conselvan de Oliveira <ander.conselvan.de.oliveira@intel.com>
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <drm/drm_unit_helper.h>
#include <drm/drm_dp_helper.h>

#include "../intel_dp.h"

unsigned int drm_debug = 0x1e;

void drm_err(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vprintf(format, args);
	va_end(args);

}

void drm_ut_debug_printk(const char *function_name, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	printf("[%s] ", function_name);
	vprintf(format, args);
	va_end(args);
}


struct sink_device {
	ssize_t (*dpcd_write)(struct sink_device *sink, unsigned int offset,
			      void *buffer, size_t size);
	bool (*get_link_status)(struct sink_device *sink,
				uint8_t link_status[DP_LINK_STATUS_SIZE]);

	struct {
		bool lane_count_and_bw_set;
		bool training_pattern_1_set;
		bool started_with_non_zero_levels;
		bool cr_done;
		bool channel_eq_done;

		uint8_t dpcd[0x3000];
	} data;
};

/* Fake sink device implementation */

static uint8_t
sink_device_lane_count(struct sink_device *sink)
{
	return sink->data.dpcd[DP_LANE_COUNT_SET];
}

static uint8_t
sink_device_get_training_pattern(struct sink_device *sink)
{
	return sink->data.dpcd[DP_TRAINING_PATTERN_SET] & DP_TRAINING_PATTERN_MASK;
}

static uint8_t
sink_device_get_voltage_swing(struct sink_device *sink, int lane)
{
	return sink->data.dpcd[DP_TRAINING_LANE0_SET + lane] &
		DP_TRAIN_VOLTAGE_SWING_MASK;
}

static uint8_t
sink_device_get_pre_emphasis_level(struct sink_device *sink, int lane)
{
	return (sink->data.dpcd[DP_TRAINING_LANE0_SET + lane] &
		 DP_TRAIN_PRE_EMPHASIS_MASK) >> DP_TRAIN_PRE_EMPHASIS_SHIFT;
}

static void
sink_device_check_lane_count_and_bw(struct sink_device *sink)
{
	if (sink->data.lane_count_and_bw_set)
		return;

	assert(sink->data.dpcd[DP_TRAINING_PATTERN_SET] == 0);

	if (sink->data.dpcd[DP_LINK_BW_SET] != 0 &&
	    sink->data.dpcd[DP_LANE_COUNT_SET] != 0)
		sink->data.lane_count_and_bw_set = true;
}

static void
sink_device_check_pattern_1_set(struct sink_device *sink)
{
	int lane;

	if (!sink->data.lane_count_and_bw_set ||
	    sink->data.training_pattern_1_set)
		return;

	assert(sink_device_get_training_pattern(sink) <= DP_TRAINING_PATTERN_1);

	if (sink_device_get_training_pattern(sink) != DP_TRAINING_PATTERN_1)
		return;

	assert(sink->data.dpcd[DP_LINK_BW_SET] == DP_LINK_BW_1_62 ||
	       sink->data.dpcd[DP_LINK_BW_SET] == DP_LINK_BW_2_7);

	assert(sink->data.dpcd[DP_LANE_COUNT_SET] == 1 ||
		   sink->data.dpcd[DP_LANE_COUNT_SET] == 2 ||
		   sink->data.dpcd[DP_LANE_COUNT_SET] == 4);

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		if (sink_device_get_voltage_swing(sink, lane) != DP_TRAIN_VOLTAGE_SWING_LEVEL_0 ||
		    sink_device_get_pre_emphasis_level(sink, lane) != DP_TRAIN_PRE_EMPH_LEVEL_0)
			sink->data.started_with_non_zero_levels = true;
	}

	sink->data.training_pattern_1_set = true;
}

static void
sink_device_check_pattern_2_set(struct sink_device *sink)
{
	if (!sink->data.cr_done)
		return;

	assert(sink_device_get_training_pattern(sink) == DP_TRAINING_PATTERN_2);
}

static void
sink_device_check_pattern_disable(struct sink_device *sink)
{
	if (!sink->data.cr_done || ! sink->data.channel_eq_done)
		return;

	assert(sink_device_get_training_pattern(sink) == DP_TRAINING_PATTERN_DISABLE);
}

static ssize_t
sink_device_dpcd_write(struct sink_device *sink, unsigned int offset,
		       void *buffer, size_t size)
{
	memcpy(sink->data.dpcd + offset, buffer, size);

	sink_device_check_lane_count_and_bw(sink);

	if (!sink->data.cr_done)
		sink_device_check_pattern_1_set(sink);
	else if (!sink->data.channel_eq_done)
		sink_device_check_pattern_2_set(sink);
	else
		sink_device_check_pattern_disable(sink);

	return size;
}

static bool
sink_device_max_voltage_reached(struct sink_device *sink, int lane)
{
	return (sink->data.dpcd[DP_TRAINING_LANE0_SET + lane] & DP_TRAIN_MAX_SWING_REACHED) ==
		DP_TRAIN_MAX_SWING_REACHED;
}

static bool
sink_device_max_pre_emphasis_reached(struct sink_device *sink, int lane)
{
	return (sink->data.dpcd[DP_TRAINING_LANE0_SET + lane] & DP_TRAIN_MAX_PRE_EMPHASIS_REACHED) ==
		DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;
}

static void
sink_device_set_adjust_voltage(struct sink_device *sink,
			       int lane, uint8_t level)
{
	int shift = DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT * (lane & 1);

	sink->data.dpcd[DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1)] &=
		~(DP_ADJUST_VOLTAGE_SWING_LANE0_MASK << shift);
	sink->data.dpcd[DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1)] |=
		level << shift;
}

static void
sink_device_set_adjust_pre_emphasis(struct sink_device *sink,
				    int lane, uint8_t level)
{
	int shift = DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT * (lane & 1);

	sink->data.dpcd[DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1)] &=
		~(DP_ADJUST_PRE_EMPHASIS_LANE0_MASK << shift);
	sink->data.dpcd[DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1)] |=
		level << (DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT + shift);
}

static bool
sink_device_request_higher_voltage_swing(struct sink_device *sink)
{
	bool max_reached = false;
	int lane;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		if (sink_device_max_voltage_reached(sink, lane)) {
			max_reached = true;
			break;
		}
	}

	if (max_reached)
		return false;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		uint8_t new_voltage =
			sink_device_get_voltage_swing(sink, lane) + 1;

		sink_device_set_adjust_voltage(sink, lane, new_voltage);
	}

	return true;
}

static bool
sink_device_request_higher_pre_emphasis(struct sink_device *sink)
{
	bool max_reached = false;
	int lane;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		if (sink_device_max_pre_emphasis_reached(sink, lane)) {
			max_reached = true;
			break;
		}
	}

	if (max_reached)
		return false;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		uint8_t new_pre_emphasis =
			sink_device_get_pre_emphasis_level(sink, lane) + 1;

		sink_device_set_adjust_pre_emphasis(sink, lane, new_pre_emphasis);
	}

	return true;
}

static void
sink_device_mark_cr_done(struct sink_device *sink)
{
	int lane;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++)
		sink->data.dpcd[DP_LANE0_1_STATUS + (lane >> 1)] |=
			DP_LANE_CR_DONE << (4 * (lane & 1));

	sink->data.cr_done = true;
}

static void
sink_device_mark_channel_eq_done(struct sink_device *sink)
{
	int lane;

	for (lane = 0; lane < sink_device_lane_count(sink); lane++) {
		uint8_t mask = (DP_LANE_CHANNEL_EQ_DONE | DP_LANE_SYMBOL_LOCKED);
		sink->data.dpcd[DP_LANE0_1_STATUS + (lane >> 1)] |=
			mask << (4 * (lane & 1));
	}

	sink->data.dpcd[DP_LANE_ALIGN_STATUS_UPDATED] |= DP_INTERLANE_ALIGN_DONE;

	sink->data.channel_eq_done = true;
}

static bool
sink_device_get_link_status(struct sink_device *sink,
			    uint8_t link_status[DP_LINK_STATUS_SIZE])
{
	if (!sink->data.cr_done) {
		if (!sink_device_request_higher_voltage_swing(sink))
			sink_device_mark_cr_done(sink);
	} else if (!sink->data.channel_eq_done) {
		if (!sink_device_request_higher_pre_emphasis(sink))
			sink_device_mark_channel_eq_done(sink);
	}

	memcpy(link_status, sink->data.dpcd + DP_LANE0_1_STATUS,
	       DP_LINK_STATUS_SIZE);

	return true;
}

static void
sink_device_reset(struct sink_device *sink, int lanes, uint8_t link_bw)
{
	memset(&sink->data, 0, sizeof sink->data);
	sink->data.dpcd[DP_MAX_LINK_RATE] = link_bw;
	sink->data.dpcd[DP_MAX_LANE_COUNT] = lanes;
}

static struct sink_device simple_sink = {
	.get_link_status = sink_device_get_link_status,
	.dpcd_write = sink_device_dpcd_write,
};

/* Glue code */

struct test_intel_dp {
	struct intel_dp dp;
	struct sink_device *sink;
	uint8_t link_bw;

	uint8_t max_voltage;
	uint8_t max_pre_emphasis;
};

static struct test_intel_dp *
to_test_intel_dp(struct intel_dp *dp)
{
	return container_of(dp, struct test_intel_dp, dp);
}

void intel_dp_set_idle_link_train(struct intel_dp *intel_dp)
{
}

bool intel_dp_source_supports_hbr2(struct intel_dp *intel_dp)
{
	return false;
}

bool
intel_dp_get_link_status(struct intel_dp *intel_dp, uint8_t link_status[DP_LINK_STATUS_SIZE])
{
	struct sink_device *sink = to_test_intel_dp(intel_dp)->sink;
	return sink->get_link_status(sink, link_status);
}

void
intel_dp_update_signal_levels(struct intel_dp *intel_dp)
{
}

void intel_dp_compute_rate(struct intel_dp *intel_dp, int port_clock,
			   uint8_t *link_bw, uint8_t *rate_select)
{
	*link_bw = to_test_intel_dp(intel_dp)->link_bw;
	*rate_select = 0;
}

void
intel_dp_program_link_training_pattern(struct intel_dp *intel_dp,
				       uint8_t dp_train_pat)
{
}

uint8_t
intel_dp_voltage_max(struct intel_dp *intel_dp)
{
	return to_test_intel_dp(intel_dp)->max_voltage <<
		DP_TRAIN_VOLTAGE_SWING_SHIFT;
}

uint8_t
intel_dp_pre_emphasis_max(struct intel_dp *intel_dp, uint8_t voltage_swing)
{
	return to_test_intel_dp(intel_dp)->max_pre_emphasis <<
		DP_TRAIN_PRE_EMPHASIS_SHIFT;
}

ssize_t drm_dp_dpcd_write(struct drm_dp_aux *aux, unsigned int offset,
			  void *buffer, size_t size)
{
	struct intel_dp *intel_dp =
		container_of(aux, struct intel_dp, aux);
	struct sink_device *sink = to_test_intel_dp(intel_dp)->sink;

	return sink->dpcd_write(sink, offset, buffer, size);
}

/* --- */

static struct test_intel_dp test_dp;

static void
do_test(struct sink_device *sink, int lanes, uint8_t link_bw,
	     uint8_t max_voltage, uint8_t max_pre_emphasis)
{
	int lane;

	memset(&test_dp, 0, sizeof test_dp);
	test_dp.dp.lane_count = lanes;
	test_dp.link_bw = link_bw;
	test_dp.sink = sink;
	test_dp.max_voltage =
		max_voltage >> DP_TRAIN_VOLTAGE_SWING_SHIFT;
	test_dp.max_pre_emphasis =
		max_pre_emphasis >> DP_TRAIN_PRE_EMPHASIS_SHIFT;

	sink_device_reset(sink, lanes, link_bw);

	intel_dp_start_link_train(&test_dp.dp);
	intel_dp_stop_link_train(&test_dp.dp);

	assert(sink->data.cr_done);
	assert(sink->data.channel_eq_done);

	for (lane = 0; lane < test_dp.dp.lane_count; lane++) {
		uint8_t cur_v = sink_device_get_voltage_swing(sink, lane);
		uint8_t cur_p = sink_device_get_pre_emphasis_level(sink, lane);

		assert(cur_v == test_dp.max_voltage);
		assert(cur_p == test_dp.max_pre_emphasis);
	}
}

int test_lanes[] = {
	1, 2, 4,
};

uint8_t test_bw[] = {
	DP_LINK_BW_1_62,
	DP_LINK_BW_2_7,
};

uint8_t test_max_voltage[] = {
	DP_TRAIN_VOLTAGE_SWING_LEVEL_0,
	DP_TRAIN_VOLTAGE_SWING_LEVEL_1,
	DP_TRAIN_VOLTAGE_SWING_LEVEL_2,
	DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
};

uint8_t test_max_pre_emphasis[] = {
	DP_TRAIN_PRE_EMPH_LEVEL_0,
	DP_TRAIN_PRE_EMPH_LEVEL_1,
	DP_TRAIN_PRE_EMPH_LEVEL_2,
	DP_TRAIN_PRE_EMPH_LEVEL_3,
};

void
run_test(void)
{
	int lane, bw, voltage, emph;

	for (lane = 0; lane < ARRAY_SIZE(test_lanes); lane++)
	for (bw = 0; bw < ARRAY_SIZE(test_bw); bw++)
	for (voltage = 0; voltage < ARRAY_SIZE(test_max_voltage); voltage++)
	for (emph = 0; emph < ARRAY_SIZE(test_max_pre_emphasis); emph++) {
		DRM_DEBUG_KMS("l%d-bw%d-v%d-pe%d",
			      test_lanes[lane],
			      test_bw[bw],
			      test_max_voltage[voltage],
			      test_max_pre_emphasis[emph]);
		do_test(&simple_sink,
			test_lanes[lane],
			test_bw[bw],
			test_max_voltage[voltage],
			test_max_pre_emphasis[emph]);
	}
}

int
main(int argc, char *argv[])
{
	run_test();
}
