/*
 * Copyright © 2008-2015 Intel Corporation
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
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <drm/drm_dp_helper.h>
#include <drm/drm_unit_helper.h>
#include <i915_unit_helper.h>

#define I915_WRITE(reg, val)
#define POSTING_READ(reg)
#define MISSING_CASE(...)

#define INTEL_INFO(dev) ((struct drm_i915_private *) (dev)->dev_private)

enum dpio_channel {
	DPIO_CH0,
	DPIO_CH1
};

struct intel_digital_port;
static inline enum dpio_channel
vlv_dport_to_channel(struct intel_digital_port *dport)
{
	return DPIO_CH0;
}

static inline
u32 vlv_dpio_read(void *dev_priv, enum pipe pipe, int reg)
{
	return 0;
}

static inline
void vlv_dpio_write(void *dev_priv, enum pipe pipe, int reg, u32 val)
{
}

struct intel_dp;
struct signal_levels {
	uint8_t max_voltage;
	uint8_t max_pre_emph[4];

	void (*set)(struct intel_dp *intel_dp, uint8_t train_set);
};

struct intel_crtc_config {
	int lane_count;
};

struct drm_crtc {
};

struct intel_crtc {
	struct drm_crtc base;
	struct intel_crtc_config *config;
	enum pipe pipe;
};

static inline struct intel_crtc *
to_intel_crtc(struct drm_crtc *crtc)
{
	return (struct intel_crtc *) crtc;
}

struct drm_encoder {
	struct drm_device *dev;
	struct drm_crtc *crtc;
};

struct intel_encoder {
	struct drm_encoder base;
	int type;
};

struct intel_dp {
	uint32_t output_reg;
	uint32_t DP;
	uint8_t train_set[4];
	const struct signal_levels *signal_levels;
};

struct intel_digital_port {
	struct intel_encoder base;
	struct intel_dp dp;
	enum port port;
};

static struct intel_digital_port *
dp_to_dig_port(struct intel_dp *intel_dp)
{
	return container_of(intel_dp, struct intel_digital_port, dp);
}

static struct drm_device *
intel_dp_to_dev(struct intel_dp *intel_dp)
{
	struct intel_digital_port *dport = dp_to_dig_port(intel_dp);

	return dport->base.base.dev;
}

struct drm_i915_private {
	int gen;
	bool is_atom;
	bool has_pch_cpt;
	bool is_haswell;
	bool edp_low_vswing;
	struct mutex sb_lock;
};

static inline struct drm_i915_private *
to_i915(struct drm_device *dev)
{
	return (struct drm_i915_private *) dev->dev_private;
}

static bool is_(struct drm_device *dev, int gen, bool atom)
{
	return INTEL_INFO(dev)->gen == gen && INTEL_INFO(dev)->is_atom == atom;
}

static bool IS_SKYLAKE(struct drm_device *dev) { return is_(dev, 9, false); }
static bool IS_BROADWELL(struct drm_device *dev) { return is_(dev, 8, false); }
static bool IS_BROXTON(struct drm_device *dev) { return is_(dev, 9, true); }
static bool IS_CHERRYVIEW(struct drm_device *dev) { return is_(dev, 8, true); }

static bool IS_VALLEYVIEW(struct drm_device *dev)
{
	return is_(dev, 7, true) || is_(dev, 8, true);
}

static bool IS_IVYBRIDGE(struct drm_device *dev)
{
	return is_(dev, 7, false) && !INTEL_INFO(dev)->is_haswell && !INTEL_INFO(dev)->is_atom;
}

static bool IS_HASWELL(struct drm_device *dev)
{
	return is_(dev, 7, false) && !IS_IVYBRIDGE(dev);
}

static bool HAS_DDI(struct drm_device *dev)
{
	if (INTEL_INFO(dev)->gen >= 9)
		return true;

	if (INTEL_INFO(dev)->is_atom)
		return false;

	if (INTEL_INFO(dev)->gen < 7)
		return false;

	if (INTEL_INFO(dev)->gen == 8)
		return true;

	if (INTEL_INFO(dev)->is_haswell)
		return true;

	return false;
}

static bool IS_GEN6(struct drm_device *dev) { return INTEL_INFO(dev)->gen == 6; }
static bool IS_GEN7(struct drm_device *dev) { return INTEL_INFO(dev)->gen == 7; }

static bool HAS_PCH_CPT(struct drm_device *dev)
{
	return IS_GEN6(dev) || IS_IVYBRIDGE(dev);
}


static void skl_ddi_set_iboost(struct drm_device *dev, u32 level,
			       enum port port, int type) {}
static void bxt_ddi_vswing_sequence(struct drm_device *dev, u32 level,
				    enum port port, int type) {}
uint32_t ddi_signal_levels(struct intel_dp *intel_dp) { return 0; }


/* Prevent inclusion of intel_dp.h */
#define __INTEL_DP_H__

#include "../intel_dp_signal_levels.c"

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

int
main(int argc, char *argv[])
{
	struct drm_device dev = {0};
	struct drm_i915_private dev_priv = {0};
	struct intel_digital_port dport = {0};

	dev.dev_private = &dev_priv;
	dport.base.base.dev = &dev;

	for (int gen = 2; gen <= 9; gen++)
	for (int port_a = 0; port_a < 2; port_a++)
	for (int is_atom = 0; is_atom < 2; is_atom++)
	for (int is_haswell = 0; is_haswell < 2; is_haswell++)
	for (int edp_low_vswing = 0; edp_low_vswing < 2; edp_low_vswing++)
	for (int has_pch_cpt = 0; has_pch_cpt < 2; has_pch_cpt++)
	{
		struct intel_dp *intel_dp = &dport.dp;
		uint8_t voltage, max_voltage, pre_emph;

		/* is_atom is only relevant for IS_VLV, CHV, BXT */
		if (is_atom && gen < 7)
			continue;

		dev_priv.gen = gen;
		dev_priv.is_atom = is_atom;
		dev_priv.is_haswell = is_haswell;
		dev_priv.edp_low_vswing = edp_low_vswing;
		dev_priv.has_pch_cpt = has_pch_cpt;

		dport.port = port_a ? PORT_A : PORT_B;

		intel_dp_init_signal_levels(intel_dp);
		max_voltage = intel_dp_voltage_max(intel_dp);

		printf("%d %d %d %d %d %d\n",
		       gen, port_a, is_atom, is_haswell, edp_low_vswing, has_pch_cpt);
		printf("max voltage: %x\n", max_voltage);

		printf("max pre emph: ");

		for (voltage = 0; voltage <= max_voltage; voltage++) {
			pre_emph = intel_dp_pre_emphasis_max(intel_dp, voltage);
			printf("%x, ", pre_emph);
		}

		printf("\n");
	}
}
