/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_FIRMWARE_LOADER_USB_DFU)

#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(firmware_loader_usb, LOG_LEVEL_INF);

USBD_DEVICE_DEFINE(ardep_usb_dfu,
                   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
                   0x25e1,
                   0x1b1e);

USBD_DESC_LANG_DEFINE(ardep_usb_dfu_lang);

USBD_DESC_MANUFACTURER_DEFINE(ardep_usb_dfu_mfr, "Mercedes-Benz AG");
USBD_DESC_PRODUCT_DEFINE(ardep_usb_dfu_product, "ARDEP USB DFU");

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "DFU FS Configuration");
USBD_DESC_CONFIG_DEFINE(hs_cfg_desc, "DFU HS Configuration");

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(sample_fs_config, 0, 100, &fs_cfg_desc);
USBD_CONFIGURATION_DEFINE(sample_hs_config, 0, 100, &hs_cfg_desc);

#if defined(CONFIG_FIRMWARE_LOADER_USB_DFU_REBOOT_AFTER_COMPLETION)
static void reset_work_handler(struct k_work *work) {
  LOG_INF("Rebooting system to apply firmware update");
  k_msleep(CONFIG_FIRMWARE_LOADER_USB_DFU_REBOOT_DELAY_MS);
  sys_reboot(SYS_REBOOT_COLD);
}

K_WORK_DEFINE(reset_work, reset_work_handler);
#endif

static int firmware_loader_usb_init();

static void msg_cb(struct usbd_context *const usbd_ctx,
                   const struct usbd_msg *const msg) {
  switch (msg->type) {
    case USBD_MSG_DFU_APP_DETACH:
      LOG_INF("DFU Detach command received, reiniting DFU USB device");
      usbd_disable(usbd_ctx);
      usbd_shutdown(usbd_ctx);

      firmware_loader_usb_init();
      break;

#if defined(CONFIG_FIRMWARE_LOADER_USB_DFU_REBOOT_AFTER_COMPLETION)
    case USBD_MSG_DFU_DOWNLOAD_COMPLETED:
      k_work_submit(&reset_work);
      break;
#endif

    default:
      break;
  }
}

static int firmware_loader_usb_init() {
  int err;

  err = usbd_add_descriptor(&ardep_usb_dfu, &ardep_usb_dfu_lang);
  if (err) {
    LOG_ERR("Failed to add language descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&ardep_usb_dfu, &ardep_usb_dfu_mfr);
  if (err) {
    LOG_ERR("Failed to add manufacturer descriptor: %d", err);
    return err;
  }

  err = usbd_add_descriptor(&ardep_usb_dfu, &ardep_usb_dfu_product);
  if (err) {
    LOG_ERR("Failed to add product descriptor: %d", err);
    return err;
  }

  if (usbd_caps_speed(&ardep_usb_dfu) == USBD_SPEED_HS) {
    err = usbd_add_configuration(&ardep_usb_dfu, USBD_SPEED_HS,
                                 &sample_hs_config);
    if (err) {
      LOG_ERR("Failed to add High-Speed configuration: %d", err);
      return err;
    }

    err = usbd_register_class(&ardep_usb_dfu, "dfu_dfu", USBD_SPEED_HS, 1);
    if (err) {
      LOG_ERR("Failed to register classes: %d", err);
      return err;
    }

    usbd_device_set_code_triple(&ardep_usb_dfu, USBD_SPEED_HS, 0, 0, 0);
  }

  err =
      usbd_add_configuration(&ardep_usb_dfu, USBD_SPEED_FS, &sample_fs_config);
  if (err) {
    LOG_ERR("Failed to add Full-Speed configuration: %d", err);
    return err;
  }

  err = usbd_register_class(&ardep_usb_dfu, "dfu_dfu", USBD_SPEED_FS, 1);
  if (err) {
    LOG_ERR("Failed to register classes: %d", err);
    return err;
  }

  usbd_device_set_code_triple(&ardep_usb_dfu, USBD_SPEED_FS, 0, 0, 0);

  err = usbd_init(&ardep_usb_dfu);
  if (err) {
    LOG_ERR("Failed to initialize usb device: %d", err);
    return err;
  }

  err = usbd_msg_register_cb(&ardep_usb_dfu, msg_cb);
  if (err) {
    LOG_ERR("Failed to register message callback: %d", err);
    return err;
  }

  err = usbd_enable(&ardep_usb_dfu);
  if (err) {
    LOG_ERR("Failed to enable usb device: %d", err);
    return err;
  }

  return 0;
}

SYS_INIT(firmware_loader_usb_init,
         APPLICATION,
         CONFIG_APPLICATION_INIT_PRIORITY);

#endif
