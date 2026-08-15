/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/ztest.h>

#include <ardep/can_log.h>
#include <ardep/uds.h>

static const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

/*
 * The gearshift is a zephyr,binary-encoded-gpio node: pins 0..2 of gpio0 carry
 * the position as a 3-bit little-endian number, so positions 0..7 are possible.
 * There is no gearshift hardware on native_sim, so drive the pins through the
 * GPIO emulator to stand in for it.
 */
static void set_gearshift_position(uint8_t pos)
{
	zassert_equal(gpio_emul_input_set(gpio0, 0, (pos >> 0) & 1), 0);
	zassert_equal(gpio_emul_input_set(gpio0, 1, (pos >> 1) & 1), 0);
	zassert_equal(gpio_emul_input_set(gpio0, 2, (pos >> 2) & 1), 0);
}

static void *gearshift_setup(void)
{
	zassert_true(device_is_ready(gpio0), "gpio0 not ready");
	return NULL;
}

/* Start every test from position 0 so a previous test cannot leak state. */
static void gearshift_before(void *f)
{
	ARG_UNUSED(f);
	set_gearshift_position(0);
}

ZTEST_SUITE(lib_gearshift_address_providers, NULL, gearshift_setup, gearshift_before,
	    NULL, NULL);

/*
 * The CAN log provider derives its ID as base + gearshift position, so each ECU
 * on the bus logs under a distinct ID. Sweep the low, an arbitrary middle and
 * the highest encodable position to confirm the offset is applied as-is and not
 * clamped or masked somewhere.
 */
ZTEST(lib_gearshift_address_providers, test_can_log_id_offsets)
{
	set_gearshift_position(0);
	zassert_equal(can_log_get_id(),
		      CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID);

	set_gearshift_position(2);
	zassert_equal(can_log_get_id(),
		      CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID + 2);

	set_gearshift_position(7);
	zassert_equal(can_log_get_id(),
		      CONFIG_GEARSHIFT_CAN_LOG_ADDRESS_PROVIDER_BASE_ID + 7);
}

/*
 * The UDS provider offsets both physical addresses by the same position, while
 * the functional addresses stay unused. Checking the functional pair matters as
 * much as the physical one: the provider must not accidentally offset them.
 */
ZTEST(lib_gearshift_address_providers, test_uds_address_offsets)
{
	set_gearshift_position(2);

	UDSISOTpCConfig_t cfg = uds_default_instance_get_addresses();

	zassert_equal(cfg.source_addr,
		      CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_SA + 2);
	zassert_equal(cfg.target_addr,
		      CONFIG_GEARSHIFT_UDS_ADDRESS_PROVIDER_BASE_PHYS_TA + 2);
	zassert_equal(cfg.source_addr_func, UDS_TP_NOOP_ADDR);
	zassert_equal(cfg.target_addr_func, UDS_TP_NOOP_ADDR);
}
