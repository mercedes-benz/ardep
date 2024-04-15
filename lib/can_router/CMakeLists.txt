# Copyright (c) 2024 Frickly Systems GmbH
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(can_router.c)

zephyr_linker_sources(SECTIONS iterables.ld)
