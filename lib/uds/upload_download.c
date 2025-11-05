/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds, CONFIG_UDS_LOG_LEVEL);

#include "uds.h"

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>
#ifdef CONFIG_UDS_FILE_TRANSFER
#include "upload_download_file_transfer.h"
#endif

static const struct device* const flash_controller =
    DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));
/* Flash memory base address for STM32 - the actual memory, not the controller
 */
#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define FLASH_MAX_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
// on stm32 this is 8 bytes, for head room we set the limit to 32
#define MAXIMUM_FLASH_WRITE_BLOCK_SIZE 32

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
  size_t write_block_size;
};

struct upload_download_state upload_download_state = {
  .state = UDS_UPDOWN__IDLE,

  .start_address = 0,
  .current_address = 0,
  .total_size = 0,
  .write_block_size = 0,
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

  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  // normalize address to start at 0, not at the flash base address
  if ((uintptr_t)(args->addr) > FLASH_BASE_ADDRESS) {
    args->addr = (void*)((uintptr_t)args->addr - FLASH_BASE_ADDRESS);
  }

  if ((uintptr_t)(args->addr) + args->size > FLASH_MAX_SIZE) {
    LOG_WRN(
        "Address out of range: requested download from: 0x%lx, to: 0x%lx, "
        "flash "
        "is "
        "only 0x%x large",
        (uintptr_t)(args->addr), (uintptr_t)(args->addr) + args->size,
        FLASH_MAX_SIZE);
    return UDS_NRC_RequestOutOfRange;
  }

  // only support plain data (it is the only format defined in the standard)
  if (args->dataFormatIdentifier != 0x00) {
    return UDS_NRC_RequestOutOfRange;
  }

  LOG_INF("Starting download to flash addr 0x%08lx, size %zu",
          (uintptr_t)(args->addr), args->size);

  upload_download_state.start_address = (uintptr_t)(args->addr);
  upload_download_state.current_address = (uintptr_t)(args->addr);
  upload_download_state.total_size = args->size;

  upload_download_state.write_block_size =
      flash_get_write_block_size(flash_controller);

  if (upload_download_state.write_block_size > MAXIMUM_FLASH_WRITE_BLOCK_SIZE) {
    LOG_ERR("Flash write block size %zu exceeds maximum of %d",
            upload_download_state.write_block_size,
            MAXIMUM_FLASH_WRITE_BLOCK_SIZE);
    return UDS_NRC_UploadDownloadNotAccepted;
  }

#if defined(CONFIG_FLASH_HAS_EXPLICIT_ERASE)
  // prepare flash by erasing necessary sectors
  // int rc = flash_erase(flash_controller, upload_download_state.start_address,
  //                      upload_download_state.total_size);
  // if (rc != 0) {
  //   LOG_ERR("Flash erase failed at addr 0x%08lx, size %zu, err %d",
  //           upload_download_state.start_address,
  //           upload_download_state.total_size, rc);
  //   return UDS_NRC_GeneralProgrammingFailure;
  // }
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

  if (args->size == 0) {
    return UDS_NRC_RequestOutOfRange;
  }

  // normalize address to start at 0, not at the flash base address
  if ((uintptr_t)(args->addr) > FLASH_BASE_ADDRESS) {
    args->addr = (void*)((uintptr_t)args->addr - FLASH_BASE_ADDRESS);
  }

  if ((uintptr_t)(args->addr) + args->size > FLASH_MAX_SIZE) {
    LOG_WRN(
        "Address out of range: requested upload from: 0x%lx, to: 0x%lx, flash "
        "is "
        "only 0x%x large",
        (uintptr_t)(args->addr), (uintptr_t)(args->addr) + args->size,
        FLASH_MAX_SIZE);
    return UDS_NRC_RequestOutOfRange;
  }

  upload_download_state.start_address = (uintptr_t)args->addr;
  upload_download_state.current_address = (uintptr_t)args->addr;
  upload_download_state.total_size = args->size;

  upload_download_state.state = UDS_UPDOWN__UPLOAD_IN_PROGRESS;

  args->maxNumberOfBlockLength =
      MIN(CONFIG_UDS_UPLOAD_MAX_BLOCK_SIZE, args->maxNumberOfBlockLength);

  LOG_INF("Requested upload: from %p, size: %d", args->addr, args->size);

  return UDS_OK;
}

static UDSErr_t continue_download(const struct uds_context* const context) {
  UDSTransferDataArgs_t* args = (UDSTransferDataArgs_t*)context->arg;

  if (args->len == 0 || upload_download_state.current_address + args->len >
                            upload_download_state.start_address +
                                upload_download_state.total_size) {
    return UDS_NRC_RequestOutOfRange;
  }

  LOG_INF("Writing to flash at addr 0x%08lx, size %u",
          upload_download_state.current_address, args->len);

  // Flash can only be written to in blocks of write_block_size, first we
  // calculate the number of bytes that would overflow this block size.
  // Then we write all full blocks first, and if there are remaining bytes, we
  // create a temporary buffer to write the last block with padding (0xFF).
  const size_t overflow_size =
      args->len % upload_download_state.write_block_size;
  const size_t first_write = args->len - overflow_size;

  int rc;
  if (first_write > 0) {
    rc = flash_write(flash_controller, upload_download_state.current_address,
                     args->data, first_write);

    if (rc != 0) {
      LOG_ERR("Flash write failed at addr 0x%08lx, size %u, err %d",
              upload_download_state.current_address, first_write, rc);
      return UDS_NRC_GeneralProgrammingFailure;
    }
  }

  if (overflow_size != 0) {
    // need to write remaining bytes
    uint8_t last_bytes
        [MAXIMUM_FLASH_WRITE_BLOCK_SIZE];  // todo: maybe use
                                           // upload_download_state.write_block_size
                                           // as size
    memset(last_bytes, 0xFF, upload_download_state.write_block_size);
    memcpy(last_bytes, &args->data[first_write], args->len - first_write);

    rc = flash_write(flash_controller,
                     upload_download_state.current_address + first_write,
                     last_bytes, upload_download_state.write_block_size);
    if (rc != 0) {
      LOG_ERR("Flash write failed at addr 0x%08lx, size %u, err %d",
              upload_download_state.current_address + first_write,
              upload_download_state.write_block_size, rc);
      return UDS_NRC_GeneralProgrammingFailure;
    }
  }

  LOG_INF("Write finished");

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

  LOG_INF("Transfer Exit");

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

  // reset state before start requests (as timeouts are not reported to us)
  switch (context->event) {
    case UDS_EVT_RequestDownload:
    case UDS_EVT_RequestUpload:
    case UDS_EVT_RequestFileTransfer: {
      int err = transfer_exit(context);
      // only return status on non-expected errors
      if (err != UDS_OK && err != UDS_NRC_RequestSequenceError) {
        return err;
      }
    }
    default:
      break;
  }

  switch (context->event) {
    case UDS_EVT_RequestDownload:
      return start_download(context);
      break;

    case UDS_EVT_RequestUpload:
      return start_upload(context);
      break;

    case UDS_EVT_RequestFileTransfer:
#ifdef CONFIG_UDS_FILE_TRANSFER
      return uds_file_transfer_request(context);
#else
      return UDS_NRC_ServiceNotSupported;
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

    case UDS_EVT_RequestTransferExit:
      return transfer_exit(context);

    default:
      return UDS_ERR_MISUSE;
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
