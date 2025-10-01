/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds.h"
#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/drivers/flash.h>
#include <zephyr/fs/fs.h>

#include <string.h>
#include <errno.h>

#include <zephyr/ztest.h>

#define FLASH_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))
#define STORAGE_PARTITION DT_NODELABEL(storage_partition)
#define STORAGE_PARTITION_OFFSET DT_REG_ADDR(STORAGE_PARTITION)
#define STORAGE_PARTITION_SIZE DT_REG_SIZE(STORAGE_PARTITION)

#define STORAGE_BASE_ADDRESS (FLASH_BASE_ADDRESS + STORAGE_PARTITION_OFFSET)

const struct device *const flash_controller =
  DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_flash_controller));

#define UDS_TEST_FS_MOUNT_POINT "/lfs"

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_size_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
    .size = 0,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_fail_on_format_identifier_not_0) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
    .size = 128,
    .dataFormatIdentifier = 0x01,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

static void clear_storage_partition(void) {
  int ret = flash_erase(flash_controller, STORAGE_PARTITION_OFFSET, STORAGE_PARTITION_SIZE);
  zassert_equal(ret, 0);
}

static void fill_storage_with_test_pattern(void) {
  clear_storage_partition();

  uint8_t buf[STORAGE_PARTITION_SIZE];

  for (size_t i = 0; i < sizeof(buf); i++) {
    buf[i] = (uint8_t)i; // some test pattern
  }

  int ret = flash_write(flash_controller, STORAGE_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);
}

