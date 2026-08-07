/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/can_fake.h>
#include <zephyr/fff.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include <ardep/can_log.h>

LOG_MODULE_REGISTER(test_can_log, LOG_LEVEL_INF);

DEFINE_FFF_GLOBALS;

static const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

static struct can_frame last_sent_frame;
static const struct device *last_send_dev;
static uint32_t send_call_count;

#ifdef CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL
uint16_t can_log_get_id(void)
{
	return 0x456;
}
#endif

static int capture_can_send_fake(const struct device *dev,
				 const struct can_frame *frame,
				 k_timeout_t timeout,
				 can_tx_callback_t callback,
				 void *user_data)
{
	ARG_UNUSED(timeout);

	last_send_dev = dev;
	last_sent_frame = *frame;
	send_call_count++;

	if (callback != NULL) {
		callback(dev, 0, user_data);
	}

	return 0;
}

static void reset_fakes(void)
{
	RESET_FAKE(fake_can_send);
	FFF_RESET_HISTORY();

	memset(&last_sent_frame, 0, sizeof(last_sent_frame));
	last_send_dev = NULL;
	send_call_count = 0;

	fake_can_send_fake.custom_fake = capture_can_send_fake;
}

static void *can_log_setup(void)
{
	zassert_true(device_is_ready(can_dev), "chosen CAN device not ready");
	return NULL;
}

static void can_log_before(void *f)
{
	ARG_UNUSED(f);
	reset_fakes();
}

ZTEST_SUITE(lib_can_log, NULL, can_log_setup, can_log_before, NULL, NULL);

ZTEST(lib_can_log, test_log_message_is_sent_on_can)
{
	LOG_INF("can-log");

	zassert_true(send_call_count >= 1, "expected at least one CAN frame");
	zassert_equal(last_send_dev, can_dev);

#ifdef CONFIG_CAN_LOG_ADDRESS_PROVIDER_EXTERNAL
	zassert_equal(last_sent_frame.id, 0x456);
#else
	zassert_equal(last_sent_frame.id, CONFIG_CAN_LOG_ID);
#endif
	zassert_true(last_sent_frame.dlc > 0);
	zassert_true(last_sent_frame.dlc <= CAN_MAX_DLEN);
}
