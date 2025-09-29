/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_macro.h"
#include "fixture.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       data_id_check_fn,
                       const struct uds_context *const,
                       bool *);

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       data_id_action_fn,
                       struct uds_context *const,
                       bool *);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(copy)                 \
  FAKE(data_id_check_fn)     \
  FAKE(data_id_action_fn)

struct uds_instance_t fixture_uds_instance;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
bool test_dynamic_registration_check_invoked;
bool test_dynamic_registration_action_invoked;
#endif  // # CONFIG_UDS_USE_DYNAMIC_REGISTRATION

const uint16_t data_id_r = 1;
uint8_t data_id_r_data[4];

const uint16_t data_id_rw = 2;
uint8_t data_id_rw_data[4];

const uint16_t data_id_rw_duplicated1 = 3;
const uint16_t data_id_rw_duplicated2 = 3;
uint8_t data_id_rw_duplicated_data[4];

const uint16_t routine_id = 0xDEAD;

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&fixture_uds_instance,
                                        data_id_r,
                                        data_id_r_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        NULL,
                                        NULL,
                                        // io control
                                        NULL,
                                        NULL,
                                        NULL)

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&fixture_uds_instance,
                                        data_id_rw,
                                        data_id_rw_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // io control
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

// Duplicated Registratin for the same data ID
UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&fixture_uds_instance,
                                        data_id_rw_duplicated1,
                                        data_id_rw_duplicated_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // io control
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

UDS_REGISTER_DATA_BY_IDENTIFIER_HANDLER(&fixture_uds_instance,
                                        data_id_rw_duplicated2,
                                        data_id_rw_duplicated_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // io control
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

UDS_REGISTER_ECU_RESET_HANDLER(&fixture_uds_instance,
                               ECU_RESET__KEY_OFF_ON,
                               // ecu_reset
                               data_id_check_fn,
                               data_id_action_fn,
                               // do_scheduled_reset
                               data_id_check_fn,
                               data_id_action_fn,
                               NULL)

UDS_REGISTER_MEMORY_DEFAULT_HANDLER(&fixture_uds_instance)

UDS_REGISTER_DIAG_SESSION_CTRL_HANDLER(&fixture_uds_instance,
                                       // diag_session_ctrl
                                       data_id_check_fn,
                                       data_id_action_fn,
                                       // session_timeout
                                       data_id_check_fn,
                                       data_id_action_fn,
                                       NULL)

UDS_REGISTER_READ_DTC_INFO_HANDLER_ALL(&fixture_uds_instance,
                                       data_id_check_fn,
                                       data_id_action_fn,
                                       NULL)

UDS_REGISTER_CLEAR_DIAG_INFO_HANDLER(&fixture_uds_instance,
                                     data_id_check_fn,
                                     data_id_action_fn,
                                     NULL)

UDS_REGISTER_ROUTINE_CONTROL_HANDLER(&fixture_uds_instance,
                                     routine_id,
                                     data_id_check_fn,
                                     data_id_action_fn,
                                     NULL)

UDS_REGISTER_SECURITY_ACCESS_HANDLER(&fixture_uds_instance,
                                     data_id_check_fn,
                                     data_id_action_fn,
                                     data_id_check_fn,
                                     data_id_action_fn,
                                     NULL)

UDS_REGISTER_COMMUNICATION_CONTROL_HANDLER(&fixture_uds_instance,
                                           data_id_check_fn,
                                           data_id_action_fn,
                                           NULL)

UDS_REGISTER_CONTROL_DTC_SETTING_HANDLER(&fixture_uds_instance,
                                         data_id_check_fn,
                                         data_id_action_fn,
                                         NULL)

UDS_REGISTER_LINK_CONTROL_HANDLER(&fixture_uds_instance,
                                  data_id_check_fn,
                                  data_id_action_fn,
                                  NULL)

UDS_REGISTER_DYNAMICALLY_DEFINE_DATA_IDS_DEFAULT_HANDLER(&fixture_uds_instance)

static const UDSISOTpCConfig_t default_cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

static uint8_t copied_data[4096];
static uint32_t copied_len;

void assert_copy_data_offset(const uint8_t *data,
                             uint32_t len,
                             uint32_t offset) {
  zassert_equal(copied_len, len + offset, "Expected length %u, but got %u", len,
                copied_len);
  zassert_mem_equal(copied_data + offset, data, len);
}

void assert_copy_data(const uint8_t *data, uint32_t len) {
  assert_copy_data_offset(data, len, 0);
}

UDSErr_t receive_event(struct uds_instance_t *inst,
                       UDSEvent_t event,
                       void *args) {
  return inst->iso14229.event_callback(&inst->iso14229, event, args, inst);
}

static uint8_t custom_copy(UDSServer_t *server,
                           const void *data,
                           uint16_t len) {
  memcpy(copied_data + copied_len, data, len);
  copied_len += len;

  return 0;
}

static void *uds_setup(void) {
  memset(&fixture_uds_instance, 0, sizeof(fixture_uds_instance));

  static struct lib_uds_fixture fixture = {
    .cfg = default_cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
    .instance = &fixture_uds_instance,
  };

  printk("Using CAN device %s\n", fixture.can_dev->name);

  return &fixture;
}

static void uds_before(void *f) {
  struct lib_uds_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct uds_instance_t *uds_instance = fixture->instance;

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  copy_fake.custom_fake = custom_copy;

  int ret = uds_init(uds_instance, &default_cfg, dev, fixture);
  assert(ret == 0);

  STRUCT_SECTION_FOREACH (uds_registration_t, reg) {
  }

  test_dynamic_registration_check_invoked = false;
  test_dynamic_registration_action_invoked = false;

  data_id_r_data[0] = 0x12;
  data_id_r_data[1] = 0x34;
  data_id_r_data[2] = 0x56;
  data_id_r_data[3] = 0x78;

  data_id_rw_data[0] = 0x87;
  data_id_rw_data[1] = 0x65;
  data_id_rw_data[2] = 0x43;
  data_id_rw_data[3] = 0x21;

  data_id_rw_duplicated_data[0] = 0x11;
  data_id_rw_duplicated_data[1] = 0x22;
  data_id_rw_duplicated_data[2] = 0x33;
  data_id_rw_duplicated_data[3] = 0x44;

  memset(copied_data, 0, sizeof(copied_data));
  copied_len = 0;
}

static void uds_after(void *f) {
  struct lib_uds_fixture *fixture = f;

#ifdef CONFIG_UDS_USE_DYNAMIC_REGISTRATION
  struct uds_registration_t *reg;
  struct uds_registration_t *temp;

  SYS_SLIST_FOR_EACH_CONTAINER_SAFE (&fixture->instance->dynamic_registrations,
                                     reg, temp, node) {
    k_free(reg);
  }

  sys_slist_init(&fixture->instance->dynamic_registrations);
#endif
}

ZTEST_SUITE(lib_uds, NULL, uds_setup, uds_before, uds_after, NULL);