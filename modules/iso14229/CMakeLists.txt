if(CONFIG_ISO14229_LIB)
  zephyr_library()
  zephyr_library_sources(
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/server.c
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/log.c
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/tp.c
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/util.c
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/tp/isotp_c.c
    ${ZEPHYR_ISO14229_MODULE_DIR}/src/tp/isotp-c/isotp.c
    drv.c
  )
  zephyr_include_directories(include)
  zephyr_include_directories(${ZEPHYR_ISO14229_MODULE_DIR}/src)
  zephyr_compile_definitions(ISO_TP_USER_SEND_CAN_ARG UDS_CUSTOM_MILLIS=1 UDS_TP_ISOTP_C)
endif()