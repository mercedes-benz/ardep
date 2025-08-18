/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_ECU_RESET_H
#define ARDEP_LIB_UDS_ECU_RESET_H

#include <ardep/uds.h>

uds_check_fn uds_get_check_for_ecu_reset(
    const struct uds_registration_t* const reg);
uds_action_fn uds_get_action_for_ecu_reset(
    const struct uds_registration_t* const reg);

uds_check_fn uds_get_check_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg);
uds_action_fn uds_get_action_for_execute_scheduled_reset(
    const struct uds_registration_t* const reg);

#endif  // ARDEP_LIB_UDS_ECU_RESET_H