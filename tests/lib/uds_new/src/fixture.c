/**
 * Copyright (c) Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ardep/uds_new.h"
#include "fixture.h"

#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/ztest.h>

#include <iso14229.h>

DEFINE_FFF_GLOBALS;

DEFINE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       data_id_check_fn,
                       const struct uds_new_context *const,
                       bool *);

DEFINE_FAKE_VALUE_FUNC(UDSErr_t,
                       data_id_action_fn,
                       struct uds_new_context *const,
                       bool *);

#define FFF_FAKES_LIST(FAKE) \
  FAKE(copy)                 \
  FAKE(data_id_check_fn)     \
  FAKE(data_id_action_fn)

struct uds_new_instance_t fixture_uds_instance;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
bool test_dynamic_registration_check_invoked;
bool test_dynamic_registration_action_invoked;
#endif  // # CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION

const uint16_t data_id_r = 1;
uint8_t data_id_r_data[4];

const uint16_t data_id_rw = 2;
uint8_t data_id_rw_data[4];

const uint16_t data_id_rw_duplicated1 = 3;
const uint16_t data_id_rw_duplicated2 = 3;
uint8_t data_id_rw_duplicated_data[4];

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(&fixture_uds_instance,
                                        data_id_r,
                                        data_id_r_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        NULL,
                                        NULL,
                                        NULL)

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(&fixture_uds_instance,
                                        data_id_rw,
                                        data_id_rw_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

// Duplicated Registratin for the same data ID
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(&fixture_uds_instance,
                                        data_id_rw_duplicated1,
                                        data_id_rw_duplicated_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(&fixture_uds_instance,
                                        data_id_rw_duplicated2,
                                        data_id_rw_duplicated_data,
                                        // read
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        // write
                                        data_id_check_fn,
                                        data_id_action_fn,
                                        NULL)

static const UDSISOTpCConfig_t cfg = {
  // Hardware Addresses
  .source_addr = 0x7E8,  // Can ID Server (us)
  .target_addr = 0x7E0,  // Can ID Client (them)

  // Functional Addresses
  .source_addr_func = 0x7DF,             // ID Server (us)
  .target_addr_func = UDS_TP_NOOP_ADDR,  // ID Client (them)
};

static uint8_t copied_data[4096];
static uint32_t copied_len;

void assert_copy_data(const uint8_t *data, uint32_t len) {
  zassert_equal(copied_len, len, "Expected length %u, but got %u", len,
                copied_len);
  zassert_mem_equal(copied_data, data, len);
}

UDSErr_t receive_event(struct uds_new_instance_t *inst,
                       UDSEvent_t event,
                       void *args) {
  return inst->iso14229.event_callback(&inst->iso14229, event, args, inst);
}

static uint8_t custom_copy(UDSServer_t *server,
                           const void *data,
                           uint16_t len) {
  copied_len = len;
  memcpy(copied_data, data, len);

  return 0;
}

static void *uds_new_setup(void) {
  memset(&fixture_uds_instance, 0, sizeof(fixture_uds_instance));

  static struct lib_uds_new_fixture fixture = {
    .cfg = cfg,
    .can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus)),
    .instance = &fixture_uds_instance,
  };

  printk("Using CAN device %s\n", fixture.can_dev->name);

  return &fixture;
}

static void uds_new_before(void *f) {
  struct lib_uds_new_fixture *fixture = f;
  const struct device *dev = fixture->can_dev;
  struct uds_new_instance_t *uds_instance = fixture->instance;

  FFF_FAKES_LIST(RESET_FAKE);
  FFF_RESET_HISTORY();

  copy_fake.custom_fake = custom_copy;

  int ret = uds_new_init(uds_instance, &cfg, dev, fixture);
  assert(ret == 0);

  STRUCT_SECTION_FOREACH (uds_new_registration_t, reg) {
  }

  test_dynamic_registration_check_invoked = false;
  test_dynamic_registration_action_invoked = false;

  memset(copied_data, 0, sizeof(copied_data));
  copied_len = 0;
}

static void uds_new_after(void *f) {
  struct lib_uds_new_fixture *fixture = f;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_REGISTRATION
  struct uds_new_registration_t *next =
      fixture->instance->dynamic_registrations;

  // Free all dynamically registered event handler
  while (next != NULL) {
    struct uds_new_registration_t *current = next;
    next = current->next;
    k_free(current);
  }

  fixture->instance->dynamic_registrations = NULL;
#endif
}

ZTEST_SUITE(
    lib_uds_new, NULL, uds_new_setup, uds_new_before, uds_new_after, NULL);