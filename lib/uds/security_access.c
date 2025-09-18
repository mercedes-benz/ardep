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
#include "security_access.h"

bool uds_filter_for_security_access_event(UDSEvent_t event) {
  return event == UDS_EVT_SecAccessRequestSeed ||
         event == UDS_EVT_SecAccessValidateKey;
}

uds_check_fn uds_get_check_for_security_access_request_seed(
    const struct uds_registration_t* const reg) {
  return reg->security_access.request_seed.check;
}

uds_action_fn uds_get_action_for_security_access_request_seed(
    const struct uds_registration_t* const reg) {
  return reg->security_access.request_seed.action;
}

uds_check_fn uds_get_check_for_security_access_validate_key(
    const struct uds_registration_t* const reg) {
  return reg->security_access.validate_key.check;
}

uds_action_fn uds_get_action_for_security_access_validate_key(
    const struct uds_registration_t* const reg) {
  return reg->security_access.validate_key.action;
}