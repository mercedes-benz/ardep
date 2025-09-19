/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

const uint16_t primitive_type_id = 0x50;
uint16_t primitive_type = 5;
uint16_t primitive_type_size = sizeof(primitive_type);

const uint16_t string_id = 0x100;
char string[] = "Hello from UDS";
// Include NULL as string terminator
uint16_t string_size = sizeof(string);

UDSErr_t read_data_by_id_check(const struct uds_context *const context,
                               bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  // Return Ok, when we don't handle this event but don't set
  // `apply_action` to true
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  if (args->dataId != primitive_type_id && args->dataId != string_id) {
    return UDS_OK;
  }
  LOG_INF("Check to read data id: 0x%02X successful",
          context->registration->data_identifier.data_id);
  // Set to true, when we want to handle this event
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_data_by_id_action(struct uds_context *const context,
                                bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

  LOG_INF("Reading data id: 0x%02X", args->dataId);

  uint8_t temp[50] = {0};

  if (args->dataId == primitive_type_id) {
    // Convert to MSB-First as defined in ISO 14229
    uint16_t t = sys_cpu_to_be16(
        *(uint16_t *)context->registration->data_identifier.data);
    memcpy(temp, &t, sizeof(t));
  } else if (args->dataId == string_id) {
    // Transport string as raw data without conversion
    uint16_t size =
        *(uint16_t *)context->registration->data_identifier.user_context;
    memcpy(temp, context->registration->data_identifier.data, size);
  }

  // Signal this action consumes the event
  *consume_event = true;

  return args->copy(
      context->server, temp,
      *(uint16_t *)context->registration->data_identifier.user_context);
}

UDSErr_t write_data_by_id_check(const struct uds_context *const context,
                                bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  // Return Ok, when we don't handle this event
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  if (args->dataId != primitive_type_id) {
    return UDS_OK;
  }
  LOG_INF("Check to write data id: 0x%02X successful",
          context->registration->data_identifier.data_id);
  // Set to true, when we want to handle this event
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t write_data_by_id_action(struct uds_context *const context,
                                 bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;

  uint16_t *data = context->registration->data_identifier.data;
  *data = sys_be16_to_cpu(*(uint16_t *)args->data);

  LOG_INF("Written data to id 0x%02X: 0x%04X",
          context->registration->data_identifier.data_id, *data);

  // Signal this action consumes the event
  *consume_event = true;

  return UDS_PositiveResponse;
}

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        primitive_type_id,
                                        &primitive_type,
                                        read_data_by_id_check,
                                        read_data_by_id_action,
                                        write_data_by_id_check,
                                        write_data_by_id_action,
                                        NULL,
                                        NULL,
                                        &primitive_type_size);

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        string_id,
                                        &string,
                                        read_data_by_id_check,
                                        read_data_by_id_action,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &string_size);

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_MUTEX_DEFINE(led_data_mutex);

const uint16_t led_id = 0x200;
struct {
  uint8_t led_state;
  uint8_t io_control_led_state;
  bool io_control_active;
} led_data;

