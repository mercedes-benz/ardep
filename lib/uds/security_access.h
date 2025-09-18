/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_SECURITY_ACCESS_H
#define ARDEP_LIB_UDS_SECURITY_ACCESS_H

#include <ardep/iso14229.h>
#include <ardep/uds.h>

bool uds_filter_for_security_access_event(UDSEvent_t event);

uds_check_fn uds_get_check_for_security_access_request_seed(
    const struct uds_registration_t* const reg);

uds_action_fn uds_get_action_for_security_access_request_seed(
    const struct uds_registration_t* const reg);

uds_check_fn uds_get_check_for_security_access_validate_key(
    const struct uds_registration_t* const reg);

uds_action_fn uds_get_action_for_security_access_validate_key(
    const struct uds_registration_t* const reg);

#endif  // ARDEP_LIB_UDS_SECURITY_ACCESS_H