static void assert_storage_is_erased(void) {
  uint8_t buf[STORAGE_PARTITION_SIZE];

  int ret = flash_read(flash_controller, STORAGE_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);

  uint8_t erased_pattern = 0xFF;
  for (size_t i = 0; i < sizeof(buf); i++) {
    zassert_equal(buf[i], erased_pattern);
  }
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_download_success) {
  fill_storage_with_test_pattern();

  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
  .size = STORAGE_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &args);
  zassert_equal(ret, UDS_OK);

  assert_storage_is_erased();
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_data) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t download_args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
  .size = STORAGE_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  assert_storage_is_erased();

  UDSTransferDataArgs_t transfer_args_1 = {
    .data = (const uint8_t[]){0xDE, 0xAD, 0xBE, 0xEF},
    .len = 4,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args_1);
  zassert_equal(ret, UDS_OK);

  uint8_t buf[4];
  ret = flash_read(flash_controller, STORAGE_PARTITION_OFFSET, buf, sizeof(buf));
  zassert_equal(ret, 0);
  zassert_mem_equal(buf, transfer_args_1.data, sizeof(buf));

  UDSTransferDataArgs_t transfer_args_2 = {
    .data = (const uint8_t[]){0xCA, 0xFE, 0xBA, 0xBE},
    .len = 4,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args_2);
  zassert_equal(ret, UDS_OK);

  ret = flash_read(flash_controller, STORAGE_PARTITION_OFFSET + 4, buf, sizeof(buf));
  zassert_equal(ret, 0);
  zassert_mem_equal(buf, transfer_args_2.data, sizeof(buf));
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_data_prevent_overflow) {
  struct uds_instance_t *instance = fixture->instance;

  const size_t download_size = STORAGE_PARTITION_SIZE;
  zassert_true(download_size > 0);

  UDSRequestDownloadArgs_t download_args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
    .size = download_size,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  assert_storage_is_erased();

  uint8_t guard_before;
  ret = flash_read(flash_controller,
                   STORAGE_PARTITION_OFFSET + download_size,
                   &guard_before,
                   sizeof(guard_before));
  zassert_equal(ret, 0);

  uint8_t chunk[256];
  size_t bytes_sent = 0;

  while (bytes_sent < download_size) {
    size_t remaining = download_size - bytes_sent;
    size_t chunk_len = remaining < sizeof(chunk) ? remaining : sizeof(chunk);

    for (size_t i = 0; i < chunk_len; i++) {
      chunk[i] = (uint8_t)((bytes_sent + i) & 0xFF);
    }

    UDSTransferDataArgs_t transfer_chunk = {
      .data = chunk,
      .len = chunk_len,
    };

    ret = receive_event(instance, UDS_EVT_TransferData, &transfer_chunk);
    zassert_equal(ret, UDS_OK);

    bytes_sent += chunk_len;
  }

  uint8_t last_written_byte;
  ret = flash_read(flash_controller,
                   STORAGE_PARTITION_OFFSET + download_size - 1,
                   &last_written_byte,
                   sizeof(last_written_byte));
  zassert_equal(ret, 0);
  zassert_equal(last_written_byte, (uint8_t)((download_size - 1) & 0xFF));

  const uint8_t overflow_byte = 0xAA;
  UDSTransferDataArgs_t overflow_args = {
    .data = &overflow_byte,
    .len = 1,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &overflow_args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  uint8_t guard_after;
  ret = flash_read(flash_controller,
                   STORAGE_PARTITION_OFFSET + download_size,
                   &guard_after,
                   sizeof(guard_after));
  zassert_equal(ret, 0);
  zassert_equal(guard_after, guard_before);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_upload_fail_on_size_0) {
  struct uds_instance_t *instance = fixture->instance;

  int cleanup = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_true(cleanup == UDS_OK || cleanup == UDS_NRC_RequestSequenceError);

  UDSRequestUploadArgs_t args = {
  .addr = (void*)STORAGE_PARTITION_OFFSET,
    .size = 0,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestUpload, &args);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_upload_and_transfer_data) {
  struct uds_instance_t *instance = fixture->instance;

  int cleanup = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_true(cleanup == UDS_OK || cleanup == UDS_NRC_RequestSequenceError);

  fill_storage_with_test_pattern();

  const size_t upload_size = 8;
  zassert_true(STORAGE_PARTITION_SIZE >= upload_size);

  UDSRequestUploadArgs_t upload_args = {
  .addr = (void*)STORAGE_PARTITION_OFFSET,
    .size = upload_size,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestUpload, &upload_args);
  zassert_equal(ret, UDS_OK);

  uint8_t buffer[4];
  const uint8_t expected_first_chunk[4] = {0x00, 0x01, 0x02, 0x03};
  const uint8_t expected_second_chunk[4] = {0x04, 0x05, 0x06, 0x07};

  UDSTransferDataArgs_t transfer_args = {
    .data = buffer,
    .len = sizeof(buffer),
    .maxRespLen = sizeof(buffer),
    .copyResponse = copy,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_OK);
  zassert_mem_equal(buffer, expected_first_chunk, sizeof(buffer));
  zassert_equal(copy_fake.call_count, 1);
  zassert_equal(copy_fake.arg0_val, &instance->iso14229.server);
  zassert_equal(copy_fake.arg1_val, buffer);
  zassert_equal(copy_fake.arg2_val, sizeof(buffer));
  assert_copy_data(expected_first_chunk, sizeof(expected_first_chunk));

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_OK);
  zassert_mem_equal(buffer, expected_second_chunk, sizeof(buffer));
  zassert_equal(copy_fake.call_count, 2);
  zassert_equal(copy_fake.arg2_val, sizeof(buffer));
  const uint8_t expected_combined[8] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  };
  assert_copy_data(expected_combined, sizeof(expected_combined));

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_request_upload_prevent_overflow) {
  struct uds_instance_t *instance = fixture->instance;

  int cleanup = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_true(cleanup == UDS_OK || cleanup == UDS_NRC_RequestSequenceError);

  fill_storage_with_test_pattern();

  const size_t upload_size = 4;
  zassert_true(STORAGE_PARTITION_SIZE >= upload_size);

  UDSRequestUploadArgs_t upload_args = {
  .addr = (void*)STORAGE_PARTITION_OFFSET,
    .size = upload_size,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestUpload, &upload_args);
  zassert_equal(ret, UDS_OK);

  uint8_t buffer[4];

  UDSTransferDataArgs_t transfer_args = {
    .data = buffer,
    .len = sizeof(buffer),
    .maxRespLen = sizeof(buffer),
    .copyResponse = copy,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_OK);

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_exit_success) {
  struct uds_instance_t *instance = fixture->instance;

  UDSRequestDownloadArgs_t download_args = {
  .addr = (void*)STORAGE_BASE_ADDRESS,
  .size = STORAGE_PARTITION_SIZE,
    .dataFormatIdentifier = 0x00,
  };

  int ret = receive_event(instance, UDS_EVT_RequestDownload, &download_args);
  zassert_equal(ret, UDS_OK);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);

  const uint8_t dummy_data = 0xFF;
  UDSTransferDataArgs_t transfer_args = {
    .data = &dummy_data,
    .len = 1,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer_args);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);
}

ZTEST_F(lib_uds, test_0x34_0x38_upload_download_transfer_exit_out_of_sequence) {
  struct uds_instance_t *instance = fixture->instance;

  int ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  if (ret == UDS_OK) {
    // try again to make sure we are out of sequence
    ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  }

  zassert_equal(ret, UDS_NRC_RequestSequenceError);
}

