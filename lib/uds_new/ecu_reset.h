/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_NEW_ECU_RESET_H
#define ARDEP_LIB_UDS_NEW_ECU_RESET_H

#include <ardep/uds_new.h>

UDSErr_t handle_ecu_reset_event(struct uds_new_instance_t* inst,
                                enum ecu_reset_type reset_type);

#endif  // ARDEP_LIB_UDS_NEW_ECU_RESET_H