UDSErr_t read_led_check(const struct uds_context *const context,
                        bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  if (args->dataId != led_id) {
    return UDS_OK;
  }
  LOG_INF("Check to read led state");
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_led_action(struct uds_context *const context,
                         bool *consume_event) {
  int led_state = gpio_pin_get_dt(&led);
  if (led_state < 0) {
    LOG_ERR("Failed to read LED state");
    return UDS_NRC_ConditionsNotCorrect;
  }
  LOG_INF("Reading led state: 0x%02X", led_state);

  UDSRDBIArgs_t *args = context->arg;
  uint8_t state = (uint8_t)led_state;

  *consume_event = true;
  return args->copy(context->server, &state, sizeof(state));
}

UDSErr_t write_led_check(const struct uds_context *const context,
                         bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  if (args->dataId != led_id) {
    return UDS_OK;
  }
  LOG_INF("Check to update led state");
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t write_led_action(struct uds_context *const context,
                          bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;
  if (args->len < 1) {
    LOG_ERR("Invalid data length for LED state");
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }

  uint8_t new_led_state = args->data[0];

  k_mutex_lock(&led_data_mutex, K_FOREVER);
  led_data.led_state = new_led_state;
  k_mutex_unlock(&led_data_mutex);

  LOG_INF("Updated LED state: 0x%02X", new_led_state);
  *consume_event = true;

  return UDS_PositiveResponse;
}

UDSErr_t io_ctrl_led_check(const struct uds_context *const context,
                           bool *apply_action) {
  UDSIOCtrlArgs_t *args = context->arg;
  if (args->dataId != context->registration->data_identifier.data_id) {
    return UDS_OK;
  }
  if (args->dataId != led_id) {
    return UDS_OK;
  }
  LOG_INF("Check for LED IO control");
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t io_ctrl_led_action(struct uds_context *const context,
                            bool *consume_event) {
  UDSIOCtrlArgs_t *args = context->arg;

  if (args->ioCtrlParam == UDS_IO_CONTROL__SHORT_TERM_ADJUSTMENT &&
      args->ctrlStateAndMaskLen >= 1) {
    k_mutex_lock(&led_data_mutex, K_FOREVER);
    int current_led_state = gpio_pin_get_dt(&led);
    if (current_led_state < 0) {
      k_mutex_unlock(&led_data_mutex);
      LOG_ERR("Failed to read LED state (IO control)");
      return UDS_NRC_ConditionsNotCorrect;
    }

    led_data.io_control_active = true;
    led_data.io_control_led_state = *(uint8_t *)args->ctrlStateAndMask;
    LOG_INF("Set LED state (IO control): 0x%02X",
            led_data.io_control_led_state);
    k_mutex_unlock(&led_data_mutex);

    uint8_t state = (uint8_t)current_led_state;
    return args->copy(context->server, &state, sizeof(state));
  } else if (args->ioCtrlParam == UDS_IO_CONTROL__RESET_TO_DEFAULT) {
    k_mutex_lock(&led_data_mutex, K_FOREVER);
    const uint8_t state = 0;
    led_data.io_control_led_state = state;
    led_data.led_state = state;
    k_mutex_unlock(&led_data_mutex);

    return args->copy(context->server, &state, sizeof(state));
  }
  if (args->ioCtrlParam == UDS_IO_CONTROL__RETURN_CONTROL_TO_ECU) {
    k_mutex_lock(&led_data_mutex, K_FOREVER);
    uint8_t state = led_data.led_state;
    led_data.io_control_active = false;
    k_mutex_unlock(&led_data_mutex);

    return args->copy(context->server, &state, sizeof(state));
  }

  return UDS_NRC_RequestOutOfRange;
}

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        led_id,
                                        &led_data.led_state,
                                        read_led_check,
                                        read_led_action,
                                        write_led_check,
                                        write_led_action,
                                        io_ctrl_led_check,
                                        io_ctrl_led_action,
                                        &led_data);

void led_update_thread(void *p1, void *p2, void *p3) {
  ARG_UNUSED(p1);
  ARG_UNUSED(p2);
  ARG_UNUSED(p3);

  k_mutex_lock(&led_data_mutex, K_FOREVER);
  int ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
  if (ret < 0) {
    LOG_ERR("Unable to configure led\n");
    return;
  }

  led_data.led_state = 0;
  led_data.io_control_active = false;
  led_data.io_control_led_state = 0;
  k_mutex_unlock(&led_data_mutex);

  while (true) {
    k_mutex_lock(&led_data_mutex, K_FOREVER);
    if (led_data.io_control_active) {
      gpio_pin_set_dt(&led, led_data.io_control_led_state);
    } else {
      gpio_pin_set_dt(&led, led_data.led_state);
    }
    k_mutex_unlock(&led_data_mutex);
    k_sleep(K_MSEC(100));
  }
}

K_THREAD_DEFINE(led_update_thread_id,
                512,
                led_update_thread,
                NULL,
                NULL,
                NULL,
                K_PRIO_PREEMPT(7),
                0,
                0);
