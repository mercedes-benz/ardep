/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H
#define ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>

#pragma once

UDSErr_t uds_new_handle_read_data_by_identifier(
    struct uds_new_instance_t* instance, UDSRDBIArgs_t* args);

UDSErr_t uds_new_handle_write_data_by_identifier(
    struct uds_new_instance_t* instance, UDSWDBIArgs_t* args);

int uds_new_register_runtime_data_identifier(struct uds_new_instance_t* inst,
                                             uint16_t data_id,
                                             void* addr,
                                             size_t num_of_elem,
                                             size_t len_elem,
                                             bool can_write);

#endif  // ARDEP_LIB_UDS_NEW_DATA_BY_IDENTIFIER_H