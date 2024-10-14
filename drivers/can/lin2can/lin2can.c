/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

// Note: this is a poc for using zephyr's isotp stack via lin. To use it this
// driver translates CAN frames from isotp to lin and vice versa. As CAN can
// send bidirectionally on a frame id and lin only in one direction the driver
// translates up to 4 can IDs into the first 2 bits of the lin frames.

// Note: we use the first 2 bits of the lin frames as these are unused by isotp
// (without extended addressing)

#define DT_DRV_COMPAT virtual_lin2can
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/abstract_lin.h>

LOG_MODULE_REGISTER(lin2can, CONFIG_LIN2CAN_LOG_LEVEL);

// 2 bits are unused in isotp, use these
#define TRANSLATABLE_ADDRESSES_LEN 4

static const uint32_t id_mapping[TRANSLATABLE_ADDRESSES_LEN] = {
  CONFIG_LIN2CAN_CAN_ID0,
  CONFIG_LIN2CAN_CAN_ID1,
  CONFIG_LIN2CAN_CAN_ID2,
  CONFIG_LIN2CAN_CAN_ID3,
};

struct lin2can_config {
  const struct device *lin_bus;
  enum lin_mode mode;
};

struct lin2can_tx_entry_t {
  uint8_t frame_id;
  can_tx_callback_t tx_callback;
  void *tx_user_data;
};

struct lin2can_data {
  // 1:1 mapped array from first 2 frame bits to can_id, see id_mapping
  struct {
    can_rx_callback_t callback;  // 0 if unused
    void *user_data;
  } incoming_callbacks[TRANSLATABLE_ADDRESSES_LEN];

  // note: we are using a queue here to make blocking possible
  struct k_msgq *outgoing_frame_queue;

  // outgoing and incoming lin IDs
  uint8_t outgoing_id;
  uint8_t incoming_id;
};

// frame struct for outgoing queue
struct lin2can_outgoing_frame_t {
  struct lin_frame frame;
  can_tx_callback_t callback;
  void *user_data;
};

/**
 * @brief Finds index in id_mapping for given can id
 *
 * @param can_id to be searched for
 * @retval < TRANSLATABLE_ADDRESSES_LEN, offset in id_mapping
 * @retval 0xff, if id was not found -> not mappable
 */
static inline uint8_t map_from_can_id(uint32_t can_id) {
  for (int i = 0; i < TRANSLATABLE_ADDRESSES_LEN; i++) {
    if (id_mapping[i] == can_id) {
      return i;
    }
  }

  return 0xff;
}

/**
 * @brief Callback called by abstract lin driver that supplies outgoing frames.
 * Reads message from queue and writes it to output parameter `frame`.
 *
 * @param frame Output parameter frame that should be written
 * @param user_data Device instance pointer
 * @return true When a frame has been written to frame, can be sent
 * @return false If no frame is available to be sent.
 */
static bool lin_outgoing_cb(struct lin_frame *frame, void *user_data) {
  const struct device *dev = (const struct device *)user_data;
  struct lin2can_data *data = dev->data;
  struct k_msgq *outgoing_frame_buffer = data->outgoing_frame_queue;

  // take frame from queue and send it
  struct lin2can_outgoing_frame_t outgoing;
  if (k_msgq_get(outgoing_frame_buffer, &outgoing, K_NO_WAIT)) {
    // nothing available
    return false;
  }

  // copy frame content
  memcpy(frame, &outgoing.frame, sizeof(*frame));

  LOG_DBG("Calling can tx callback");

  outgoing.callback(dev, 0, outgoing.user_data);

  return true;
}

/**
 * @brief Callback that is called if a lin frame arrives on the lin bus.
 * Removes the encoded can_id from the first 2 bits of the lin frame and calls
 * the callback with the cleaned frame (and mapped can id)
 *
 * @param frame
 * @param user_data
 */
