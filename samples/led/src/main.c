/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// Enables Logging for this module
#define LOG_MODULE_NAME app
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_APP_LOG_LEVEL);

// Gets a node Identifier from the devicetree for each color LED
// using the alias set in the devicetree
// see boards/arm/ardep/ardep.dts
#define LED_RED_NODE DT_ALIAS(red_led)
#define LED_GREEN_NODE DT_ALIAS(green_led)

// Creates a gpio_dt_spec struct for each color LED
static const struct gpio_dt_spec red_led =
    GPIO_DT_SPEC_GET(LED_RED_NODE, gpios);
static const struct gpio_dt_spec green_led =
    GPIO_DT_SPEC_GET(LED_GREEN_NODE, gpios);

#define TOGGLE_TIME_MS 1000

// Checks, whether gpio ports are ready
int init_leds() {
  if (!gpio_is_ready_dt(&red_led)) {
    LOG_ERR("Red led gpio is not ready\n");
    return -ENODEV;
  }

  if (!gpio_is_ready_dt(&green_led)) {
    LOG_ERR("Green led gpio is not ready\n");
    return -ENODEV;
  }

  return 0;
}

// Configures the gpio port for each LED to be output
int configure_leds() {
  int ret = gpio_pin_configure_dt(&red_led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Unable to configure red led\n");
    return ret;
  }

  ret = gpio_pin_configure_dt(&green_led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Unable to configure green led\n");
    return ret;
  }

  return 0;
}

// Sets the logic level for each LED port according to the state
void toggle_leds(bool red_is_on) {
  LOG_INF("Toggle LED's");
  gpio_pin_set_dt(&red_led, red_is_on);
  gpio_pin_set_dt(&green_led, !red_is_on);
}

int main(void) {
  int rc = 0;

  printk("LED sample\n");

  rc = init_leds();
  if (rc < 0) {
    return rc;
  }

  rc = configure_leds();
  if (rc < 0) {
    return rc;
  }

  bool red_is_on = true;
  while (1) {
    toggle_leds(red_is_on);

    red_is_on = !red_is_on;

    k_msleep(TOGGLE_TIME_MS);
  }
}
