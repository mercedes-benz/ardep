/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "uds.h"

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/byteorder.h>

// Security-protected data identifier (only accessible at security level 1)
const uint16_t secure_data_id = 0x300;
char secure_data[] = "Secret Data Protected by Security Access";
uint16_t secure_data_size = sizeof(secure_data);

// Security access state
static uint32_t current_seed = 0;
static bool seed_requested = false;

/**
 * @brief Check function for security access request seed
 */
UDSErr_t security_access_request_seed_check(
    const struct uds_context *const context, bool *apply_action) {
  UDSSecAccessRequestSeedArgs_t *args = context->arg;

  // Only handle security level 1
  if (args->level != 1) {
    LOG_WRN("Unsupported security level: %d", args->level);
    return UDS_OK;  // Don't apply action, let other handlers try
  }

  LOG_INF("Security Access: Request seed for level %d", args->level);
  *apply_action = true;
  return UDS_OK;
}

/**
 * @brief Action function for security access request seed
 */
UDSErr_t security_access_request_seed_action(struct uds_context *const context,
                                             bool *consume_event) {
  UDSSecAccessRequestSeedArgs_t *args = context->arg;

  // Generate a random seed
  current_seed = sys_rand32_get();
  seed_requested = true;

  LOG_INF("Security Access: Generated seed 0x%08X for level %d", current_seed,
          args->level);

  // Convert seed to big-endian for transmission
  uint32_t seed_be = sys_cpu_to_be32(current_seed);

  *consume_event = true;
  return args->copySeed(&context->instance->iso14229.server,
                        (uint8_t *)&seed_be, sizeof(seed_be));
}

/**
 * @brief Check function for security access validate key
 */
UDSErr_t security_access_validate_key_check(
    const struct uds_context *const context, bool *apply_action) {
  UDSSecAccessValidateKeyArgs_t *args = context->arg;

  // Only handle security level 1
  if (args->level != 1) {
    LOG_WRN("Unsupported security level for key validation: %d", args->level);
    return UDS_OK;
  }

  LOG_INF("Security Access: Validate key for level %d", args->level);
  *apply_action = true;
  return UDS_OK;
}

/**
 * @brief Action function for security access validate key
 */
UDSErr_t security_access_validate_key_action(struct uds_context *const context,
                                             bool *consume_event) {
  UDSSecAccessValidateKeyArgs_t *args = context->arg;

  if (!seed_requested) {
    LOG_ERR("Security Access: Key validation without seed request");
    *consume_event = true;
    return UDS_NRC_RequestSequenceError;
  }

  if (args->len != sizeof(uint32_t)) {
    LOG_ERR("Security Access: Invalid key length %d (expected %zu)", args->len,
            sizeof(uint32_t));
    *consume_event = true;
    return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
  }

  // Convert received key from big-endian
  uint32_t received_key = sys_be32_to_cpu(*(uint32_t *)args->key);

  // Expected key is the bitwise negation of the seed
  uint32_t expected_key = ~current_seed;

  LOG_INF("Security Access: Received key 0x%08X, expected 0x%08X", received_key,
          expected_key);

  if (received_key == expected_key) {
    LOG_INF("Security Access: Key validation successful for level 1");
    seed_requested = false;  // Reset state
    *consume_event = true;
    return UDS_PositiveResponse;
  } else {
    LOG_WRN("Security Access: Key validation failed");
    seed_requested = false;  // Reset state
    *consume_event = true;
    return UDS_NRC_InvalidKey;
  }
}

// Register the security access handler for level 1
UDS_REGISTER_SECURITY_ACCESS_HANDLER(&instance,
                                     security_access_request_seed_check,
                                     security_access_request_seed_action,
                                     security_access_validate_key_check,
                                     security_access_validate_key_action,
                                     NULL);

/**
 * @brief Check function for reading secure data - requires security level 1
 */
UDSErr_t read_secure_data_check(const struct uds_context *const context,
                                bool *apply_action) {
  UDSRDBIArgs_t *args = context->arg;

  // Only handle our secure data identifier
  if (args->dataId != secure_data_id) {
    return UDS_OK;
  }

  // Check if security access level 1 is unlocked
  if (context->instance->iso14229.server.securityLevel < 1) {
    LOG_WRN("Secure data access denied - security level %d (required: 1)",
            context->instance->iso14229.server.securityLevel);
    *apply_action =
        true;  // We want to handle this to return security access error
    return UDS_NRC_SecurityAccessDenied;
  }

  LOG_INF("Secure data access granted - security level %d",
          context->instance->iso14229.server.securityLevel);
  *apply_action = true;
  return UDS_OK;
}

/**
 * @brief Action function for reading secure data
 */
UDSErr_t read_secure_data_action(struct uds_context *const context,
                                 bool *consume_event) {
  UDSRDBIArgs_t *args = context->arg;

  LOG_INF("Reading secure data ID: 0x%04X", args->dataId);

  // Return the secure data as-is (it's a string)
  *consume_event = true;
  return args->copy(&context->instance->iso14229.server, (uint8_t *)secure_data,
                    secure_data_size);
}

// Register the secure data identifier handler
UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&instance,
                                        secure_data_id,
                                        &secure_data,
                                        read_secure_data_check,
                                        read_secure_data_action,
                                        NULL,  // write_check - read-only
                                        NULL,  // write_action - read-only
                                        NULL,  // io_control_check
                                        NULL,  // io_control_action
                                        &secure_data_size);