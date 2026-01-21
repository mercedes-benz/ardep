/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>
LOG_MODULE_REGISTER(ardep_usb, CONFIG_ARDEP_USB_LOG_LEVEL);

#include <zephyr/logging/log_ctrl.h>
#include <zephyr/retention/bootmode.h>
#include <zephyr/sys/reboot.h>

USBD_DEVICE_DEFINE(ardep_usb,
                   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   0x25E1,
                   0x1B1E);

USBD_DESC_LANG_DEFINE(ardep_usb_lang);
USBD_DESC_MANUFACTURER_DEFINE(ardep_usb_mfr, "Mercedes-Benz AG");
USBD_DESC_PRODUCT_DEFINE(ardep_usb_product, CONFIG_ARDEP_USB_PRODUCT_STRING);

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "HS Configuration");

USBD_CONFIGURATION_DEFINE(ardep_usb_fs_config, 0, 250, &fs_cfg_desc);
USBD_CONFIGURATION_DEFINE(ardep_usb_hs_config, 0, 250, &hs_cfg_desc);

static void msg_cb(struct usbd_context *const usbd_ctx,
                   const struct usbd_msg *const msg) {
  switch (msg->type) {
    case USBD_MSG_DFU_APP_DETACH:
      LOG_INF("DFU Detach command received, switching to firmware loader");

      usbd_disable(usbd_ctx);
      usbd_shutdown(usbd_ctx);

      bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
      sys_reboot(SYS_REBOOT_WARM);
      break;
    default:
      break;
  }
}

static int ardep_usb_init() {
  const char *const blacklist_class_names[] = {"dfu_dfu", NULL};
  int err;

  err = usbd_add_descriptor(&ardep_usb, &ardep_usb_lang);
  if (err) {
    LOG_ERR("Failed to add language descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&ardep_usb, &ardep_usb_mfr);
  if (err) {
    LOG_ERR("Failed to add manufacturer descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&ardep_usb, &ardep_usb_product);
  if (err) {
    LOG_ERR("Failed to add product descriptor: %d", err);
    return err;
  }

  if (usbd_caps_speed(&ardep_usb) == USBD_SPEED_HS) {
    err =
        usbd_add_configuration(&ardep_usb, USBD_SPEED_HS, &ardep_usb_hs_config);
    if (err) {
      LOG_ERR("Failed to add HS configuration: %d", err);
      return err;
    }

    err = usbd_register_all_classes(&ardep_usb, USBD_SPEED_HS, 1,
                                    blacklist_class_names);
    if (err) {
      LOG_ERR("Failed to register HS classes: %d", err);
      return err;
    }
  }

  err = usbd_add_configuration(&ardep_usb, USBD_SPEED_FS, &ardep_usb_fs_config);
  if (err) {
    LOG_ERR("Failed to add FS configuration: %d", err);
    return err;
  }

  err = usbd_register_all_classes(&ardep_usb, USBD_SPEED_FS, 1,
                                  blacklist_class_names);
  if (err) {
    LOG_ERR("Failed to register FS classes: %d", err);
    return err;
  }

  err = usbd_init(&ardep_usb);
  if (err) {
    LOG_ERR("Failed to initialize usb device: %d", err);
    return err;
  }

  err = usbd_msg_register_cb(&ardep_usb, msg_cb);
  if (err) {
    LOG_ERR("Failed to register message callback: %d", err);
    return err;
  }

  err = usbd_enable(&ardep_usb);
  if (err) {
    LOG_ERR("Failed to enable usb device: %d", err);
    return err;
  }

  return 0;
}

SYS_INIT(ardep_usb_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