ZTEST_F(lib_uds, test_0x34_0x38_file_transfer_addfile_and_write) {
  struct uds_instance_t *instance = fixture->instance;
  const char path[] = UDS_TEST_FS_MOUNT_POINT "/transfer.bin";
  const uint8_t payload[] = {0x10, 0x20, 0x30, 0x40};

  UDSRequestFileTransferArgs_t request = {
    .modeOfOperation = UDS_MOOP_ADDFILE,
    .filePathLen = (uint16_t)strlen(path),
    .filePath = (const uint8_t *)path,
    .dataFormatIdentifier = 0x00,
    .fileSizeUnCompressed = sizeof(payload),
    .fileSizeCompressed = sizeof(payload),
    .maxNumberOfBlockLength = 0,
  };

  int ret = receive_event(instance, UDS_EVT_RequestFileTransfer, &request);
  zassert_equal(ret, UDS_OK);
  zassert_true(request.maxNumberOfBlockLength >= sizeof(payload));

  UDSTransferDataArgs_t transfer = {
    .data = payload,
    .len = sizeof(payload),
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer);
  zassert_equal(ret, UDS_OK);

  const uint8_t extra = 0xAA;
  UDSTransferDataArgs_t extra_transfer = {
    .data = &extra,
    .len = 1,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &extra_transfer);
  zassert_equal(ret, UDS_NRC_RequestOutOfRange);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);

  struct fs_file_t file;
  fs_file_t_init(&file);
  ret = fs_open(&file, path, FS_O_READ);
  zassert_equal(ret, 0);

  uint8_t buffer[sizeof(payload)] = {0};
  ssize_t read_bytes = fs_read(&file, buffer, sizeof(buffer));
  zassert_equal(read_bytes, (ssize_t)sizeof(payload));
  zassert_mem_equal(buffer, payload, sizeof(payload));
  fs_close(&file);
}

ZTEST_F(lib_uds, test_0x34_0x38_file_transfer_readfile) {
  struct uds_instance_t *instance = fixture->instance;
  const char path[] = UDS_TEST_FS_MOUNT_POINT "/read.bin";
  const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};

  struct fs_file_t file;
  fs_file_t_init(&file);
  int ret = fs_open(&file, path, FS_O_CREATE | FS_O_TRUNC | FS_O_RDWR);
  zassert_equal(ret, 0);
  ret = fs_write(&file, payload, sizeof(payload));
  zassert_equal(ret, (int)sizeof(payload));
  fs_close(&file);

  UDSRequestFileTransferArgs_t request = {
    .modeOfOperation = UDS_MOOP_RDFILE,
    .filePathLen = (uint16_t)strlen(path),
    .filePath = (const uint8_t *)path,
    .dataFormatIdentifier = 0x00,
    .fileSizeUnCompressed = 0,
    .fileSizeCompressed = 0,
    .maxNumberOfBlockLength = 0,
  };

  ret = receive_event(instance, UDS_EVT_RequestFileTransfer, &request);
  zassert_equal(ret, UDS_OK);

  uint8_t buffer[sizeof(payload)] = {0};
  UDSTransferDataArgs_t transfer = {
    .data = buffer,
    .len = sizeof(buffer),
    .maxRespLen = sizeof(buffer),
    .copyResponse = copy,
  };

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer);
  zassert_equal(ret, UDS_OK);
  zassert_mem_equal(buffer, payload, sizeof(payload));
  zassert_equal(copy_fake.call_count, 1);
  assert_copy_data(payload, sizeof(payload));

  ret = receive_event(instance, UDS_EVT_TransferData, &transfer);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_OK);
}

ZTEST_F(lib_uds, test_0x34_0x38_file_transfer_deletefile) {
  struct uds_instance_t *instance = fixture->instance;
  const char path[] = UDS_TEST_FS_MOUNT_POINT "/remove.bin";
  const uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};

  struct fs_file_t file;
  fs_file_t_init(&file);
  int ret = fs_open(&file, path, FS_O_CREATE | FS_O_TRUNC | FS_O_RDWR);
  zassert_equal(ret, 0);
  ret = fs_write(&file, payload, sizeof(payload));
  zassert_equal(ret, (int)sizeof(payload));
  fs_close(&file);

  UDSRequestFileTransferArgs_t request = {
    .modeOfOperation = UDS_MOOP_DELFILE,
    .filePathLen = (uint16_t)strlen(path),
    .filePath = (const uint8_t *)path,
    .dataFormatIdentifier = 0x00,
    .fileSizeUnCompressed = 0,
    .fileSizeCompressed = 0,
    .maxNumberOfBlockLength = 0,
  };

  ret = receive_event(instance, UDS_EVT_RequestFileTransfer, &request);
  zassert_equal(ret, UDS_OK);

  struct fs_dirent entry;
  ret = fs_stat(path, &entry);
  zassert_equal(ret, -ENOENT);

  ret = receive_event(instance, UDS_EVT_RequestTransferExit, NULL);
  zassert_equal(ret, UDS_NRC_RequestSequenceError);
}


