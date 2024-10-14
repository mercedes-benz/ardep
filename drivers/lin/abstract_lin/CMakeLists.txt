# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(abstract_lin.c)

if(CONFIG_ABSTRACT_LIN_SCHEDULER)
  zephyr_library_sources(lin_scheduler.c)
endif()
