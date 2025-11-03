/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "uds.h"
#include "upload_download_file_transfer.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>

static const struct device* const flash_controller =
    DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))

enum UploadDownloadState {
  UDS_UPDOWN__IDLE,
  UDS_UPDOWN__DOWNLOAD_IN_PROGRESS,
  UDS_UPDOWN__UPLOAD_IN_PROGRESS,
};

struct upload_download_state {
  enum UploadDownloadState state;
  uintptr_t start_address;
  uintptr_t current_address;
  size_t total_size;
};

struct upload_download_state upload_download_state = {
  .state = UDS_UPDOWN__IDLE,

  .start_address = 0,
  .current_address = 0,
  .total_size = 0,
};

static UDSErr_t start_download(const struct uds_context* const context) {
  /*
   * Here we assume that the upper layer has already checked whether an
   * operation is already in progress. So if we are not in the idle state here
   * thats okay.
   */

  if (flash_controller == NULL || !device_is_ready(flash_controller)) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  UDSRequestDownloadArgs_t* args = (UDSRequestDownloadArgs_t*)context->arg;

  // do not check addr == 0, as this might be correct address for the flash
  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  // only support plain data (it is the only format defined in the standard)
  if (args->dataFormatIdentifier != 0x00) {
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address =
      (uintptr_t)(args->addr) - FLASH_BASE_ADDRESS;
  upload_download_state.current_address =
      (uintptr_t)(args->addr) - FLASH_BASE_ADDRESS;
  upload_download_state.total_size = args->size;

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
  // prepare flash by erasing necessary sectors
  int rc = flash_erase(flash_controller, upload_download_state.start_address,
                       upload_download_state.total_size);
  if (rc != 0) {
    LOG_ERR("Flash erase failed at addr 0x%08lx, size %zu, err %d",
            upload_download_state.start_address,
            upload_download_state.total_size, rc);
    return UDS_NRC_GeneralProgrammingFailure;
  }
#endif

  upload_download_state.state = UDS_UPDOWN__DOWNLOAD_IN_PROGRESS;

  return UDS_OK;
}

static UDSErr_t start_upload(const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN__IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  if (flash_controller == NULL || !device_is_ready(flash_controller)) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  UDSRequestUploadArgs_t* args = (UDSRequestUploadArgs_t*)context->arg;

  // do not check addr == 0, as this might be correct address for the flash
  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address = (uintptr_t)args->addr;
  upload_download_state.current_address = (uintptr_t)args->addr;
  upload_download_state.total_size = args->size;

  // TODO: verify that the address range is valid for reading

  upload_download_state.state = UDS_UPDOWN__UPLOAD_IN_PROGRESS;

  return UDS_OK;
}

static UDSErr_t continue_download(const struct uds_context* const context) {
  UDSTransferDataArgs_t* args = (UDSTransferDataArgs_t*)context->arg;

  if (args->len == 0 || upload_download_state.current_address + args->len >
                            upload_download_state.start_address +
                                upload_download_state.total_size) {
    return UDS_NRC_RequestOutOfRange;
  }

  int rc = flash_write(flash_controller, upload_download_state.current_address,
                       args->data, args->len);

  if (rc != 0) {
    LOG_ERR("Flash write failed at addr 0x%08lx, size %u, err %d",
            upload_download_state.current_address, args->len, rc);
    return UDS_NRC_GeneralProgrammingFailure;
  }

  upload_download_state.current_address += args->len;

  return UDS_OK;
}

static UDSErr_t continue_upload(const struct uds_context* const context) {
  if (upload_download_state.state != UDS_UPDOWN__UPLOAD_IN_PROGRESS) {
    return UDS_NRC_RequestSequenceError;
  }

  if (upload_download_state.current_address >=
      upload_download_state.start_address + upload_download_state.total_size) {
    return UDS_NRC_RequestSequenceError;
  }

  UDSTransferDataArgs_t* args = (UDSTransferDataArgs_t*)context->arg;

  size_t len_to_copy =
      MIN(args->maxRespLen, upload_download_state.start_address +
                                upload_download_state.total_size -
                                upload_download_state.current_address);

  int ret = flash_read(flash_controller, upload_download_state.current_address,
                       (void*)args->data, len_to_copy);

  if (ret != 0) {
    return UDS_NRC_GeneralProgrammingFailure;
  }

  if (args->copyResponse == NULL) {
    return UDS_ERR_MISUSE;
  }

  args->copyResponse(&context->instance->iso14229.server, args->data,
                     len_to_copy);

  upload_download_state.current_address += len_to_copy;

  return UDS_OK;
}

static UDSErr_t uds_upload_download_reset() {
  if (upload_download_state.state == UDS_UPDOWN__IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  upload_download_state.state = UDS_UPDOWN__IDLE;
  upload_download_state.start_address = 0;
  upload_download_state.current_address = 0;
  upload_download_state.total_size = 0;

  return UDS_OK;
}

static UDSErr_t transfer_exit(const struct uds_context* const context) {
  ARG_UNUSED(context);

#ifdef CONFIG_UDS_FILE_TRANSFER
  UDSErr_t ret1 = uds_upload_download_reset();
  UDSErr_t ret2 = uds_file_transfer_exit();

  // at least one must return out of sequence error, not both can be running
  assert(ret1 == UDS_NRC_RequestSequenceError ||
         ret2 == UDS_NRC_RequestSequenceError);
  return ret1 == UDS_NRC_RequestSequenceError ? ret2 : ret1;
#else
  return uds_upload_download_reset();
#endif
}

static UDSErr_t uds_action_upload_download(struct uds_context* context,
                                           bool* consume_event) {
  // there currently should only be one handler for upload/download
  *consume_event = true;

  // reset state before start request, as timeout has no event
  if (context->event != UDS_EVT_TransferData) {
    int err = transfer_exit(context);
    if (err != UDS_OK && err != UDS_NRC_RequestSequenceError) {
      return err;
    }

    if (context->event == UDS_EVT_RequestTransferExit) {
      return err;
    }
  }

  switch (context->event) {
    case UDS_EVT_RequestDownload:
      return start_download(context);
      break;

    case UDS_EVT_RequestUpload:
      return start_upload(context);
      break;

#ifdef CONFIG_UDS_FILE_TRANSFER
    case UDS_EVT_RequestFileTransfer:
      return uds_file_transfer_request(context);
#endif

    case UDS_EVT_TransferData:
#ifdef CONFIG_UDS_FILE_TRANSFER
      if (uds_file_transfer_is_active()) {
        return uds_file_transfer_continue(context);
      }
#endif

      if (upload_download_state.state == UDS_UPDOWN__DOWNLOAD_IN_PROGRESS) {
        return continue_download(context);
      }

      if (upload_download_state.state == UDS_UPDOWN__UPLOAD_IN_PROGRESS) {
        return continue_upload(context);
      }

      return UDS_NRC_RequestSequenceError;
    default:
      return UDS_ERR_MISUSE;
      break;
  }

  return UDS_OK;
}

static UDSErr_t uds_check_always_apply(const struct uds_context* const context,
                                       bool* apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static uds_check_fn get_check(const struct uds_registration_t* const reg) {
  return uds_check_always_apply;
}

static uds_action_fn get_action(const struct uds_registration_t* const reg) {
  return uds_action_upload_download;
}

// Handlers for all request event types
// - UDS_EVT_RequestUpload
// - UDS_EVT_RequestDownload
// - UDS_EVT_TransferData
// - UDS_EVT_RequestTransferExit
// - UDS_EVT_RequestFileTransfer

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_upload_download_request_download_) = {
  .event = UDS_EVT_RequestDownload,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_upload_download_request_upload_) = {
  .event = UDS_EVT_RequestUpload,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_upload_download_transfer_data_) = {
  .event = UDS_EVT_TransferData,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_upload_download_request_transfer_exit_) = {
  .event = UDS_EVT_RequestTransferExit,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

STRUCT_SECTION_ITERABLE(
    uds_event_handler_data,
    __uds_event_handler_data_upload_download_request_file_transfer_) = {
  .event = UDS_EVT_RequestFileTransfer,
  .get_check = get_check,
  .get_action = get_action,
  .default_nrc = UDS_NRC_SubFunctionNotSupported,
  .registration_type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};

// Dummy registration to get the upload/download handler registered
STRUCT_SECTION_ITERABLE(uds_registration_t,
                        __uds_registration_dummy_upload_download_) = {
  .instance = NULL,
  .type = UDS_REGISTRATION_TYPE__UPLOAD_DOWNLOAD,
};
