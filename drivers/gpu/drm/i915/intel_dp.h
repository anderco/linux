#ifndef __INTEL_DP_H__
#define __INTEL_DP_H__

#include <drm/drm_dp_helper.h>
#include <drm/drm_dp_mst_helper.h>

#include <i915_unit_helper.h>

#include "intel_bios.h"

#define DP_MAX_DOWNSTREAM_PORTS		0x10

struct sink_crc {
	bool started;
	u8 last_crc[6];
	int last_count;
};

struct intel_dp;
struct signal_levels {
	uint8_t max_voltage;
	uint8_t max_pre_emph[4];

	void (*set)(struct intel_dp *intel_dp, uint8_t train_set);
};

struct intel_dp {
	uint32_t output_reg;
	uint32_t aux_ch_ctl_reg;
	uint32_t DP;
	int link_rate;
	uint8_t lane_count;
	bool has_audio;
	enum hdmi_force_audio force_audio;
	bool limited_color_range;
	bool color_range_auto;
	uint8_t dpcd[DP_RECEIVER_CAP_SIZE];
	uint8_t psr_dpcd[EDP_PSR_RECEIVER_CAP_SIZE];
	uint8_t downstream_ports[DP_MAX_DOWNSTREAM_PORTS];
	/* sink rates as reported by DP_SUPPORTED_LINK_RATES */
	uint8_t num_sink_rates;
	int sink_rates[DP_MAX_SUPPORTED_RATES];
	struct sink_crc sink_crc;
	struct drm_dp_aux aux;
	const struct signal_levels *signal_levels;
	uint8_t train_set[4];
	int panel_power_up_delay;
	int panel_power_down_delay;
	int panel_power_cycle_delay;
	int backlight_on_delay;
	int backlight_off_delay;
	struct delayed_work panel_vdd_work;
	bool want_panel_vdd;
	unsigned long last_power_cycle;
	unsigned long last_power_on;
	unsigned long last_backlight_off;

	struct notifier_block edp_notifier;

	/*
	 * Pipe whose power sequencer is currently locked into
	 * this port. Only relevant on VLV/CHV.
	 */
	enum pipe pps_pipe;
	struct edp_power_seq pps_delays;

	bool can_mst; /* this port supports mst */
	bool is_mst;
	int active_mst_links;
	/* connector directly attached - won't be use for modeset in mst world */
	struct intel_connector *attached_connector;

	/* mst connector list */
	struct intel_dp_mst_encoder *mst_encoders[I915_MAX_PIPES];
	struct drm_dp_mst_topology_mgr mst_mgr;

	uint32_t (*get_aux_clock_divider)(struct intel_dp *dp, int index);
	/*
	 * This function returns the value we have to program the AUX_CTL
	 * register with to kick off an AUX transaction.
	 */
	uint32_t (*get_aux_send_ctl)(struct intel_dp *dp,
				     bool has_aux_irq,
				     int send_bytes,
				     uint32_t aux_clock_divider);

	/* This is called before a link training is starterd */
	void (*prepare_link_retrain)(struct intel_dp *intel_dp);

	bool train_set_valid;

	/* Displayport compliance testing */
	unsigned long compliance_test_type;
	unsigned long compliance_test_data;
	bool compliance_test_active;
};

/* intel_dp.c */
struct drm_encoder;
struct drm_plane;

struct drm_i915_private;
struct intel_crtc_state;
struct intel_digital_port;
struct intel_encoder;

void intel_dp_init(struct drm_device *dev, int output_reg, enum port port);
bool intel_dp_init_connector(struct intel_digital_port *intel_dig_port,
			     struct intel_connector *intel_connector);
void intel_dp_set_link_params(struct intel_dp *intel_dp,
			      const struct intel_crtc_state *pipe_config);
void intel_dp_start_link_train(struct intel_dp *intel_dp);
void intel_dp_stop_link_train(struct intel_dp *intel_dp);
void intel_dp_sink_dpms(struct intel_dp *intel_dp, int mode);
void intel_dp_encoder_destroy(struct drm_encoder *encoder);
int intel_dp_sink_crc(struct intel_dp *intel_dp, u8 *crc);
bool intel_dp_compute_config(struct intel_encoder *encoder,
			     struct intel_crtc_state *pipe_config);
bool intel_dp_is_edp(struct drm_device *dev, enum port port);
enum irqreturn intel_dp_hpd_pulse(struct intel_digital_port *intel_dig_port,
				  bool long_hpd);
void intel_edp_backlight_on(struct intel_dp *intel_dp);
void intel_edp_backlight_off(struct intel_dp *intel_dp);
void intel_edp_panel_vdd_on(struct intel_dp *intel_dp);
void intel_edp_panel_on(struct intel_dp *intel_dp);
void intel_edp_panel_off(struct intel_dp *intel_dp);
void intel_dp_add_properties(struct intel_dp *intel_dp, struct drm_connector *connector);
void intel_dp_mst_suspend(struct drm_device *dev);
void intel_dp_mst_resume(struct drm_device *dev);
int intel_dp_max_link_rate(struct intel_dp *intel_dp);
int intel_dp_rate_select(struct intel_dp *intel_dp, int rate);
void intel_dp_hot_plug(struct intel_encoder *intel_encoder);
void vlv_power_sequencer_reset(struct drm_i915_private *dev_priv);
uint32_t intel_dp_pack_aux(const uint8_t *src, int src_bytes);
void intel_plane_destroy(struct drm_plane *plane);
void intel_edp_drrs_enable(struct intel_dp *intel_dp);
void intel_edp_drrs_disable(struct intel_dp *intel_dp);
void intel_edp_drrs_invalidate(struct drm_device *dev,
		unsigned frontbuffer_bits);
void intel_edp_drrs_flush(struct drm_device *dev, unsigned frontbuffer_bits);
bool intel_digital_port_connected(struct drm_i915_private *dev_priv,
					 struct intel_digital_port *port);
void hsw_dp_set_ddi_pll_sel(struct intel_crtc_state *pipe_config);

void
intel_dp_program_link_training_pattern(struct intel_dp *intel_dp,
				       uint8_t dp_train_pat);
void
intel_dp_update_signal_levels(struct intel_dp *intel_dp);
void intel_dp_set_idle_link_train(struct intel_dp *intel_dp);
uint8_t
intel_dp_voltage_max(struct intel_dp *intel_dp);
uint8_t
intel_dp_pre_emphasis_max(struct intel_dp *intel_dp, uint8_t voltage_swing);
void intel_dp_compute_rate(struct intel_dp *intel_dp, int port_clock,
			   uint8_t *link_bw, uint8_t *rate_select);
bool intel_dp_source_supports_hbr2(struct intel_dp *intel_dp);
bool
intel_dp_get_link_status(struct intel_dp *intel_dp, uint8_t link_status[DP_LINK_STATUS_SIZE]);

void intel_dp_init_signal_levels(struct intel_dp *intel_dp);


#endif /* __INTEL_DP_H__ */
