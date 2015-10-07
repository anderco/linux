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

#include "intel_drv.h"

/* These are source-specific values. */
static uint8_t
_dp_voltage_max(struct intel_dp *intel_dp)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	struct drm_i915_private *dev_priv = dev->dev_private;
	enum port port = dp_to_dig_port(intel_dp)->port;

	if (IS_BROXTON(dev))
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_3;
	else if (INTEL_INFO(dev)->gen >= 9) {
		if (dev_priv->edp_low_vswing && port == PORT_A)
			return DP_TRAIN_VOLTAGE_SWING_LEVEL_3;
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_2;
	} else if (IS_VALLEYVIEW(dev))
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_3;
	else if (IS_GEN7(dev) && port == PORT_A)
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_2;
	else if (HAS_PCH_CPT(dev) && port != PORT_A)
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_3;
	else
		return DP_TRAIN_VOLTAGE_SWING_LEVEL_2;
}

static uint8_t
_dp_pre_emphasis_max(struct intel_dp *intel_dp, uint8_t voltage_swing)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	enum port port = dp_to_dig_port(intel_dp)->port;

	if (INTEL_INFO(dev)->gen >= 9) {
		switch (voltage_swing & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			return DP_TRAIN_PRE_EMPH_LEVEL_3;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			return DP_TRAIN_PRE_EMPH_LEVEL_1;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		default:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		}
	} else if (IS_HASWELL(dev) || IS_BROADWELL(dev)) {
		switch (voltage_swing & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			return DP_TRAIN_PRE_EMPH_LEVEL_3;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			return DP_TRAIN_PRE_EMPH_LEVEL_1;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
		default:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		}
	} else if (IS_VALLEYVIEW(dev)) {
		switch (voltage_swing & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			return DP_TRAIN_PRE_EMPH_LEVEL_3;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			return DP_TRAIN_PRE_EMPH_LEVEL_1;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
		default:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		}
	} else if (IS_GEN7(dev) && port == PORT_A) {
		switch (voltage_swing & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			return DP_TRAIN_PRE_EMPH_LEVEL_1;
		default:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		}
	} else {
		switch (voltage_swing & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			return DP_TRAIN_PRE_EMPH_LEVEL_2;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			return DP_TRAIN_PRE_EMPH_LEVEL_1;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
		default:
			return DP_TRAIN_PRE_EMPH_LEVEL_0;
		}
	}
}

static void
hsw_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	uint32_t signal_levels = ddi_signal_levels(intel_dp);

	DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	intel_dp->DP &= ~DDI_BUF_EMP_MASK;
	intel_dp->DP |= signal_levels;
}

static const struct signal_levels hsw_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_2,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_3,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = hsw_set_signal_levels,
};

static const struct signal_levels skl_edp_low_vswing_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_3,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = hsw_set_signal_levels,
};

static void
bxt_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	ddi_signal_levels(intel_dp);
}

static const struct signal_levels bxt_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_3,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = bxt_set_signal_levels,
};

static void vlv_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_digital_port *dport = dp_to_dig_port(intel_dp);
	struct intel_crtc *intel_crtc =
		to_intel_crtc(dport->base.base.crtc);
	unsigned long demph_reg_value, preemph_reg_value,
		uniqtranscale_reg_value;
	enum dpio_channel port = vlv_dport_to_channel(dport);
	int pipe = intel_crtc->pipe;

	switch (train_set & DP_TRAIN_PRE_EMPHASIS_MASK) {
	case DP_TRAIN_PRE_EMPH_LEVEL_0:
		preemph_reg_value = 0x0004000;
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			demph_reg_value = 0x2B405555;
			uniqtranscale_reg_value = 0x552AB83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			demph_reg_value = 0x2B404040;
			uniqtranscale_reg_value = 0x5548B83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			demph_reg_value = 0x2B245555;
			uniqtranscale_reg_value = 0x5560B83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
			demph_reg_value = 0x2B405555;
			uniqtranscale_reg_value = 0x5598DA3A;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_1:
		preemph_reg_value = 0x0002000;
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			demph_reg_value = 0x2B404040;
			uniqtranscale_reg_value = 0x5552B83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			demph_reg_value = 0x2B404848;
			uniqtranscale_reg_value = 0x5580B83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			demph_reg_value = 0x2B404040;
			uniqtranscale_reg_value = 0x55ADDA3A;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_2:
		preemph_reg_value = 0x0000000;
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			demph_reg_value = 0x2B305555;
			uniqtranscale_reg_value = 0x5570B83A;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			demph_reg_value = 0x2B2B4040;
			uniqtranscale_reg_value = 0x55ADDA3A;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_3:
		preemph_reg_value = 0x0006000;
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			demph_reg_value = 0x1B405555;
			uniqtranscale_reg_value = 0x55ADDA3A;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	default:
		MISSING_CASE(train_set & DP_TRAIN_PRE_EMPHASIS_MASK);
		return;
	}

	mutex_lock(&dev_priv->sb_lock);
	vlv_dpio_write(dev_priv, pipe, VLV_TX_DW5(port), 0x00000000);
	vlv_dpio_write(dev_priv, pipe, VLV_TX_DW4(port), demph_reg_value);
	vlv_dpio_write(dev_priv, pipe, VLV_TX_DW2(port),
			 uniqtranscale_reg_value);
	vlv_dpio_write(dev_priv, pipe, VLV_TX_DW3(port), 0x0C782040);
	vlv_dpio_write(dev_priv, pipe, VLV_PCS_DW11(port), 0x00030000);
	vlv_dpio_write(dev_priv, pipe, VLV_PCS_DW9(port), preemph_reg_value);
	vlv_dpio_write(dev_priv, pipe, VLV_TX_DW5(port), 0x80000000);
	mutex_unlock(&dev_priv->sb_lock);
}

