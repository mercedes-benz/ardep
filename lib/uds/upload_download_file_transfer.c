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

#include <zephyr/fs/fs.h>
#include <zephyr/sys/util.h>

#include <ardep/uds.h>
#include <iso14229.h>

enum FileTransferMode {
  UDS_FILE_TRANSFER__IDLE,
  UDS_FILE_TRANSFER__WRITE,
  UDS_FILE_TRANSFER__READ,
};

struct file_transfer_state {
  enum FileTransferMode mode;
  struct fs_file_t file;
  bool file_open;
  size_t expected_size;
  size_t transferred;
  uint16_t block_length;
};

struct file_transfer_state file_transfer_state = {
  .mode = UDS_FILE_TRANSFER__IDLE,
  .file_open = false,
  .expected_size = 0,
  .transferred = 0,
  .block_length = 0,
};

#define UDS_FILE_TRANSFER_MAX_PATH 256U

static uint16_t uds_file_transfer_block_length(uint16_t requested) {
  uint16_t limit = UDS_TP_MTU;

  if (requested > 0 && requested < limit) {
    return requested;
  }

  return limit;
}

static UDSErr_t fs_error_to_nrc(int err) {
  switch (-err) {
    case ENOENT:
      return UDS_NRC_RequestOutOfRange;
    case EPERM:
    case EACCES:
      return UDS_NRC_SecurityAccessDenied;
    default:
      return UDS_NRC_UploadDownloadNotAccepted;
  }
}

