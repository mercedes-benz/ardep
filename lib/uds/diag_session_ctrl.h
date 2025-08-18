/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_DIAG_SESSION_CTRL_H
#define ARDEP_LIB_UDS_DIAG_SESSION_CTRL_H

#include <ardep/iso14229.h>
#include <ardep/uds.h>

uds_check_fn uds_get_check_for_diag_session_ctrl(
    const struct uds_registration_t* const reg);
uds_action_fn uds_get_action_for_diag_session_ctrl(
    const struct uds_registration_t* const reg);

uds_check_fn uds_get_check_for_session_timeout(
    const struct uds_registration_t* const reg);
uds_action_fn uds_get_action_for_session_timeout(
    const struct uds_registration_t* const reg);

#endif  // ARDEP_LIB_UDS_DIAG_SESSION_CTRL_H