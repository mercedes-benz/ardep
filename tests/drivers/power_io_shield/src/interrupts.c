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

static const struct device* gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

FAKE_VOID_FUNC(gpio_demo_interrupt,
               const struct device*,
               struct gpio_callback*,
               gpio_port_pins_t);

static void after_each() { RESET_FAKE(gpio_demo_interrupt); }

ZTEST_SUITE(mcp_driver_interrupts, NULL, NULL, NULL, after_each, NULL);

ZTEST(mcp_driver_interrupts,
      test_configuring_output_as_interrput_is_not_supported) {
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_OUTPUT(0),
                                   GPIO_INT_EDGE_TO_ACTIVE),
      -ENOTSUP);
}

ZTEST(mcp_driver_interrupts, test_rising_edge_interrupt) {
  struct gpio_callback callback;
  gpio_init_callback(&callback, gpio_demo_interrupt,
                     BIT(POWER_IO_SHIELD_INPUT(0)));
  zassert_equal(gpio_add_callback(power_io_shield, &callback), 0);

  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_INPUT(0),
                                   GPIO_INT_EDGE_TO_ACTIVE),
      0);

  // Interrupt should be configured for the chip
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPINTENA),
      0x0100);  // gpinten
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_INTCONA),
      0x0000);  // intcon
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_DEFVALA),
      0x0000);  // defval

  // nothing should happen when not INTF is set
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_yield();
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 0);

  // set INTF and INTCAP to simulate rising edge interrupt

  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTFA,
                                   0x0100);  // input pin 0
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTCAPA,
                                   0x0100);  // input pin 0

  // set interrupt gpio to 1
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_yield();
  zassert_equal(gpio_demo_interrupt_fake.call_count, 1);
  zassert_equal(gpio_demo_interrupt_fake.arg0_val, power_io_shield);
  zassert_equal(gpio_demo_interrupt_fake.arg1_val, &callback);
  zassert_equal(gpio_demo_interrupt_fake.arg2_val,
                BIT(POWER_IO_SHIELD_INPUT(0)));
  k_msleep(10);  // wait a bit to check that no further ISR calls happen
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  k_msleep(10);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 1);

  // reset
  power_io_shield_emul_set_reg(power_io_shield_emul, 0x11, 0x00);
  power_io_shield_emul_set_reg(power_io_shield_emul, 0x0F, 0x00);

  zassert_equal(gpio_remove_callback(power_io_shield, &callback), 0);
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_INPUT(0),
                                   GPIO_INT_DISABLE),
      0);
}

ZTEST(mcp_driver_interrupts, test_fault_interrupt) {
  struct gpio_callback callback;
  gpio_init_callback(
      &callback, gpio_demo_interrupt,
      BIT(POWER_IO_SHIELD_FAULT(0)) | BIT(POWER_IO_SHIELD_FAULT(1)));
  zassert_equal(gpio_add_callback(power_io_shield, &callback), 0);

  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_FAULT(0),
                                   GPIO_INT_EDGE_TO_ACTIVE),
      0);
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_FAULT(1),
                                   GPIO_INT_EDGE_TO_ACTIVE),
      0);

  // Interrupt should be configured for the chip
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPINTENA),
      0x4001);  // gpinten
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_INTCONA),
      0x0000);  // intcon
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_DEFVALA),
      0x0000);  // defval

  // nothing should happen when not INTF is set
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_yield();
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 0);

  // set INTF and INTCAP to simulate rising edge interrupt
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTFA,
                                   0x4001);  // fault 0 and 1
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTCAPA,
                                   0x4001);  // fault 0 and 1

  // trigger interrupt pin rising
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_yield();
  zassert_equal(gpio_demo_interrupt_fake.call_count, 1);
  zassert_equal(gpio_demo_interrupt_fake.arg0_val, power_io_shield);
  zassert_equal(gpio_demo_interrupt_fake.arg1_val, &callback);
  zassert_equal(gpio_demo_interrupt_fake.arg2_val,
                BIT(POWER_IO_SHIELD_FAULT(0)) | BIT(POWER_IO_SHIELD_FAULT(1)));
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);

  // reset
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTFA, 0x0000);
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTCAPA, 0x0000);

  zassert_equal(gpio_remove_callback(power_io_shield, &callback), 0);
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_FAULT(0),
                                   GPIO_INT_DISABLE),
      0);
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_FAULT(1),
                                   GPIO_INT_DISABLE),
      0);
}

static int counter = 20;
static void demo_interrupt_custom_handler(const struct device* a,
                                          struct gpio_callback* b,
                                          gpio_port_pins_t c) {
  ARG_UNUSED(a);
  ARG_UNUSED(b);
  ARG_UNUSED(c);

  counter--;
  if (counter == 0) {
    power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTFA, 0x0000);
    power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTCAPA, 0x0000);
  }
}

ZTEST(mcp_driver_interrupts, test_level_interrupt) {
  gpio_demo_interrupt_fake.custom_fake = demo_interrupt_custom_handler;

  // note that we use input 1 here

  struct gpio_callback callback;
  gpio_init_callback(&callback, gpio_demo_interrupt,
                     BIT(POWER_IO_SHIELD_INPUT(1)));
  zassert_equal(gpio_add_callback(power_io_shield, &callback), 0);

  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_INPUT(1),
                                   GPIO_INT_LEVEL_HIGH),
      0);

  // Interrupt should be configured for the chip
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_GPINTENA),
      0x0200);  // gpinten
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_INTCONA),
      0x0200);  // intcon
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_DEFVALA),
      0x0000);  // defval

  // nothing should happen when not INTF is set
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_yield();
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);
  zassert_equal(gpio_demo_interrupt_fake.call_count, 0);

  // set INTF and INTCAP to simulate level high interrupt
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTFA,
                                   0x0200);  // input pin 1
  power_io_shield_emul_set_u16_reg(power_io_shield_emul, REG_INTCAPA,
                                   0x0200);  // input pin 1

  // set interrupt gpio to 1
  zassert_equal(gpio_emul_input_set(gpio0, 0, 1), 0);
  k_msleep(1);
  // driver now does its thing and calls the interrupt handler and after 20
  // times, the handler resets the intf which stops further interrupts then we
  // jump back here as the wq is then empty

  // the interrupt handler should only be called 20 times, not more or less
  zassert_true(gpio_demo_interrupt_fake.call_count == 20);
  zassert_true(counter == 0);

  // check that the registers were reset
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_INTFA),
      0x0000);
  zassert_equal(
      power_io_shield_emul_get_u16_reg(power_io_shield_emul, REG_INTCAPA),
      0x0000);
  k_msleep(1);
  zassert_equal(gpio_emul_input_set(gpio0, 0, 0), 0);

  zassert_true(gpio_demo_interrupt_fake.call_count == 20);

  zassert_equal(gpio_remove_callback(power_io_shield, &callback), 0);
  zassert_equal(
      gpio_pin_interrupt_configure(power_io_shield, POWER_IO_SHIELD_INPUT(1),
                                   GPIO_INT_DISABLE),
      0);
}
