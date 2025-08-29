#ifndef APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_
#define APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_

#include "ardep/uds_new.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/uds_minimal.h>
#include <server.h>
#include <tp.h>
#include <tp/isotp_c.h>

DECLARE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

struct lib_uds_new_fixture {
  UDSISOTpCConfig_t cfg;

  struct uds_new_instance_t* instance;

  const struct device *can_dev;
};

static const uint16_t by_id_data1_default = 5;
static uint16_t by_id_data1;
static const uint16_t by_id_data1_id = 0x1234;

static const uint16_t by_id_data2_default[3] = {0x1234, 0x5678, 0x9ABC};
static uint16_t by_id_data2[3];
static const uint16_t by_id_data2_id = 0x2468;

/**
 * @brief Receive an event from iso14229
 */
UDSErr_t receive_event(struct uds_new_instance_t *inst,
                       UDSEvent_t event,
                       void *args);

/**
 * @brief Assert that the copied data matches the expected data.
 *
 * Beware that the data is in big endian!
 */
void assert_copy_data(const uint8_t *data, uint32_t len);

#endif  // APP_TESTS_LIB_UDS_MINIMAL_SRC_FIXTURE_H_