static const struct signal_levels vlv_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_3,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = vlv_set_signal_levels,
};

static bool chv_need_uniq_trans_scale(uint8_t train_set)
{
	return (train_set & DP_TRAIN_PRE_EMPHASIS_MASK) == DP_TRAIN_PRE_EMPH_LEVEL_0 &&
		(train_set & DP_TRAIN_VOLTAGE_SWING_MASK) == DP_TRAIN_VOLTAGE_SWING_LEVEL_3;
}

static void chv_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct intel_digital_port *dport = dp_to_dig_port(intel_dp);
	struct intel_crtc *intel_crtc = to_intel_crtc(dport->base.base.crtc);
	u32 deemph_reg_value, margin_reg_value, val;
	enum dpio_channel ch = vlv_dport_to_channel(dport);
	enum pipe pipe = intel_crtc->pipe;
	int i;

	switch (train_set & DP_TRAIN_PRE_EMPHASIS_MASK) {
	case DP_TRAIN_PRE_EMPH_LEVEL_0:
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			deemph_reg_value = 128;
			margin_reg_value = 52;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			deemph_reg_value = 128;
			margin_reg_value = 77;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			deemph_reg_value = 128;
			margin_reg_value = 102;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
			deemph_reg_value = 128;
			margin_reg_value = 154;
			/* FIXME extra to set for 1200 */
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_1:
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			deemph_reg_value = 85;
			margin_reg_value = 78;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			deemph_reg_value = 85;
			margin_reg_value = 116;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
			deemph_reg_value = 85;
			margin_reg_value = 154;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_2:
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			deemph_reg_value = 64;
			margin_reg_value = 104;
			break;
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
			deemph_reg_value = 64;
			margin_reg_value = 154;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_3:
		switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
		case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
			deemph_reg_value = 43;
			margin_reg_value = 154;
			break;
		default:
			MISSING_CASE(train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
			return;
		}
		break;
	default:
		MISSING_CASE(train_set & DP_TRAIN_PRE_EMPHASIS_MASK);
		return;
	}

	mutex_lock(&dev_priv->sb_lock);

	/* Clear calc init */
	val = vlv_dpio_read(dev_priv, pipe, VLV_PCS01_DW10(ch));
	val &= ~(DPIO_PCS_SWING_CALC_TX0_TX2 | DPIO_PCS_SWING_CALC_TX1_TX3);
	val &= ~(DPIO_PCS_TX1DEEMP_MASK | DPIO_PCS_TX2DEEMP_MASK);
	val |= DPIO_PCS_TX1DEEMP_9P5 | DPIO_PCS_TX2DEEMP_9P5;
	vlv_dpio_write(dev_priv, pipe, VLV_PCS01_DW10(ch), val);

	if (intel_crtc->config->lane_count > 2) {
		val = vlv_dpio_read(dev_priv, pipe, VLV_PCS23_DW10(ch));
		val &= ~(DPIO_PCS_SWING_CALC_TX0_TX2 | DPIO_PCS_SWING_CALC_TX1_TX3);
		val &= ~(DPIO_PCS_TX1DEEMP_MASK | DPIO_PCS_TX2DEEMP_MASK);
		val |= DPIO_PCS_TX1DEEMP_9P5 | DPIO_PCS_TX2DEEMP_9P5;
		vlv_dpio_write(dev_priv, pipe, VLV_PCS23_DW10(ch), val);
	}

	val = vlv_dpio_read(dev_priv, pipe, VLV_PCS01_DW9(ch));
	val &= ~(DPIO_PCS_TX1MARGIN_MASK | DPIO_PCS_TX2MARGIN_MASK);
	val |= DPIO_PCS_TX1MARGIN_000 | DPIO_PCS_TX2MARGIN_000;
	vlv_dpio_write(dev_priv, pipe, VLV_PCS01_DW9(ch), val);

	if (intel_crtc->config->lane_count > 2) {
		val = vlv_dpio_read(dev_priv, pipe, VLV_PCS23_DW9(ch));
		val &= ~(DPIO_PCS_TX1MARGIN_MASK | DPIO_PCS_TX2MARGIN_MASK);
		val |= DPIO_PCS_TX1MARGIN_000 | DPIO_PCS_TX2MARGIN_000;
		vlv_dpio_write(dev_priv, pipe, VLV_PCS23_DW9(ch), val);
	}

	/* Program swing deemph */
	for (i = 0; i < intel_crtc->config->lane_count; i++) {
		val = vlv_dpio_read(dev_priv, pipe, CHV_TX_DW4(ch, i));
		val &= ~DPIO_SWING_DEEMPH9P5_MASK;
		val |= deemph_reg_value << DPIO_SWING_DEEMPH9P5_SHIFT;
		vlv_dpio_write(dev_priv, pipe, CHV_TX_DW4(ch, i), val);
	}

	/* Program swing margin */
	for (i = 0; i < intel_crtc->config->lane_count; i++) {
		val = vlv_dpio_read(dev_priv, pipe, CHV_TX_DW2(ch, i));

		val &= ~DPIO_SWING_MARGIN000_MASK;
		val |= margin_reg_value << DPIO_SWING_MARGIN000_SHIFT;

		/*
		 * Supposedly this value shouldn't matter when unique transition
		 * scale is disabled, but in fact it does matter. Let's just
		 * always program the same value and hope it's OK.
		 */
		val &= ~(0xff << DPIO_UNIQ_TRANS_SCALE_SHIFT);
		val |= 0x9a << DPIO_UNIQ_TRANS_SCALE_SHIFT;

		vlv_dpio_write(dev_priv, pipe, CHV_TX_DW2(ch, i), val);
	}

	/*
	 * The document said it needs to set bit 27 for ch0 and bit 26
	 * for ch1. Might be a typo in the doc.
	 * For now, for this unique transition scale selection, set bit
	 * 27 for ch0 and ch1.
	 */
	for (i = 0; i < intel_crtc->config->lane_count; i++) {
		val = vlv_dpio_read(dev_priv, pipe, CHV_TX_DW3(ch, i));
		if (chv_need_uniq_trans_scale(train_set))
			val |= DPIO_TX_UNIQ_TRANS_SCALE_EN;
		else
			val &= ~DPIO_TX_UNIQ_TRANS_SCALE_EN;
		vlv_dpio_write(dev_priv, pipe, CHV_TX_DW3(ch, i), val);
	}

	/* Start swing calculation */
	val = vlv_dpio_read(dev_priv, pipe, VLV_PCS01_DW10(ch));
	val |= DPIO_PCS_SWING_CALC_TX0_TX2 | DPIO_PCS_SWING_CALC_TX1_TX3;
	vlv_dpio_write(dev_priv, pipe, VLV_PCS01_DW10(ch), val);

	if (intel_crtc->config->lane_count > 2) {
		val = vlv_dpio_read(dev_priv, pipe, VLV_PCS23_DW10(ch));
		val |= DPIO_PCS_SWING_CALC_TX0_TX2 | DPIO_PCS_SWING_CALC_TX1_TX3;
		vlv_dpio_write(dev_priv, pipe, VLV_PCS23_DW10(ch), val);
	}

	mutex_unlock(&dev_priv->sb_lock);
}

