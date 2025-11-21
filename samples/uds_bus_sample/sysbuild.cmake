# Calculate Addresses based on base address
math(EXPR PHYS_SA "0x7E8 + ${SB_CONFIG_UDS_BASE_ADDR}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR PHYS_TA "0x7E0 + ${SB_CONFIG_UDS_BASE_ADDR}" OUTPUT_FORMAT HEXADECIMAL)
math(EXPR LOG_ID  "0x100 + ${SB_CONFIG_UDS_BASE_ADDR}" OUTPUT_FORMAT HEXADECIMAL)

# Set calculated addresses in config for each sub-application
set_config_int(uds_bus_sample  CONFIG_UDS_ADDR_PHYS_SA "${PHYS_SA}")
set_config_int(uds_bus_sample  CONFIG_UDS_ADDR_PHYS_TA "${PHYS_TA}")
set_config_int(firmware_loader CONFIG_FIRMWARE_LOADER_PHYS_SA "${PHYS_SA}")
set_config_int(firmware_loader CONFIG_FIRMWARE_LOADER_PHYS_TA "${PHYS_TA}")

set_config_int(uds_bus_sample  CONFIG_CAN_LOG_ID "${LOG_ID}")
set_config_int(firmware_loader CONFIG_CAN_LOG_ID "${LOG_ID}")
