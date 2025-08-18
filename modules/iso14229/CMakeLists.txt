# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_MODULE_ISO14229_EXTERNAL)
  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_ISO14229_MODULE_DIR}/iso14229.c
    drv.c
  )

  zephyr_include_directories(include)
  zephyr_include_directories(${ZEPHYR_ISO14229_MODULE_DIR})
  zephyr_compile_definitions(ISO_TP_USER_SEND_CAN_ARG UDS_CUSTOM_MILLIS=1 UDS_TP_ISOTP_C)
endif()