static void uds_file_transfer_reset(void) {
  if (file_transfer_state.file_open) {
    fs_close(&file_transfer_state.file);
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER__IDLE;
  file_transfer_state.file_open = false;
  file_transfer_state.expected_size = 0;
  file_transfer_state.transferred = 0;
  file_transfer_state.block_length = 0;
}

static UDSErr_t uds_file_transfer_begin_write(
    const char *path,
    size_t expected_size,
    UDSRequestFileTransferArgs_t *args) {
  fs_file_t_init(&file_transfer_state.file);
  int rc = fs_open(&file_transfer_state.file, path,
                   FS_O_CREATE | FS_O_TRUNC | FS_O_RDWR);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER__WRITE;
  file_transfer_state.file_open = true;
  file_transfer_state.expected_size = expected_size;
  file_transfer_state.transferred = 0U;
  file_transfer_state.block_length =
      uds_file_transfer_block_length(args->maxNumberOfBlockLength);

  args->maxNumberOfBlockLength = file_transfer_state.block_length;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_begin_read(
    const char *path, UDSRequestFileTransferArgs_t *args) {
  struct fs_dirent entry;

  fs_file_t_init(&file_transfer_state.file);
  int rc = fs_open(&file_transfer_state.file, path, FS_O_READ);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  rc = fs_stat(path, &entry);
  if (rc < 0) {
    uds_file_transfer_reset();
    return fs_error_to_nrc(rc);
  }

  if (entry.type != FS_DIR_ENTRY_FILE) {
    uds_file_transfer_reset();
    return UDS_NRC_RequestOutOfRange;
  }

  file_transfer_state.mode = UDS_FILE_TRANSFER__READ;
  file_transfer_state.file_open = true;
  file_transfer_state.expected_size = entry.size;
  file_transfer_state.transferred = 0U;
  file_transfer_state.block_length =
      uds_file_transfer_block_length(args->maxNumberOfBlockLength);

  args->maxNumberOfBlockLength = file_transfer_state.block_length;

  return UDS_OK;
}

static UDSErr_t uds_file_transfer_delete(const char *path) {
  int rc = fs_unlink(path);

  if (rc < 0) {
    return fs_error_to_nrc(rc);
  }

  return UDS_OK;
}

UDSErr_t uds_file_transfer_request(struct uds_context *context) {
  if (context == NULL || context->arg == NULL) {
    return UDS_ERR_MISUSE;
  }

  UDSRequestFileTransferArgs_t *args = context->arg;

  if (args->filePath == NULL || args->filePathLen == 0U) {
    return UDS_NRC_RequestOutOfRange;
  }

  if (args->filePathLen >= UDS_FILE_TRANSFER_MAX_PATH) {
    return UDS_NRC_RequestOutOfRange;
  }

  char path[UDS_FILE_TRANSFER_MAX_PATH];
  memcpy(path, args->filePath, args->filePathLen);
  path[args->filePathLen] = '\0';

  switch (args->modeOfOperation) {
    case UDS_MOOP_ADDFILE:
    case UDS_MOOP_REPLFILE:
      return uds_file_transfer_begin_write(path, args->fileSizeCompressed,
                                           args);
    case UDS_MOOP_DELFILE:
      args->maxNumberOfBlockLength = 0U;
      return uds_file_transfer_delete(path);
    case UDS_MOOP_RDFILE:
      return uds_file_transfer_begin_read(path, args);
    default:
      return UDS_NRC_RequestOutOfRange;
  }
}

static UDSErr_t uds_file_transfer_write(const UDSTransferDataArgs_t *args) {
  if (args == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (args->data == NULL || args->len == 0U) {
    return UDS_NRC_RequestOutOfRange;
  }

  if (file_transfer_state.expected_size > 0U &&
      file_transfer_state.transferred + args->len >
          file_transfer_state.expected_size) {
    return UDS_NRC_RequestOutOfRange;
  }

  ssize_t rc = fs_write(&file_transfer_state.file, args->data, args->len);

  if (rc < 0) {
    return fs_error_to_nrc((int)rc);
  }

  if ((size_t)rc != args->len) {
    return UDS_NRC_UploadDownloadNotAccepted;
  }

  file_transfer_state.transferred += args->len;

  return UDS_OK;
}

UDSErr_t uds_file_transfer_read(struct uds_context *context,
                                UDSTransferDataArgs_t *args) {
  if (context == NULL || args == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (args->copyResponse == NULL) {
    return UDS_ERR_MISUSE;
  }

  size_t remaining = 0U;
  if (file_transfer_state.expected_size >= file_transfer_state.transferred) {
    remaining =
        file_transfer_state.expected_size - file_transfer_state.transferred;
  }

  if (remaining == 0U) {
    return UDS_NRC_RequestSequenceError;
  }

  size_t max_len = MIN((size_t)args->maxRespLen, remaining);

  ssize_t rc = fs_read(&file_transfer_state.file, (void *)args->data, max_len);

  if (rc < 0) {
    return fs_error_to_nrc((int)rc);
  }

  if (rc == 0) {
    return UDS_NRC_RequestSequenceError;
  }

  uint8_t copy_status =
      args->copyResponse(context->server, args->data, (uint16_t)rc);
  if (copy_status != UDS_PositiveResponse) {
    return copy_status;
  }

  file_transfer_state.transferred += (size_t)rc;

  return UDS_OK;
}

UDSErr_t uds_file_transfer_continue(struct uds_context *context) {
  if (context == NULL || context->arg == NULL) {
    return UDS_ERR_MISUSE;
  }

  if (file_transfer_state.mode == UDS_FILE_TRANSFER__WRITE) {
    return uds_file_transfer_write((UDSTransferDataArgs_t *)context->arg);
  } else if (file_transfer_state.mode == UDS_FILE_TRANSFER__READ) {
    return uds_file_transfer_read(context,
                                  (UDSTransferDataArgs_t *)context->arg);
  }

  return UDS_NRC_RequestSequenceError;
}

UDSErr_t uds_file_transfer_exit(void) {
  enum FileTransferMode mode = file_transfer_state.mode;
  bool complete = true;

  if (mode == UDS_FILE_TRANSFER__IDLE) {
    return UDS_NRC_RequestSequenceError;
  }

  if (mode == UDS_FILE_TRANSFER__WRITE &&
      file_transfer_state.expected_size > 0U &&
      file_transfer_state.transferred != file_transfer_state.expected_size) {
    complete = false;
  }

  uds_file_transfer_reset();

  return complete ? UDS_OK : UDS_NRC_GeneralProgrammingFailure;
}

bool uds_file_transfer_is_active(void) {
  return file_transfer_state.mode != UDS_FILE_TRANSFER__IDLE;
}
