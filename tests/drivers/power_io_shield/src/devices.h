/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

static const struct device* power_io_shield =
    DEVICE_DT_GET(DT_NODELABEL(power_io_shield0));
static const struct emul* power_io_shield_emul =
    EMUL_DT_GET(DT_NODELABEL(power_io_shield0));
