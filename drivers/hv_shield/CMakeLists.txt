# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_HV_SHIELD)
  zephyr_library()
  add_subdirectory(hv_shield)
  add_subdirectory_ifdef(CONFIG_HV_SHIELD_GPIO gpio)
  add_subdirectory_ifdef(CONFIG_HV_SHIELD_DAC dac)
endif()
