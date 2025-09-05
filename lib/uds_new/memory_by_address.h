/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_NEW_MEMORY_BY_ADDRESS_H
#define ARDEP_LIB_UDS_NEW_MEMORY_BY_ADDRESS_H

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>

uds_new_check_fn uds_new_get_check_for_read_memory_by_addr(
    const struct uds_new_registration_t* const reg);
uds_new_action_fn uds_new_get_action_for_read_memory_by_addr(
    const struct uds_new_registration_t* const reg);

uds_new_check_fn uds_new_get_check_for_write_memory_by_addr(
    const struct uds_new_registration_t* const reg);
uds_new_action_fn uds_new_get_action_for_write_memory_by_addr(
    const struct uds_new_registration_t* const reg);

#endif  // ARDEP_LIB_UDS_NEW_MEMORY_BY_ADDRESS_H
