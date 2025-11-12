/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "devices.h"
#include "regs.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/fff.h>
#include <zephyr/ztest.h>

#include <ardep/drivers/emul/power_io_shield.h>
#include <ardep/dt-bindings/power-io-shield.h>

DEFINE_FFF_GLOBALS;

ZTEST_SUITE(mcp_driver, NULL, NULL, NULL, NULL, NULL);

ZTEST(mcp_driver, test_iodir_and_iocon_init) {
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_IODIRA),
      0x7F03);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_IOCONA),
      0x4444);
}

ZTEST(mcp_driver, test_set_output_0) {
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_OUTPUT(0), 1), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0004);  // Bank A: 0x04, Bank B: 0x00

  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_OUTPUT(0), 0), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0000);
}

ZTEST(mcp_driver, test_set_output_5) {
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_OUTPUT(5), 1), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0080);  // Bank A: 0x80, Bank B: 0x00

  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_OUTPUT(5), 0), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0000);
}

ZTEST(mcp_driver, test_read_inputs_and_faults) {
  gpio_port_value_t value;

  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_GPIOA,
                                   0xD242);  // set input 1 and 4, fault 0 and 2

  zassert_equal(gpio_port_get(power_io_shield, &value), 0);
  zassert_equal((value >> POWER_IO_SHIELD_INPUT_BASE) & 0x3F, 0b10010);
  zassert_equal((value >> POWER_IO_SHIELD_FAULT_BASE) & 0x7, 0b101);

  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_GPIOA,
                                   0x0003);  // set fault 1 and 2
  zassert_equal(gpio_port_get(power_io_shield, &value), 0);
  zassert_equal((value >> POWER_IO_SHIELD_FAULT_BASE), 0b110);

  // reset
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_GPIOA, 0x0000);
  zassert_equal(gpio_port_get(power_io_shield, &value), 0);
  zassert_equal(value, 0);
}

ZTEST(mcp_driver, test_configure_outputs) {
  // configure with active flag (meaning active on configure)
  zassert_equal(gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                                   GPIO_OUTPUT_ACTIVE),
                0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0004);

  // reconfigure without ACTIVE flag
  zassert_equal(gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                                   GPIO_OUTPUT),
                0);

  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0000);

  // try to configure with invalid flags
  zassert_not_equal(gpio_pin_configure(power_io_shield,
                                       POWER_IO_SHIELD_OUTPUT(0), GPIO_INPUT),
                    0);
  zassert_not_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                         GPIO_OUTPUT | GPIO_OPEN_DRAIN),
      0);
  zassert_not_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                         GPIO_OUTPUT | GPIO_PULL_UP),
      0);
  zassert_not_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                         GPIO_OUTPUT | GPIO_PULL_DOWN),
      0);
}

ZTEST(mcp_driver, test_configure_inputs) {
  zassert_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_INPUT(0), GPIO_INPUT),
      0);
  zassert_not_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_INPUT(0),
                         GPIO_INPUT | GPIO_PULL_DOWN),
      0);
  zassert_not_equal(
      gpio_pin_configure(power_io_shield, POWER_IO_SHIELD_INPUT(0),
                         GPIO_INPUT | GPIO_PULL_UP),
      0);
  zassert_not_equal(gpio_pin_configure(power_io_shield,
                                       POWER_IO_SHIELD_INPUT(0), GPIO_OUTPUT),
                    0);
}

ZTEST(mcp_driver, test_setting_input_pin_is_ignored) {
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_INPUT(0), 1), 0);
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_INPUT(4), 1), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0000);
}

ZTEST(mcp_driver, test_setting_fault_pin_is_ignored) {
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_FAULT(0), 1), 0);
  zassert_equal(gpio_pin_set(power_io_shield, POWER_IO_SHIELD_FAULT(2), 1), 0);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPIOA),
      0x0000);
}
