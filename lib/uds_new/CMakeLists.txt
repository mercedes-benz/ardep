# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(
    ecu_reset.c
    uds.c
    read_data_by_identifier.c
    read_memory_by_address.c
    write_memory_by_addr_impl.c
    
)

zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)