static void lin_incoming_cb(const struct lin_frame *frame, void *user_data) {
  const struct device *dev = (const struct device *)user_data;
  const struct lin2can_data *data = dev->data;

  // note this is not directly the can id, see id_mapping for that
  uint8_t mapped_id = frame->data[0] >> 6;

  __ASSERT(mapped_id < TRANSLATABLE_ADDRESSES_LEN, "Unexpected LIN-ID: %x",
           mapped_id);

  LOG_DBG("Incoming can frame with can id %x", id_mapping[mapped_id]);

  // no callback for this id -> skip frame
  if (!data->incoming_callbacks[mapped_id].callback) {
    return;
  }

  struct can_frame translated = {
    .id = id_mapping[mapped_id],
    .dlc = 8,
  };
  memcpy(translated.data, frame->data, frame->len);

  // remove mapped id from frame
  translated.data[0] = frame->data[0] & 0x3f;

  LOG_DBG("Calling according can callback");

  data->incoming_callbacks[mapped_id].callback(
      dev, &translated, data->incoming_callbacks[mapped_id].user_data);
}

static int lin2can_get_caps(const struct device *dev, can_mode_t *cap) {
  *cap = CAN_MODE_ONE_SHOT;
  return 0;
}

/**
 * @brief Checks whether the driver has not yet mapped a callback to given can
 * id. If not registers callback for it
 *
 * @note Only ids from id_mapping -> CONFIG_LIN2CAN_CAN_ID{0,3} are accepted
 *
 * @param filter filter where only the can id is read from
 */
static int lin2can_add_rx_filter(const struct device *dev,
                                 can_rx_callback_t callback,
                                 void *user_data,
                                 const struct can_filter *filter) {
  struct lin2can_data *data = dev->data;

  // first option: wildcard for all ids
  if ((filter->mask & 0x7FF) == 0) {
    // first check all ids unused
    for (int i = 0; i < TRANSLATABLE_ADDRESSES_LEN; i++) {
      if (data->incoming_callbacks[i].callback != NULL) {
        return -ENOSPC;
      }
    }

    // set respective callbacks
    for (int i = 0; i < TRANSLATABLE_ADDRESSES_LEN; i++) {
      data->incoming_callbacks[i].callback = callback;
      data->incoming_callbacks[i].user_data = user_data;
    }

    // return value has to be the id of the registered filter. When we register
    // a wildcard use this special return value so that remove_rx_filter knows
    return TRANSLATABLE_ADDRESSES_LEN;
  }

  // second option: single id
  // as there is no third option: return error
  if (filter->mask != CAN_STD_ID_MASK && filter->mask != CAN_EXT_ID_MASK) {
    LOG_ERR("Only wildcard filter for all ids or single id supported");
    return -ENOTSUP;
  }

  uint8_t mapped_id = map_from_can_id(filter->id);
  if (mapped_id == 0xff) {
    LOG_ERR("ID Not Mappable");
    return -ENOTSUP;
  }

  if (data->incoming_callbacks[mapped_id].callback != NULL) {
    LOG_ERR("Callback already registered for this id");
    return -ENOSPC;
  }

  data->incoming_callbacks[mapped_id].callback = callback;
  data->incoming_callbacks[mapped_id].user_data = user_data;

  LOG_DBG("RX filter added with can id %x (translated to lin %d)", filter->id,
          mapped_id);

  return mapped_id;
}

static void lin2can_remove_rx_filter(const struct device *dev, int filter_id) {
  struct lin2can_data *data = dev->data;

  // wildcard filter
  if (filter_id == TRANSLATABLE_ADDRESSES_LEN) {
    for (int i = 0; i < TRANSLATABLE_ADDRESSES_LEN; i++) {
      data->incoming_callbacks[i].callback = 0;
    }

    return;
  }

  if (filter_id >= TRANSLATABLE_ADDRESSES_LEN) {
    return;
  }
  data->incoming_callbacks[filter_id].callback = 0;
}

/**
 * @brief Maps the can id to the id_mapping index (first 2 bits in lin frame)
 * and queues the lin frame for transmission
 *
 * @param timeout queueing timeout
 * @param callback callback called upon sending
 */
