#include <ardep/uds_minimal.h>
#include <ardep/uds_new.h>

#pragma once

UDSErr_t handle_data_read_by_identifier(struct uds_new_instance_t* instance,
                                        UDSRDBIArgs_t* args);

int uds_new_register_runtime_data_identifier(struct uds_new_instance_t* inst,
                                             uint16_t data_id,
                                             void* addr,
                                             size_t len,
                                             size_t len_elem);
