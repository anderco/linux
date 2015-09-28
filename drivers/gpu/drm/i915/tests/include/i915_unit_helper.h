#ifndef __I915_UNIT_HELPER_H_
#define __I915_UNIT_HELPER_H_

#include <drm/drm_unit_helper.h>
#include <drm/drm_log.h>

/* FIXME: Do a better split of the headers so these enums don't need to be
 * copied here. */

enum hdmi_force_audio {
	HDMI_AUDIO_OFF_DVI = -2,	/* no aux data for HDMI-DVI converter */
	HDMI_AUDIO_OFF,			/* force turn off HDMI audio */
	HDMI_AUDIO_AUTO,		/* trust EDID */
	HDMI_AUDIO_ON,			/* force turn on HDMI audio */
};

enum pipe {
	INVALID_PIPE = -1,
	PIPE_A = 0,
	PIPE_B,
	PIPE_C,
	_PIPE_EDP,
	I915_MAX_PIPES = _PIPE_EDP
};
#define pipe_name(p) ((p) + 'A')

enum transcoder {
	TRANSCODER_A = 0,
	TRANSCODER_B,
	TRANSCODER_C,
	TRANSCODER_EDP,
	I915_MAX_TRANSCODERS
};
#define transcoder_name(t) ((t) + 'A')

/*
 * This is the maximum (across all platforms) number of planes (primary +
 * sprites) that can be active at the same time on one pipe.
 *
 * This value doesn't count the cursor plane.
 */
#define I915_MAX_PLANES	4

enum plane {
	PLANE_A = 0,
	PLANE_B,
	PLANE_C,
};
#define plane_name(p) ((p) + 'A')

#define sprite_name(p, s) ((p) * INTEL_INFO(dev)->num_sprites[(p)] + (s) + 'A')

enum port {
	PORT_A = 0,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_E,
	I915_MAX_PORTS
};
#define port_name(p) ((p) + 'A')

#endif /* __I915_UNIT_HELPER_H_ */
