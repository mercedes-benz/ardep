/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "iso14229.h"

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

// Setup data for Data by Identifier handlers
const uint16_t primitive_type_id = 0x50;
uint16_t primitive_type = 5;
uint16_t primitive_type_size = sizeof(primitive_type);

const uint16_t string_id = 0x100;
char string[] = "Hello from UDS";
// Include NULL as string terminator
uint16_t string_size = sizeof(string);

const uint16_t authenticated_type_id = 0x150;
uint8_t authenticated_type = 0x42;
uint16_t authenticated_type_size = sizeof(authenticated_type);

// Check function for the Read Data by Identifier event
// This callback is used for all data identifiers defined above. Because of
// this, we need to check for the dataID in here. Otherwise, we can assume the
// correct data ID because it is checked by the framework
UDSErr_t read_data_by_id_check(const struct uds_context *const context,
                               bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;

  // Check for authentication if required
  if (args->dataId == authenticated_type_id) {
    struct authentication_data *auth = context->instance->user_context;
    if (!auth->authenticated) {
      LOG_WRN("Access to data id 0x%02X denied - client not authenticated",
              context->registration->data_identifier.data_id);
      // If we are not authenticated, we are not allowed to read this data
      // We return a NRC which gets propagated to the client.
      // No other event handlers are checked after this.
      return UDS_NRC_ConditionsNotCorrect;
    } else {
      LOG_INF("Access to data id 0x%02X granted - client authenticated",
              context->registration->data_identifier.data_id);
    }
  }

  LOG_INF("Check to read data id: 0x%02X successful",
          context->registration->data_identifier.data_id);

  // Set to true, when we want to handle this event
  *apply_action = true;
  return UDS_OK;
}

// Action function for the Read Data by Identifier event
// This callback is used for all data identifiers defined above. Because of
// this, we need to split our actions by dataID.
UDSErr_t read_data_by_id_action(struct uds_context *const context,
                                bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

  LOG_INF("Reading data id: 0x%02X", args->dataId);

  uint8_t temp[50] = {0};

  uint16_t size =
      *(uint16_t *)context->registration->data_identifier.user_context;

  if (args->dataId == primitive_type_id) {
    // Convert to MSB-First as defined in the ISO 14229 standard
    uint16_t t = sys_cpu_to_be16(
        *(uint16_t *)context->registration->data_identifier.data);
    memcpy(temp, &t, sizeof(t));
  } else if (args->dataId == string_id ||
             args->dataId == authenticated_type_id) {
    // Transport string or raw data unchanged as raw data without conversion
    memcpy(temp, context->registration->data_identifier.data, size);
  }

  // Signal this action consumes the event
  *consume_event = true;

  // Copy the data into the response buffer
  return args->copy(context->server, temp, size);
}

// Check function for the Write Data by Identifier event
UDSErr_t write_primitive_data_by_id_check(
    const struct uds_context *const context, bool *apply_action) {
  // We don't need any additional checks here, so we can just set `apply_action`
  // and return

  LOG_INF("Check to write data id: 0x%02X successful",
          context->registration->data_identifier.data_id);
  *apply_action = true;
  return UDS_OK;
}

// Write function for the Write Data by Identifier event
UDSErr_t write_primitive_data_by_id_action(struct uds_context *const context,
                                           bool *consume_event) {
  UDSWDBIArgs_t *args = context->arg;

  // Convert the data from BE to CPU order
  uint16_t *data = context->registration->data_identifier.data;
  *data = sys_be16_to_cpu(*(uint16_t *)args->data);

  LOG_INF("Written data to id 0x%02X: 0x%04X",
          context->registration->data_identifier.data_id, *data);

  // Signal this action consumes the event
  *consume_event = true;

  return UDS_PositiveResponse;
}

// Register the data identifiers defined above
UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(
    // UDS instance the event handler is registered for
    &instance,
    // Data Identifier of the data
    primitive_type_id,
    // Pointer to the data
    &primitive_type,
    // check and action functions to read and write the data
    read_data_by_id_check,
    read_data_by_id_action,
    write_primitive_data_by_id_check,
    write_primitive_data_by_id_action,
    // IO Control check and action functions (unsused and therefore set to NULL)
    NULL,
    NULL,
    // User context pointer (used to store the size of the data here)
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

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        authenticated_type_id,
                                        &authenticated_type,
                                        read_data_by_id_check,
                                        read_data_by_id_action,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &authenticated_type_size);

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_MUTEX_DEFINE(led_data_mutex);

const uint16_t led_id = 0x200;
struct {
  // LED state as set by the WriteDataByIdentifier service
  uint8_t led_state;
  // LED state override from io_control
  uint8_t io_control_led_state;
  // Whether IO Control is active or not
  bool io_control_active;
} led_data;

UDSErr_t read_led_check(const struct uds_context *const context,
                        bool *apply_action) {
  // We don't need any additional checks here, so we can just set `apply_action`
  // and return

  LOG_INF("Check to read led state");
  *apply_action = true;
  return UDS_OK;
}

UDSErr_t read_led_action(struct uds_context *const context,
                         bool *consume_event) {
  int led_state = gpio_pin_get_dt(&led);
  if (led_state < 0) {
    LOG_ERR("Failed to read LED state");
    // Return an error if something goes wrong during the handling of the event
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
  // We don't need any additional checks here, so we can just set `apply_action`
  // and return

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
  // We set the led state here, but it will not get applied if we disabled
  // updates via IOControl
  led_data.led_state = new_led_state;
  k_mutex_unlock(&led_data_mutex);

  LOG_INF("Updated LED state: 0x%02X", new_led_state);
  *consume_event = true;

  return UDS_PositiveResponse;
}

UDSErr_t io_ctrl_led_check(const struct uds_context *const context,
                           bool *apply_action) {
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

    // Set IO Control active to notify the update thread to not apply received
    // updates anymore
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
    // Reset to the default state (0) for both, 'actual' value and IO Control
    // override
    led_data.io_control_led_state = state;
    led_data.led_state = state;
    k_mutex_unlock(&led_data_mutex);

    return args->copy(context->server, &state, sizeof(state));
  }
  if (args->ioCtrlParam == UDS_IO_CONTROL__RETURN_CONTROL_TO_ECU) {
    k_mutex_lock(&led_data_mutex, K_FOREVER);
    // Disable IO Control, so that the update thread applies the last set
    // led_state again
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

// Thread that periodically updates the LED state depending on the state and
// the override from IO Control
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
    // If IO Control is active, we want to set the override state
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
