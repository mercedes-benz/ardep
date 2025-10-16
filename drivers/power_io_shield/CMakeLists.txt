# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_POWER_IO_SHIELD)
  zephyr_library()
  zephyr_library_sources(power_io_shield.c)
  if(CONFIG_EMUL_POWER_IO_SHIELD)
    zephyr_library_sources(power_io_shield_emul.c)
  endif()
endif()
