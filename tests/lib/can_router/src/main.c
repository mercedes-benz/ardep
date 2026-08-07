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
#include <zephyr/ztest.h>

#include <ardep/can_router.h>

DEFINE_FFF_GLOBALS;

static const struct device *can_a = DEVICE_DT_GET(DT_NODELABEL(can_a));
static const struct device *can_b = DEVICE_DT_GET(DT_NODELABEL(can_b));

static can_rx_callback_t captured_rx_cb;
static void *captured_rx_user_data;
static const struct can_filter *captured_filter;
static const struct device *captured_filter_dev;

static struct can_frame last_sent_frame;
static const struct device *last_send_dev;
static uint32_t send_call_count;

static int capture_rx_filter_fake(const struct device *dev,
				  can_rx_callback_t callback,
				  void *user_data,
				  const struct can_filter *filter)
{
	captured_filter_dev = dev;
	captured_rx_cb = callback;
	captured_rx_user_data = user_data;
	captured_filter = filter;
	return 0;
}

static int fail_rx_filter_fake(const struct device *dev,
			       can_rx_callback_t callback,
			       void *user_data,
			       const struct can_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);
	ARG_UNUSED(filter);
	return -ENOSPC;
}

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
	RESET_FAKE(fake_can_add_rx_filter);
	FFF_RESET_HISTORY();

	captured_rx_cb = NULL;
	captured_rx_user_data = NULL;
	captured_filter = NULL;
	captured_filter_dev = NULL;
	memset(&last_sent_frame, 0, sizeof(last_sent_frame));
	last_send_dev = NULL;
	send_call_count = 0;
}

static void *can_router_setup(void)
{
	zassert_true(device_is_ready(can_a), "can_a not ready");
	zassert_true(device_is_ready(can_b), "can_b not ready");
	return NULL;
}

static void can_router_before(void *f)
{
	ARG_UNUSED(f);
	reset_fakes();
}

ZTEST_SUITE(lib_can_router, NULL, can_router_setup, can_router_before, NULL, NULL);

ZTEST(lib_can_router, test_register_installs_rx_filter)
{
	static const struct can_router_entry_t entries[] = {
		{
			.from = &can_a,
			.to = &can_b,
			.filter =
				{
					.flags = 0,
					.id = 0x123,
					.mask = 0x7FF,
				},
		},
	};

	fake_can_add_rx_filter_fake.custom_fake = capture_rx_filter_fake;

	zassert_equal(can_router_register(entries, ARRAY_SIZE(entries)), 0);
	zassert_equal(fake_can_add_rx_filter_fake.call_count, 1);
	zassert_equal(captured_filter_dev, can_a);
	zassert_not_null(captured_rx_cb);
	zassert_equal(captured_rx_user_data, can_b);
	zassert_not_null(captured_filter);
	zassert_equal(captured_filter->id, 0x123);
	zassert_equal(captured_filter->mask, 0x7FF);
}

ZTEST(lib_can_router, test_register_propagates_filter_error)
{
	static const struct can_router_entry_t entries[] = {
		{
			.from = &can_a,
			.to = &can_b,
			.filter =
				{
					.flags = 0,
					.id = 0,
					.mask = 0,
				},
		},
	};

	fake_can_add_rx_filter_fake.custom_fake = fail_rx_filter_fake;

	zassert_equal(can_router_register(entries, ARRAY_SIZE(entries)), -ENOSPC);
	zassert_equal(fake_can_add_rx_filter_fake.call_count, 1);
}

ZTEST(lib_can_router, test_frame_is_forwarded_to_destination)
{
	static const struct can_router_entry_t entries[] = {
		{
			.from = &can_a,
			.to = &can_b,
			.filter =
				{
					.flags = 0,
					.id = 0,
					.mask = 0,
				},
		},
	};
	struct can_frame rx_frame = {
		.id = 0x456,
		.dlc = 3,
		.flags = 0,
		.data = {0x11, 0x22, 0x33},
	};

	fake_can_add_rx_filter_fake.custom_fake = capture_rx_filter_fake;
	fake_can_send_fake.custom_fake = capture_can_send_fake;

	zassert_equal(can_router_register(entries, ARRAY_SIZE(entries)), 0);
	zassert_not_null(captured_rx_cb);

	captured_rx_cb(can_a, &rx_frame, captured_rx_user_data);

	zassert_equal(send_call_count, 1);
	zassert_equal(last_send_dev, can_b);
	zassert_equal(last_sent_frame.id, rx_frame.id);
	zassert_equal(last_sent_frame.dlc, rx_frame.dlc);
	zassert_mem_equal(last_sent_frame.data, rx_frame.data, rx_frame.dlc);
}