static const struct signal_levels chv_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_3,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = chv_set_signal_levels,
};

static void
gen4_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	uint32_t signal_levels = 0;

	switch (train_set & DP_TRAIN_VOLTAGE_SWING_MASK) {
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0:
	default:
		signal_levels |= DP_VOLTAGE_0_4;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1:
		signal_levels |= DP_VOLTAGE_0_6;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2:
		signal_levels |= DP_VOLTAGE_0_8;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_3:
		signal_levels |= DP_VOLTAGE_1_2;
		break;
	}
	switch (train_set & DP_TRAIN_PRE_EMPHASIS_MASK) {
	case DP_TRAIN_PRE_EMPH_LEVEL_0:
	default:
		signal_levels |= DP_PRE_EMPHASIS_0;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels |= DP_PRE_EMPHASIS_3_5;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_2:
		signal_levels |= DP_PRE_EMPHASIS_6;
		break;
	case DP_TRAIN_PRE_EMPH_LEVEL_3:
		signal_levels |= DP_PRE_EMPHASIS_9_5;
		break;
	}

	DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	intel_dp->DP &= ~(DP_VOLTAGE_MASK | DP_PRE_EMPHASIS_MASK);
	intel_dp->DP |= signal_levels;
}

static const struct signal_levels gen4_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_2,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = gen4_set_signal_levels,
};

