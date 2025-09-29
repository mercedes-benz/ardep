#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "iso14229.h"

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include <ardep/uds.h>

#if CONFIG_UDS_USE_LINK_CONTROL

uint32_t uds_link_control_modifier_to_baudrate(
    enum uds_link_control_modifier modifier) {
  switch (modifier) {
    case UDS_LINK_CONTROL_MODIFIER__PC_9600_BAUD:
      return 9600;
    case UDS_LINK_CONTROL_MODIFIER__PC_1920_BAUD:
      return 19200;
    case UDS_LINK_CONTROL_MODIFIER__PC_38400_BAUD:
      return 38400;
    case UDS_LINK_CONTROL_MODIFIER__PC_57600_BAUD:
      return 57600;
    case UDS_LINK_CONTROL_MODIFIER__PC_115200_BAUD:
      return 115200;
    case UDS_LINK_CONTROL_MODIFIER__CAN_125000_BAUD:
      return 125000;
    case UDS_LINK_CONTROL_MODIFIER__CAN_250000_BAUD:
      return 250000;
    case UDS_LINK_CONTROL_MODIFIER__CAN_500000_BAUD:
      return 500000;
    case UDS_LINK_CONTROL_MODIFIER__CAN_1000000_BAUD:
      return 1000000;
    case UDS_LINK_CONTROL_MODIFIER__PROGRAMMING_SETUP:
    default:
      return 0;
  }
}

UDSErr_t uds_set_can_bitrate(const struct device *can_dev, uint32_t baud_rate) {
  LOG_INF("Attempting to set CAN bitrate to %u", baud_rate);
  int ret = can_stop(can_dev);
  if (ret != 0) {
    LOG_ERR("Failed to stop CAN controller");
    return UDS_NRC_ConditionsNotCorrect;
  }

  ret = can_set_bitrate(can_dev, baud_rate);
  if (ret != 0) {
    LOG_ERR("Failed to set new CAN bitrate");
    can_start(can_dev);
    return UDS_NRC_ConditionsNotCorrect;
  }

  ret = can_start(can_dev);
  if (ret != 0) {
    LOG_ERR("Failed to start CAN controller");
    return UDS_NRC_ConditionsNotCorrect;
  }

  LOG_INF("Successfully set CAN bitrate to %u", baud_rate);

  return UDS_OK;
}

UDSErr_t uds_set_can_default_bitrate(const struct device *can_dev) {
  return uds_set_can_bitrate(can_dev, CONFIG_UDS_DEFAULT_CAN_BITRATE);
}

#endif  // CONFIG_UDS_USE_LINK_CONTROL