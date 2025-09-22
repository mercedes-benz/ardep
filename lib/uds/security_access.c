/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "iso14229.h"
#include "uds.h"

uds_check_fn uds_get_check_for_security_access_request_seed(
    const struct uds_registration_t* const reg) {
  return reg->security_access.request_seed.check;
}

uds_action_fn uds_get_action_for_security_access_request_seed(
    const struct uds_registration_t* const reg) {
  return reg->security_access.request_seed.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_sec_access_request_seed_) = {
  .event = UDS_EVT_SecAccessRequestSeed,
  .get_check = uds_get_check_for_security_access_request_seed,
  .get_action = uds_get_action_for_security_access_request_seed,
  .default_nrc = UDS_NRC_ConditionsNotCorrect,
  .registration_type = UDS_REGISTRATION_TYPE__SECURITY_ACCESS,
};

uds_check_fn uds_get_check_for_security_access_validate_key(
    const struct uds_registration_t* const reg) {
  return reg->security_access.validate_key.check;
}

uds_action_fn uds_get_action_for_security_access_validate_key(
    const struct uds_registration_t* const reg) {
  return reg->security_access.validate_key.action;
}

STRUCT_SECTION_ITERABLE(uds_event_handler_data,
                        __uds_event_handler_data_sec_access_validate_key_) = {
  .event = UDS_EVT_SecAccessValidateKey,
  .get_check = uds_get_check_for_security_access_validate_key,
  .get_action = uds_get_action_for_security_access_validate_key,
  .default_nrc = UDS_NRC_ConditionsNotCorrect,
  .registration_type = UDS_REGISTRATION_TYPE__SECURITY_ACCESS,
};