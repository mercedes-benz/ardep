/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_DRIVERS_ABSTRACT_LIN_H_
#define ARDEP_INCLUDE_DRIVERS_ABSTRACT_LIN_H_

#include <zephyr/kernel.h>

#include <zephyrboards/drivers/lin.h>

/**
 * @brief A callback that is called whenever a frame (with set id (see
 * abstract_lin_register_incoming)) is incoming on the lin bus.
 *
 * See @a abstract_lin_register_incoming() for more info
 *
 * @note this callback may be invoked in interrupt context
 */
typedef void (*abstract_lin_incoming_callback_t)(const struct lin_frame *frame,
                                                 void *user_data);

/**
 * @brief A callback that is called whenever a frame (with set id (see
 * abstract_lin_register_outgoing)) is outgoing on the lin bus.
 *
 * See @a abstract_lin_register_outgoing() for more info
 *
 * @note this callback may be invoked in interrupt context
 *
 * @retval true if frame should be sent
 * @retval false if frame should not be sent
 */
typedef bool (*abstract_lin_outgoing_callback_t)(struct lin_frame *frame,
                                                 void *user_data);

/**
 * @brief Register a callback to a outgoing LIN frame id.
 *        See @a abstract_lin_register_outgoing() for more information.
 *
 * @note Outgoing in the sense of the commander sending a frame to the responder
 *       or the responder sending a response to the commander
 */
typedef int (*abstract_lin_register_outgoing_t)(
    const struct device *dev,
    abstract_lin_outgoing_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data);

/**
 * @brief Register a callback to a incoming LIN frame id.
 * See @a abstract_lin_register_incoming() for more information
 *
 * @note Incoming in the sense of the commander reading a frame from the
 *       responder or the responder getting data from the commander
 */
typedef int (*abstract_lin_register_incoming_t)(
    const struct device *dev,
    abstract_lin_incoming_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data);

/**
 * @brief Get the number of free callback slots left that can be registered.
 *
 * See @a abstract_lin_get_free_callback_slot() for more information
 */
typedef int (*abstract_lin_get_free_callback_slot_t)(const struct device *dev,
                                                     uint8_t *free_slots);

/**
 * @brief Unregister a incoming or outgoing callback using the frame_id.
 *
 * See @a abstract_lin_unregister() for more information
 */
typedef int (*abstract_lin_unregister_t)(const struct device *dev,
                                         uint8_t frame_id);

/**
 * @brief Commander only. Schedules a frame id to be sent over the lin bus asap
 * and calls the outgoing or incoming callback accordingly.
 *
 * See @a abstract_lin_schedule_now() for more information
 */
typedef int (*abstract_lin_schedule_now_t)(const struct device *dev,
                                           uint8_t frame_id);

__subsystem struct abstract_lin_api {
  abstract_lin_register_incoming_t register_incoming_callback;
  abstract_lin_register_outgoing_t register_outgoing_callback;
  abstract_lin_get_free_callback_slot_t get_free_callback_slots;
  abstract_lin_schedule_now_t schedule_now;
  abstract_lin_unregister_t unregister;
};

/**
 * @brief Register an outgoing frame callback. When the device is a responder it
 * will respond to frame_id headers if the callback returns true. When the
 * device is a commander it will be invoked when a schedule is requested that
 * has the same id. Outgoing callbacks have to return true to be actually sent.
 *
 * @param dev Pointer to the Abstracted LIN device
 * @param callback The callback that gets invoked
 * @param frame_id The frame_id to be intercepted
 * @param frame_size The expected size of a frame
 * @param user_data Can store any information for the callback function
 * @retval 0 On success
 * @retval -EINVAL if one of the parameters in invalid
 * @retval -ENOSPC if no more free slots are available
 * @retval -EEXIST if the frame_id already exists in the list
 */
__syscall int abstract_lin_register_outgoing(
    const struct device *dev,
    abstract_lin_outgoing_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data);

