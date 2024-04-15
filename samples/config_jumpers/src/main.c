/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define JUMPERS_NODE DT_NODELABEL(config_jumpers)

#define GPIO_SPEC(node_id) GPIO_DT_SPEC_GET(node_id, gpios),

static const struct gpio_dt_spec jumpers[] = {
    DT_FOREACH_CHILD_STATUS_OKAY(JUMPERS_NODE, GPIO_SPEC)};

static int configure_input() {
  for (int i = 0; i < ARRAY_SIZE(jumpers); i++) {
    const struct gpio_dt_spec *jumper = &jumpers[i];

    if (!gpio_is_ready_dt(jumper)) {
      printk("Error: jumper device %s is not ready\n", jumper->port->name);
      return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(jumper, GPIO_INPUT);

    if (ret != 0) {
      printk("Error %d: failed to configure %s pin %d\n", ret,
             jumper->port->name, jumper->pin);
      return -ENODEV;
    }
  }
  return 0;
}

static int read_input() {
  for (int i = 0; i < ARRAY_SIZE(jumpers); i++) {
    const struct gpio_dt_spec *jumper = &jumpers[i];
    int val = gpio_pin_get(jumper->port, jumper->pin);
    printk("Jumper %d: %d\n", i, val);
  }
  return 0;
}

int main(void) {
  int ret;

  ret = configure_input();
  if (ret != 0) {
    return ret;
  }

  printk("Successfully input pins\n");

  while (true) {
    printk("Reading input pins\n");
    read_input();
    k_sleep(K_MSEC(1000));
  }
  return 0;
}