static int lin2can_send(const struct device *dev,
                        const struct can_frame *frame,
                        k_timeout_t timeout,
                        can_tx_callback_t callback,
                        void *user_data) {
  struct lin2can_data *data = dev->data;
  struct k_msgq *outgoing_frame_buffer = data->outgoing_frame_queue;

  uint8_t mapped_id = map_from_can_id(frame->id);
  if (mapped_id == 0xff) {
    LOG_ERR("Unmappable id");
    return -EINVAL;
  }

  LOG_DBG("Sending lin frame for can id %x (mapped to lin id %d)", frame->id,
          mapped_id);

  struct lin2can_outgoing_frame_t outgoing = {
    .frame =
        {
          .id = data->outgoing_id,
          .len = 8,  // overwrite length to 8
          .type = LIN_CHECKSUM_AUTO,
        },
    .callback = callback,
    .user_data = user_data,
  };

  // pad lin frame with 0xff (see lin spec)
  memset(outgoing.frame.data, 0xFF, sizeof(outgoing.frame.data));

  // copy data from can frame to lin frame
  memcpy(outgoing.frame.data, frame->data, frame->dlc);

  // add mapped id to first 2 bits of first byte
  outgoing.frame.data[0] = outgoing.frame.data[0] | (mapped_id << 6);

  LOG_DBG("scheduling in msgq");

  int err = k_msgq_put(outgoing_frame_buffer, &outgoing, timeout);
  if (err) {
    return err;
  }

  LOG_DBG("scheduled in msgq");

  return 0;
}

static int lin2can_init(const struct device *dev) {
  struct lin2can_data *data = dev->data;
  const struct lin2can_config *config = dev->config;

  // mark all callbacks as invalid
  for (int i = 0; i < TRANSLATABLE_ADDRESSES_LEN; i++) {
    data->incoming_callbacks[i].callback = NULL;
  }

  if (config->mode == LIN_MODE_COMMANDER) {
    data->incoming_id = CONFIG_LIN2CAN_SLAVE_RESPONSE_ID;
    data->outgoing_id = CONFIG_LIN2CAN_MASTER_REQUEST_ID;
  } else {
    data->incoming_id = CONFIG_LIN2CAN_MASTER_REQUEST_ID;
    data->outgoing_id = CONFIG_LIN2CAN_SLAVE_RESPONSE_ID;
  }

  int err = abstract_lin_register_incoming(config->lin_bus, lin_incoming_cb,
                                           data->incoming_id, 8, (void *)dev);
  if (err) {
    LOG_ERR("Error registering incoming lin frame callback %d", err);
    return err;
  }
  err = abstract_lin_register_outgoing(config->lin_bus, lin_outgoing_cb,
                                       data->outgoing_id, 8, (void *)dev);
  if (err) {
    LOG_ERR("Error registering outgoing lin frame callback %d", err);
    return err;
  }

  return 0;
}

static int lin2can_start(const struct device *dev) { return 0; }

static int lin2can_setmode(const struct device *dev, can_mode_t mode) {
  return 0;
}

static int lin2can_get_state(const struct device *dev,
                             enum can_state *state,
                             struct can_bus_err_cnt *err_cnt) {
  if (state != NULL) {
    *state = CAN_STATE_ERROR_ACTIVE;
  }

  return 0;
}

static struct can_driver_api lin2can_driver_api = {
  .get_capabilities = lin2can_get_caps,
  .send = lin2can_send,
  .add_rx_filter = lin2can_add_rx_filter,
  .remove_rx_filter = lin2can_remove_rx_filter,
  .start = lin2can_start,
  .set_mode = lin2can_setmode,
  .get_state = lin2can_get_state,
};

#define LIN2CAN_INIT(inst)                                               \
  K_MSGQ_DEFINE(lin2can_out_msgq_##inst,                                 \
                sizeof(struct lin2can_outgoing_frame_t),                 \
                CONFIG_LIN2CAN_OUTGOING_QUEUE_SIZE, 1);                  \
  static struct lin2can_data lin2can_data_##inst = {                     \
    .outgoing_frame_queue = &lin2can_out_msgq_##inst,                    \
  };                                                                     \
  static const struct lin2can_config lin2can_config_##inst = {           \
    .lin_bus = DEVICE_DT_GET(DT_INST_BUS(inst)),                         \
    .mode = DT_STRING_TOKEN(DT_INST_BUS(inst), type),                    \
  };                                                                     \
  DEVICE_DT_INST_DEFINE(inst, &lin2can_init, NULL, &lin2can_data_##inst, \
                        &lin2can_config_##inst, POST_KERNEL,             \
                        CONFIG_LIN2CAN_INIT_PRIORITY, &lin2can_driver_api);

DT_INST_FOREACH_STATUS_OKAY(LIN2CAN_INIT)
