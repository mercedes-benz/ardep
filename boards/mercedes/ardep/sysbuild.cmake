if(SB_CONFIG_BOOT_LOGIC_UDS_FIRMWARE_LOADER)
  ExternalZephyrProject_Add(
    APPLICATION firmware-loader
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../firmware-loader
    APP_TYPE FIRMWARE_LOADER
  )

  set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_ENTRANCE_GPIO y)
  set_config_int(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_DETECT_DELAY 0)
  set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_BOOT_MODE y)
  set_config_bool(mcuboot CONFIG_BOOT_FIRMWARE_LOADER_NO_APPLICATION y)
  set(mcuboot_EXTRA_DTC_OVERLAY_FILE "${CMAKE_CURRENT_LIST_DIR}/mcuboot.overlay" CACHE STRING "")
endif()

if(SB_CONFIG_BOOT_LOGIC_LEGACY)
  if(SB_CONFIG_BOARD_REVISION STREQUAL "1.0.0")
    set_config_bool(mcuboot CONFIG_BOOT_USB_DFU_GPIO y)
  endif()
  set_config_bool(mcuboot CONFIG_MCUBOOT_INDICATION_LED y)
  set_config_bool(mcuboot CONFIG_BOOT_BOOTSTRAP y)
endif()
