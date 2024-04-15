/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/



#include <inttypes.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#define INPUT_NODE DT_ALIAS(input)
#if !DT_NODE_HAS_STATUS(INPUT_NODE, okay)
#error "Unsupported board: input devicetree alias is not defined"
#endif
static const struct gpio_dt_spec input = GPIO_DT_SPEC_GET(INPUT_NODE, gpios);

#define OUTPUT_NODE DT_ALIAS(output)
#if !DT_NODE_HAS_STATUS(OUTPUT_NODE, okay)
#error "Unsupported board: output devicetree alias is not defined"
#endif
static const struct gpio_dt_spec output = GPIO_DT_SPEC_GET(OUTPUT_NODE, gpios);

static struct gpio_callback input_cb_data;
static void input_changed(const struct device *dev,
                          struct gpio_callback *cb,
                          uint32_t pins) {
  int value = gpio_pin_get(dev, input.pin);

  if (value < 0) {
    printk("Error %d: failed to read %s pin %d\n", value, input.port->name,
           input.pin);
    return;
  }

  // set output to same value as input
  int ret = gpio_pin_set(dev, output.pin, value);

  if (ret != 0) {
    printk("Error %d: failed to set %s pin %d\n", ret, output.port->name,
           output.pin);
    return;
  }
}

static int configure_output() {
  if (!gpio_is_ready_dt(&output)) {
    printk("Error: output device %s is not ready\n", output.port->name);
    return -ENODEV;
  }

  int ret = gpio_pin_configure_dt(&output, GPIO_OUTPUT_INACTIVE);

  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret, output.port->name,
           output.pin);
    return -ENODEV;
  }

  return 0;
}

static int configure_input() {
  if (!gpio_is_ready_dt(&input)) {
    printk("Error: input device %s is not ready\n", input.port->name);
    return -ENODEV;
  }

  int ret = gpio_pin_configure_dt(&input, GPIO_INPUT);

  if (ret != 0) {
    printk("Error %d: failed to configure %s pin %d\n", ret, input.port->name,
           input.pin);
    return -ENODEV;
  }

  ret = gpio_pin_interrupt_configure_dt(&input, GPIO_INT_EDGE_BOTH);

  if (ret != 0) {
    printk("Error %d: failed to configure interrupt on %s pin %d\n", ret,
           input.port->name, input.pin);
    return -ENODEV;
  }

  gpio_init_callback(&input_cb_data, input_changed, BIT(input.pin));
  ret = gpio_add_callback(input.port, &input_cb_data);

  if (ret != 0) {
    printk("Error %d: failed to add callback on %s pin %d\n", ret,
           input.port->name, input.pin);
    return -ENODEV;
  }

  return 0;
}

int main(void) {
  int ret;

  ret = configure_output();
  if (ret != 0) {
    return ret;
  }

  ret = configure_input();
  if (ret != 0) {
    return ret;
  }

  printk("Successfully configured input and output pins\n");

  // we can end the main thread here, since the callbacks will be called
  return 0;
}