static const struct signal_levels snb_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_3,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = gen4_set_signal_levels,
};

/* SNB's DP voltage swing and pre-emphasis control */
static void
snb_edp_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	uint32_t signal_levels;

	train_set &=
		(DP_TRAIN_VOLTAGE_SWING_MASK | DP_TRAIN_PRE_EMPHASIS_MASK);

	switch (train_set) {
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_0:
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1 | DP_TRAIN_PRE_EMPH_LEVEL_0:
		signal_levels =  EDP_LINK_TRAIN_400_600MV_0DB_SNB_B;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels = EDP_LINK_TRAIN_400MV_3_5DB_SNB_B;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_2:
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1 | DP_TRAIN_PRE_EMPH_LEVEL_2:
		signal_levels = EDP_LINK_TRAIN_400_600MV_6DB_SNB_B;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1 | DP_TRAIN_PRE_EMPH_LEVEL_1:
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2 | DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels = EDP_LINK_TRAIN_600_800MV_3_5DB_SNB_B;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2 | DP_TRAIN_PRE_EMPH_LEVEL_0:
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_3 | DP_TRAIN_PRE_EMPH_LEVEL_0:
		signal_levels = EDP_LINK_TRAIN_800_1200MV_0DB_SNB_B;
		break;
	default:
		DRM_DEBUG_KMS("Unsupported voltage swing/pre-emphasis level:"
			      "0x%x\n", train_set);
		signal_levels = EDP_LINK_TRAIN_400_600MV_0DB_SNB_B;
	}

	DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	intel_dp->DP &= ~EDP_LINK_TRAIN_VOL_EMP_MASK_SNB;
	intel_dp->DP |= signal_levels;
}

static const struct signal_levels snb_edp_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_2,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = snb_edp_set_signal_levels,
};

/* IVB's DP voltage swing and pre-emphasis control */
static void
ivb_edp_set_signal_levels(struct intel_dp *intel_dp, uint8_t train_set)
{
	uint32_t signal_levels;

	train_set &=
		(DP_TRAIN_VOLTAGE_SWING_MASK | DP_TRAIN_PRE_EMPHASIS_MASK);

	switch (train_set) {
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_0:
		signal_levels = EDP_LINK_TRAIN_400MV_0DB_IVB;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels = EDP_LINK_TRAIN_400MV_3_5DB_IVB;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_0 | DP_TRAIN_PRE_EMPH_LEVEL_2:
		signal_levels = EDP_LINK_TRAIN_400MV_6DB_IVB;
		break;

	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1 | DP_TRAIN_PRE_EMPH_LEVEL_0:
		signal_levels = EDP_LINK_TRAIN_600MV_0DB_IVB;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_1 | DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels = EDP_LINK_TRAIN_600MV_3_5DB_IVB;
		break;

	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2 | DP_TRAIN_PRE_EMPH_LEVEL_0:
		signal_levels = EDP_LINK_TRAIN_800MV_0DB_IVB;
		break;
	case DP_TRAIN_VOLTAGE_SWING_LEVEL_2 | DP_TRAIN_PRE_EMPH_LEVEL_1:
		signal_levels = EDP_LINK_TRAIN_800MV_3_5DB_IVB;
		break;

	default:
		DRM_DEBUG_KMS("Unsupported voltage swing/pre-emphasis level:"
			      "0x%x\n", train_set);
		signal_levels = EDP_LINK_TRAIN_500MV_0DB_IVB;
	}

	DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	intel_dp->DP &= ~EDP_LINK_TRAIN_VOL_EMP_MASK_IVB;
	intel_dp->DP |= signal_levels;
}

