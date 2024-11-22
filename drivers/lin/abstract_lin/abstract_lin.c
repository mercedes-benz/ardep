/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#define DT_DRV_COMPAT virtual_abstract_lin
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/abstract_lin.h>
#include <zephyrboards/drivers/lin.h>

LOG_MODULE_REGISTER(abstract_lin, CONFIG_ABSTRACT_LIN_LOG_LEVEL);

struct abstract_lin_callback_entry_t {
  uint8_t frame_id;
  uint8_t frame_size;
  enum {
    INCOMING,
    OUTGOING,
  } type;
  union {
    abstract_lin_incoming_callback_t incoming_cb;
    abstract_lin_outgoing_callback_t outgoing_cb;
  };
  void *user_data;
};

struct abstract_lin_data {
  struct abstract_lin_callback_entry_t
      callbacks[CONFIG_ABSTRACT_LIN_MAX_FRAME_COUNT];
  uint8_t used_callbacks;
};
struct abstract_lin_config {
  const struct device *lin_bus;
  enum lin_mode mode;
};

static int lin_header_callback(const struct device *lin_dev,
                               struct lin_frame *frame,
                               void *user_data) {
  ARG_UNUSED(lin_dev);

  const struct device *dev = user_data;
  struct abstract_lin_data *data = dev->data;

  // go through all registered callbacks and check if id matches. After that
  // take according action.
  for (int i = 0; i < data->used_callbacks; i++) {
    const struct abstract_lin_callback_entry_t *cb = &data->callbacks[i];

    if (cb->frame_id != frame->id) {
      continue;
    }

    frame->len = cb->frame_size;
    frame->type = LIN_CHECKSUM_AUTO;

    switch (cb->type) {
      case INCOMING:
        return LIN_ACTION_RECEIVE;

      case OUTGOING: {
        bool res = cb->outgoing_cb(frame, cb->user_data);

        // overwrite length in case the callback changed it
        frame->len = cb->frame_size;

        return res ? LIN_ACTION_SEND : LIN_ACTION_NONE;
      }
    }
  }

  return LIN_ACTION_NONE;
}

static void lin_rx_callback(const struct device *lin_dev,
                            int error,
                            const struct lin_frame *frame,
                            void *user_data) {
  ARG_UNUSED(lin_dev);

  if (error) {
    LOG_WRN("Error receiving frame");
    return;
  }

  const struct device *dev = user_data;
  struct abstract_lin_data *data = dev->data;

  // find callback with correct id and call it
  for (int i = 0; i < data->used_callbacks; i++) {
    const struct abstract_lin_callback_entry_t *cb = &data->callbacks[i];

    if (cb->frame_id != frame->id || cb->type != INCOMING) {
      continue;
    }

    __ASSERT(cb->frame_size == frame->len, "Frame sizes don't match");

    cb->incoming_cb((const struct lin_frame *)frame, cb->user_data);
  }
}

static int al_get_free_cb_slots(const struct device *dev, uint8_t *free_slots) {
  struct abstract_lin_data *data = dev->data;

  if (free_slots == NULL) {
    return -EINVAL;
  }

  *free_slots = CONFIG_ABSTRACT_LIN_MAX_FRAME_COUNT - data->used_callbacks;

  return 0;
}

/**
 * @brief Checks if there are any free callback slots and whether the frame id
 * is already in use
 *
 * @param data internal driver data
 * @param frame_id the frame id that should be registered
 * @return int positive free index in array on success, negative on error
 */
static inline int allocate_callback(struct abstract_lin_data *data,
                                    uint8_t frame_id) {
  if (data->used_callbacks >= CONFIG_ABSTRACT_LIN_MAX_FRAME_COUNT) {
    return -ENOSPC;
  }

  // check if frame_id already exists
  for (uint8_t i = 0; i < data->used_callbacks; i++) {
    if (data->callbacks[i].frame_id == frame_id) {
      return -EEXIST;
    }
  }

  // use slot and increment used counter by one after
  return data->used_callbacks++;
}

static int al_register_incoming_cb(const struct device *dev,
                                   abstract_lin_incoming_callback_t callback,
                                   uint8_t frame_id,
                                   uint8_t frame_size,
                                   void *user_data) {
  struct abstract_lin_data *data = dev->data;

  if (callback == NULL || frame_id > 0x3F || frame_size < 1 || frame_size > 8) {
    return -EINVAL;
  }

  int free_index = allocate_callback(data, frame_id);
  if (free_index < 0) {
    return free_index;
  }

  data->callbacks[free_index].frame_id = frame_id;
  data->callbacks[free_index].frame_size = frame_size;
  data->callbacks[free_index].type = INCOMING;
  data->callbacks[free_index].incoming_cb = callback;
  data->callbacks[free_index].user_data = user_data;

  return 0;
}