static inline int z_impl_abstract_lin_register_outgoing(
    const struct device *dev,
    abstract_lin_outgoing_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data) {
  const struct abstract_lin_api *api = dev->api;
  return api->register_outgoing_callback(dev, callback, frame_id, frame_size,
                                         user_data);
}

/**
 * @brief Register an incoming frame callback. When the device is a responder it
 * will listen to frames with set frame_id. When the device is a commander it
 * will be invoked when a schedule requests a frame and one of the responders
 * answer with data.
 *
 * @param dev Pointer to the Abstracted LIN device
 * @param callback The callback that gets invoked
 * @param frame_id The frame_id to be listened for
 * @param frame_size The expected size of a frame
 * @param user_data Can store any information for the callback function
 * @retval 0 On success
 * @retval -EINVAL if one of the parameters in invalid
 * @retval -ENOSPC if no more free slots are available
 * @retval -EEXIST if the frame_id already exists in the list
 */
__syscall int abstract_lin_register_incoming(
    const struct device *dev,
    abstract_lin_incoming_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data);

static inline int z_impl_abstract_lin_register_incoming(
    const struct device *dev,
    abstract_lin_incoming_callback_t callback,
    uint8_t frame_id,
    uint8_t frame_size,
    void *user_data) {
  const struct abstract_lin_api *api = dev->api;
  return api->register_incoming_callback(dev, callback, frame_id, frame_size,
                                         user_data);
}

/**
 * @brief Get the number of free callback slots left that can be registered by
 * the application
 *
 * @param dev Pointer to the Abstract LIN device
 * @param free_slots Output: free slot count
 * @retval 0 On success
 * @retval -EINVAL if one of the parameters is invalid
 */
__syscall int abstract_lin_get_free_callback_slot(const struct device *dev,
                                                  uint8_t *free_slots);

static inline int z_impl_abstract_lin_get_free_callback_slot(
    const struct device *dev, uint8_t *free_slots) {
  const struct abstract_lin_api *api = dev->api;
  return api->get_free_callback_slots(dev, free_slots);
}

/**
 * @brief Deregister an outgoing or incoming frame callback using the frame_id
 *
 * @param dev Pointer to the LIN device
 * @param frame_id The frame id to be deregistered
 * @retval 0 On success
 * @retval -EINVAL if one of the parameters is invalid or no callback with
 * frame_id is registered
 */
__syscall int abstract_lin_unregister(const struct device *dev,
                                      uint8_t frame_id);

static inline int z_impl_abstract_lin_unregister(const struct device *dev,
                                                 uint8_t frame_id) {
  const struct abstract_lin_api *api = dev->api;
  return api->unregister(dev, frame_id);
}

/**
 * @brief Schedules to send a frame header over the lin bus and calls the
 * outgoing or incoming callback accordingly.
 *
 * When a outgoing callback with set frame_id is registered it gets invoked to
 * first check whether to send the frame or not and to fill the frame with data.
 *
 * When a incoming callback with set frame_id is registered a frame header is
 * sent to request a respond from a responder.
 *
 * @warning Only usable by the commander.
 *
 * @note both receiving and sending are asynchronous if there is not currently
 * data being sent/received. See @a lin_send() and @a lin_receive()
 *
 * @param dev Pointer to the Abstract LIN device
 * @param frame_id The frame id to be scheduled to the bus
 * @retval 0 On success
 * @retval -EINVAL if no outgoing callback is registered or if the frame_id is
 * invalid
 * @retval -ENOTSUP if the device is not a commander
 * @retval other negative error values from lin_send / lin_receive
 */
__syscall int abstract_lin_schedule_now(const struct device *dev,
                                        uint8_t frame_id);

static inline int z_impl_abstract_lin_schedule_now(const struct device *dev,
                                                   uint8_t frame_id) {
  const struct abstract_lin_api *api = dev->api;
  return api->schedule_now(dev, frame_id);
}

#include <syscalls/abstract_lin.h>

#endif