static const struct signal_levels ivb_edp_signal_levels = {
	.max_voltage = DP_TRAIN_VOLTAGE_SWING_LEVEL_2,
	.max_pre_emph = {
		DP_TRAIN_PRE_EMPH_LEVEL_2,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_1,
		DP_TRAIN_PRE_EMPH_LEVEL_0,
	},

	.set = ivb_edp_set_signal_levels,
};

static void
_update_signal_levels(struct intel_dp *intel_dp)
{
	struct intel_digital_port *intel_dig_port = dp_to_dig_port(intel_dp);
	struct drm_device *dev = intel_dig_port->base.base.dev;
	uint32_t signal_levels, mask = 0;

	if (HAS_DDI(dev)) {
		signal_levels = ddi_signal_levels(intel_dp);

		if (IS_BROXTON(dev))
			signal_levels = 0;
		else
			mask = DDI_BUF_EMP_MASK;
	} else {
		WARN(1, "Should be calling intel_dp->signal_levels->set instead.");
		return;
	}

	if (mask)
		DRM_DEBUG_KMS("Using signal levels %08x\n", signal_levels);

	intel_dp->DP = (intel_dp->DP & ~mask) | signal_levels;
}

uint8_t
intel_dp_voltage_max(struct intel_dp *intel_dp)
{
	if (intel_dp->signal_levels)
		return intel_dp->signal_levels->max_voltage;
	else
		return _dp_voltage_max(intel_dp);
}

uint8_t
intel_dp_pre_emphasis_max(struct intel_dp *intel_dp, uint8_t voltage_swing)
{
	if (intel_dp->signal_levels)
		return intel_dp->signal_levels->max_pre_emph[voltage_swing];
	else
		return _dp_pre_emphasis_max(intel_dp, voltage_swing);
}

void
intel_dp_set_signal_levels(struct intel_dp *intel_dp)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	struct drm_i915_private *dev_priv = to_i915(dev);
	uint8_t train_set = intel_dp->train_set[0];

	if (intel_dp->signal_levels)
		intel_dp->signal_levels->set(intel_dp, train_set);
	else
		_update_signal_levels(intel_dp);

	DRM_DEBUG_KMS("Using vswing level %d\n",
		train_set & DP_TRAIN_VOLTAGE_SWING_MASK);
	DRM_DEBUG_KMS("Using pre-emphasis level %d\n",
		(train_set & DP_TRAIN_PRE_EMPHASIS_MASK) >>
			DP_TRAIN_PRE_EMPHASIS_SHIFT);

	I915_WRITE(intel_dp->output_reg, intel_dp->DP);
	POSTING_READ(intel_dp->output_reg);
}

void
intel_dp_init_signal_levels(struct intel_dp *intel_dp)
{
	struct drm_device *dev = intel_dp_to_dev(intel_dp);
	struct drm_i915_private *dev_priv = to_i915(dev);
	struct intel_digital_port *intel_dig_port = dp_to_dig_port(intel_dp);
	enum port port = intel_dig_port->port;

	if (IS_BROXTON(dev)) {
		intel_dp->signal_levels = &bxt_signal_levels;
	} else if (IS_SKYLAKE(dev) &&
		   port == PORT_A && dev_priv->edp_low_vswing) {
		intel_dp->signal_levels = &skl_edp_low_vswing_signal_levels;
	} else if (HAS_DDI(dev)) {
		intel_dp->signal_levels = &hsw_signal_levels;
	} else if (IS_CHERRYVIEW(dev)) {
		intel_dp->signal_levels = &chv_signal_levels;
	} else if (IS_VALLEYVIEW(dev) && !IS_CHERRYVIEW(dev)) {
		intel_dp->signal_levels = &vlv_signal_levels;
	} else if (IS_IVYBRIDGE(dev)) {
		if (port == PORT_A)
			intel_dp->signal_levels = &ivb_edp_signal_levels;
		else
			intel_dp->signal_levels = &snb_signal_levels;
	} else if (IS_GEN6(dev)) {
		if (port == PORT_A)
			intel_dp->signal_levels = &snb_edp_signal_levels;
		else
			intel_dp->signal_levels = &snb_signal_levels;
	} else if (INTEL_INFO(dev)->gen <= 5)
		intel_dp->signal_levels = &gen4_signal_levels;
}
