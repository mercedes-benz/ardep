#ifndef APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
#define APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_

#include "ardep/uds_new.h"

#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>

#include <ardep/iso14229.h>
#include <iso14229.h>

DECLARE_FAKE_VALUE_FUNC(uint8_t, copy, UDSServer_t *, const void *, uint16_t);

struct lib_uds_new_fixture {
  UDSISOTpCConfig_t cfg;

  struct uds_new_instance_t *instance;

  const struct device *can_dev;
};

extern const uint16_t by_id_data1_default;
extern uint16_t by_id_data1;
extern const uint16_t by_id_data1_id;

extern const uint16_t by_id_data2_default[3];
extern uint16_t by_id_data2[3];
extern const uint16_t by_id_data2_id;

__attribute__((unused)) extern uint16_t by_id_data_no_rw[4];
extern const uint16_t by_id_data_no_rw_id;

extern const uint16_t by_id_data_unknown_id;

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

#endif  // APP_TESTS_LIB_ISO14229_SRC_FIXTURE_H_
