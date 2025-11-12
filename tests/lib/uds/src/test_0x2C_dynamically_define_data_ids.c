/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/logging/log.h"
#include "zephyr/sys/slist.h"
LOG_MODULE_REGISTER(uds_test, CONFIG_UDS_LOG_LEVEL);

#include "ardep/uds.h"
#include "fixture.h"
#include "iso14229.h"
#include "zephyr/ztest_assert.h"

#include <zephyr/ztest.h>

UDSErr_t custom_check_for_0x2C_dynamically_define_data(
    const struct uds_context *const context, bool *apply_action) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    UDSRDBIArgs_t *args = context->arg;
    if ((context->registration->data_identifier.data_id == data_id_r &&
         args->dataId == data_id_r) ||
        (context->registration->data_identifier.data_id == data_id_rw &&
         args->dataId == data_id_rw)) {
      *apply_action = true;
    }
  }
  return UDS_OK;
}

UDSErr_t custom_action_for_0x2C_dynamically_define_data(
    struct uds_context *const context, bool *consume_event) {
  if (context->registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER) {
    UDSRDBIArgs_t *args = context->arg;
    if (context->registration->data_identifier.data_id == data_id_r) {
      *consume_event = true;
      uint8_t data[] = {0x11, 0x22, 0x33, 0x44};
      args->copy(context->server, data, sizeof(data));
    }
    if (context->registration->data_identifier.data_id == data_id_rw) {
      *consume_event = true;
      uint8_t data[] = {0x55, 0x66, 0x77, 0x88};
      args->copy(context->server, data, sizeof(data));
    }
  }
  return UDS_OK;
}

void assert_dynamic_data_registration_with_id(
    sys_slist_t *dynamic_registrations, uint16_t id, bool should_be_found) {
  bool registration_found = false;
  struct uds_registration_t *registration;
  SYS_SLIST_FOR_EACH_CONTAINER (dynamic_registrations, registration, node) {
    if (registration->type == UDS_REGISTRATION_TYPE__DATA_IDENTIFIER &&
        registration->data_identifier.data_id == id) {
      registration_found = true;
      break;
    }
  }
  zassert_equal(registration_found, should_be_found);
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_id) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDIArgs_t args = {
    .type = 0x01,  // define by data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById.sourceDataId = data_id_r,
    .subFuncArgs.defineById.position = 1,
    .subFuncArgs.defineById.size = 2,
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  args.subFuncArgs.defineById.sourceDataId = data_id_rw;
  args.subFuncArgs.defineById.position = 2;
  args.subFuncArgs.defineById.size = 1;

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  UDSRDBIArgs_t read_arg = {
    .dataId = 0xFEDC,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
  zassert_ok(ret);

  uint8_t expected_data[] = {0x22, 0x33, 0x77};

  assert_copy_data(expected_data, sizeof(expected_data));

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
}

ZTEST_F(lib_uds,
        test_0x2C_dynamically_define_data_ids__remove_defined_data_id) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDIArgs_t args = {
    .type = 0x01,
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById =
        {
          .sourceDataId = data_id_r,
          .position = 1,
          .size = 2,
        },
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  args.subFuncArgs.defineById.sourceDataId = data_id_rw;
  args.subFuncArgs.defineById.position = 2;
  args.subFuncArgs.defineById.size = 1;
  args.dynamicDataId = 0xBA98;

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xBA98, true);

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xBA98, true);

  remove_args.dynamicDataId = 0xBA98;
  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xBA98, false);
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__remove_all_data_id) {
  struct uds_instance_t *instance = fixture->instance;

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDIArgs_t args = {
    .type = 0x01,
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById =
        {
          .sourceDataId = data_id_r,
          .position = 1,
          .size = 2,
        },
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  args.subFuncArgs.defineById.sourceDataId = data_id_rw,
  args.subFuncArgs.defineById.position = 2,
  args.subFuncArgs.defineById.size = 1, args.dynamicDataId = 0xBA98;

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xBA98, true);

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = true,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xBA98, false);
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__data_by_memory) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t local_buffer[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                    0x0D, 0x0E, 0x0F, 0x10};

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDIArgs_t args = {
    .type = 0x02,  // define by memory address
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineByMemAddress =
        {
          .memAddr = (void *)local_buffer,
          .memSize = 2,
        },
  };

  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  args.subFuncArgs.defineByMemAddress.memAddr = (void *)(local_buffer + 4);
  args.subFuncArgs.defineByMemAddress.memSize = 1;

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  UDSRDBIArgs_t read_arg = {
    .dataId = 0xFEDC,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
  zassert_ok(ret);

  uint8_t expected_data[] = {0x01, 0x02, 0x05};

  assert_copy_data(expected_data, sizeof(expected_data));

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
}

ZTEST_F(lib_uds, test_0x2C_dynamically_define_data_ids__mixed_data) {
  struct uds_instance_t *instance = fixture->instance;

  const uint8_t local_buffer[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
                                    0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
                                    0x0D, 0x0E, 0x0F, 0x10};

  data_id_check_fn_fake.custom_fake =
      custom_check_for_0x2C_dynamically_define_data;
  data_id_action_fn_fake.custom_fake =
      custom_action_for_0x2C_dynamically_define_data;

  UDSDDDIArgs_t memory_args = {
    .type = 0x02,  // define by memory address
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineByMemAddress =
        {
          .memAddr = (void *)local_buffer,
          .memSize = 2,
        },
  };

  UDSDDDIArgs_t data_id_args = {
    .type = 0x01,  // define by data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
    .subFuncArgs.defineById =
        {
          .sourceDataId = data_id_r,
          .position = 1,
          .size = 2,
        },
  };

  // First Memeory Element
  int ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &memory_args);
  zassert_ok(ret);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  // First Data Id Element
  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &data_id_args);
  zassert_ok(ret);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  // Second Memory Element
  memory_args.subFuncArgs.defineByMemAddress.memAddr =
      (void *)(local_buffer + 4),
  memory_args.subFuncArgs.defineByMemAddress.memSize = 1,
  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &memory_args);
  zassert_ok(ret);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  // Second Data Id Element
  data_id_args.subFuncArgs.defineById.sourceDataId = data_id_rw,
  data_id_args.subFuncArgs.defineById.position = 2,
  data_id_args.subFuncArgs.defineById.size = 1,

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &data_id_args);
  zassert_ok(ret);
  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, true);

  UDSRDBIArgs_t read_arg = {
    .dataId = 0xFEDC,
    .copy = copy,
  };

  ret = receive_event(instance, UDS_EVT_ReadDataByIdent, &read_arg);
  zassert_ok(ret);

  uint8_t expected_data[] = {0x01, 0x02, 0x22, 0x33, 0x05, 0x77};

  assert_copy_data(expected_data, sizeof(expected_data));

  UDSDDDIArgs_t remove_args = {
    .type = 0x03,  // clear dynamic data id
    .allDataIds = false,
    .dynamicDataId = 0xFEDC,
  };

  ret = receive_event(instance, UDS_EVT_DynamicDefineDataId, &remove_args);
  zassert_ok(ret);

  assert_dynamic_data_registration_with_id(&instance->dynamic_registrations,
                                           0xFEDC, false);
}
