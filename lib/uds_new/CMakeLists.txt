# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

file(GLOB UDS_NEW_SOURCES *.c)
zephyr_library_sources(${UDS_NEW_SOURCES})


zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)