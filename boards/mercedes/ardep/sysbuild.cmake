# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

# Note that the Boot logic sysbuild configs are located in ./Kconfig.sysbuild


ExternalZephyrProject_Add(
  APPLICATION firmware_loader
  SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../firmware_loader
  APP_TYPE FIRMWARE_LOADER
)

set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO y)
set_config_int(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_DETECT_DELAY 0)
set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_BOOT_MODE y)
set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_NO_APPLICATION y)


# common mcuboot configs
set(mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/mcuboot.overlay" CACHE STRING "")