static int al_register_outgoing_cb(const struct device *dev,
                                   abstract_lin_outgoing_callback_t callback,
                                   uint8_t frame_id,
                                   uint8_t frame_size,
                                   void *user_data) {
  struct abstract_lin_data *data = dev->data;

  if (callback == NULL || frame_id > 0x3F || frame_size < 1 || frame_size > 8) {
    return -EINVAL;
  }

  int free_index = allocate_callback(data, frame_id);
  if (free_index < 0) {
    return free_index;
  }

  data->callbacks[free_index].frame_id = frame_id;
  data->callbacks[free_index].frame_size = frame_size;
  data->callbacks[free_index].type = OUTGOING;
  data->callbacks[free_index].outgoing_cb = callback;
  data->callbacks[free_index].user_data = user_data;

  return 0;
}

static int al_schedule_now(const struct device *dev, uint8_t frame_id) {
  const struct abstract_lin_config *config = dev->config;
  struct abstract_lin_data *data = dev->data;

  if (frame_id > 0x3F) {
    return -EINVAL;
  }

  if (config->mode != LIN_MODE_COMMANDER) {
    return -ENOTSUP;
  }

  // find callback with correct id. If callback is outgoing call it, otherwise
  // send request.
  for (int i = 0; i < data->used_callbacks; i++) {
    const struct abstract_lin_callback_entry_t *cb = &data->callbacks[i];

    if (cb->frame_id != frame_id) {
      continue;
    }

    switch (cb->type) {
      case INCOMING: {
        return lin_receive(config->lin_bus, cb->frame_id, LIN_CHECKSUM_AUTO,
                           cb->frame_size);
      }

      case OUTGOING: {
        struct lin_frame frame;

        frame.id = cb->frame_id;
        frame.len = cb->frame_size;
        frame.type = LIN_CHECKSUM_AUTO;

        bool send = cb->outgoing_cb(&frame, cb->user_data);

        // overwrite just to make sure
        frame.id = cb->frame_id;
        frame.len = cb->frame_size;

        if (send) {
          return lin_send(config->lin_bus, &frame);
        }

        // cb tells us to not send anything -> success
        return 0;
      }
    }
  }

  return -EINVAL;
}

static int al_unregister(const struct device *dev, uint8_t frame_id) {
  struct abstract_lin_data *data = dev->data;

  // find index to deregister
  int tbf_index = -1;
  for (int i = 0; i < data->used_callbacks; i++) {
    if (data->callbacks[i].frame_id == frame_id) {
      tbf_index = i;
      break;
    }
  }

  if (tbf_index == -1) {
    return -EINVAL;
  }

  // decrement callbacks counter
  data->used_callbacks--;

  // move callbacks to the left
  for (int i = tbf_index; i < data->used_callbacks; i++) {
    data->callbacks[i] = data->callbacks[i + 1];
  }

  return 0;
}

static int al_init(const struct device *dev) {
  const struct abstract_lin_config *config = dev->config;

  int err;

  if ((err = lin_set_mode(config->lin_bus, config->mode))) {
    LOG_ERR("Error setting mode");
    return err;
  };

  if ((err = lin_set_header_callback(config->lin_bus, &lin_header_callback,
                                     (void *)dev))) {
    LOG_ERR("Error setting header callback");
    return err;
  }

  if ((err = lin_set_rx_callback(config->lin_bus, &lin_rx_callback,
                                 (void *)dev))) {
    LOG_ERR("Error setting rx callback");
    return err;
  }

  return 0;
}

static const struct abstract_lin_api al_api = {
  .get_free_callback_slots = al_get_free_cb_slots,
  .register_incoming_callback = al_register_incoming_cb,
  .register_outgoing_callback = al_register_outgoing_cb,
  .schedule_now = al_schedule_now,
  .unregister = al_unregister,
};

#define ABSTRACT_LIN_INIT(n)                                          \
  static struct abstract_lin_data abstract_lin_data_##n = {           \
    .used_callbacks = 0,                                              \
  };                                                                  \
  static const struct abstract_lin_config abstract_lin_config_##n = { \
    .lin_bus = DEVICE_DT_GET(DT_INST_BUS(n)),                         \
    .mode = DT_INST_STRING_TOKEN(n, type),                            \
  };                                                                  \
  DEVICE_DT_INST_DEFINE(n, &al_init, NULL, &abstract_lin_data_##n,    \
                        &abstract_lin_config_##n, POST_KERNEL,        \
                        CONFIG_LIN_INIT_PRIORITY, &al_api);

DT_INST_FOREACH_STATUS_OKAY(ABSTRACT_LIN_INIT)
