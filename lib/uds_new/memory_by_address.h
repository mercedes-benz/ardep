#ifndef ARDEP_LIB_UDS_NEW_READ_MEMORY_BY_ADDRESS_H
#define ARDEP_LIB_UDS_NEW_READ_MEMORY_BY_ADDRESS_H

#include <ardep/iso14229.h>
#include <ardep/uds_new.h>

UDSErr_t handle_read_memory_by_address(struct uds_new_instance_t* instance,
                                       UDSReadMemByAddrArgs_t* args);

UDSErr_t handle_write_memory_by_address(struct uds_new_instance_t* instance,
                                       UDSWriteMemByAddrArgs_t* args);

#endif  // ARDEP_LIB_UDS_NEW_READ_MEMORY_BY_ADDRESS_H
