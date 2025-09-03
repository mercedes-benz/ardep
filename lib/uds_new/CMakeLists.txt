# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(
    data_by_identifier.c
    ecu_reset.c
    memory_by_address.c
    uds.c
)


zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)