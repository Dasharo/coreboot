/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __FW_CONFIG__
#define __FW_CONFIG__

#include <device/device.h>
#include <static.h>  /* Provides fw_config definitions from devicetree.cb */
#include <stdbool.h>
#include <stdint.h>

/**
 * struct fw_config - Firmware configuration field and option.
 * @field_name: Name of the field that this option belongs to.
 * @option_name: Name of the option within this field.
 * @mask: Bitmask of the field.
 * @value: Value of the option within the mask.
 */
struct fw_config {
	const char *field_name;
	const char *option_name;
	uint32_t mask;
	uint32_t value;
};

/* Generate a pointer to a compound literal of the fw_config structure. */
#define FW_CONFIG(__field, __option)	(&(const struct fw_config) {		\
	.field_name = FW_CONFIG_FIELD_##__field##_NAME,				\
	.option_name = FW_CONFIG_FIELD_##__field##_OPTION_##__option##_NAME,	\
	.mask = FW_CONFIG_FIELD_##__field##_MASK,				\
	.value = FW_CONFIG_FIELD_##__field##_OPTION_##__option##_VALUE		\
})

#if CONFIG(FW_CONFIG)

/**
 * fw_config_probe() - Check if field and option matches.
 * @match: Structure containing field and option to probe.
 *
 * Return %true if match is found, %false if match is not found.
 */
bool fw_config_probe(const struct fw_config *match);

/**
 * fw_config_for_each_found() - Call a callback for each fw_config field found
 * @cb: The callback function
 * @arg: A context argument that is passed to the callback
 */
void fw_config_for_each_found(void (*cb)(const struct fw_config *config, void *arg), void *arg);

/**
 * fw_config_get_found() - Return a pointer to the fw_config struct for a given field.
 * @field_mask: A field mask from static.h, e.g., FW_CONFIG_FIELD_FEATURE_MASK
 *
 * Return pointer to cached `struct fw_config` if successfully probed, otherwise NULL.
*/
const struct fw_config *fw_config_get_found(uint32_t field_mask);

#else

static inline bool fw_config_probe(const struct fw_config *match)
{
	/* Always return true when probing with disabled fw_config. */
	return true;
}

#endif /* CONFIG(FW_CONFIG) */

#endif /* __FW_CONFIG__ */
