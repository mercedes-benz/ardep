/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hv_shield

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/hv_shield.h>

LOG_MODULE_REGISTER(hv_shield, CONFIG_HV_SHIELD_LOG_LEVEL);

struct hv_shield_config_t {
  struct spi_dt_spec spi_spec;
  struct gpio_dt_spec oe_gpio_spec;
};

struct __packed hv_shield_registers_t {
  uint32_t gpio_output : 32;  // 1 for output, 0 for input
  enum hv_shield_dac_gains_t dac0 : 4;
  enum hv_shield_dac_gains_t dac1 : 4;
};

BUILD_ASSERT(sizeof(struct hv_shield_registers_t) == 5,
             "Invalid registers size.");

struct hv_shield_data_t {
  struct hv_shield_registers_t registers;
};

/**
 * @brief Internal: writes registers to the shield through spi
 *
 * @param dev hv_shield device
 * @retval 0 on success
 * @retval other from spi_write_dt on error
 */
static int _hv_shield_update(const struct device* dev) {
  const struct hv_shield_config_t* config = dev->config;
  struct hv_shield_data_t* data = dev->data;

  LOG_HEXDUMP_DBG(&data->registers, sizeof(data->registers),
                  "Writing register data");

  uint8_t* raw_buffer = (uint8_t*)&data->registers;
  // bits have to be flipped to match shift registers
  uint8_t rotated_buffer[sizeof(data->registers)];

  // copy inverted bytes (bits are inverted through SPI output order(MSB))
  for (int i = 0; i < sizeof(rotated_buffer); i++) {
    rotated_buffer[i] = raw_buffer[sizeof(rotated_buffer) - i - 1];
  }

  struct spi_buf buffer[] = {
    {.buf = rotated_buffer, .len = sizeof(rotated_buffer)},
  };

  struct spi_buf_set buffer_set = {
    .buffers = buffer,
    .count = ARRAY_SIZE(buffer),
  };

  int err = spi_write_dt(&config->spi_spec, &buffer_set);
  if (err) {
    LOG_ERR("Error writing to spi (%d)", err);
    return err;
  }

  return 0;
}

static int hv_shield_init(const struct device* dev) {
  const struct hv_shield_config_t* config = dev->config;
  struct hv_shield_data_t* data = dev->data;

  int err = gpio_pin_configure_dt(&config->oe_gpio_spec, GPIO_OUTPUT_INACTIVE);
  if (err) {
    LOG_ERR("Error setting up Output enable pin (%d)", err);
    return err;
  }

  // set everything to known state
  memset(&data->registers, 0, sizeof(data->registers));

  err = _hv_shield_update(dev);
  if (err) {
    LOG_ERR("Error writing registers (%d)", err);
    return err;
  }

  err = gpio_pin_set_dt(&config->oe_gpio_spec, 1);
  if (err) {
    LOG_ERR("Error setting enabling output (%d)", err);
  }

  return err;
}

static int hvs_set_dac_gain(const struct device* dev,
                            uint8_t dac,
                            enum hv_shield_dac_gains_t gain) {
  if (dac > 1) return -EINVAL;

  struct hv_shield_data_t* data = dev->data;
  switch (dac) {
    case 0:
      data->registers.dac0 = gain;
      break;
    case 1:
      data->registers.dac1 = gain;
      break;
    default:
      return -EINVAL;
  }

  return _hv_shield_update(dev);
}

static int hvs_set_gpio_output_enable(const struct device* dev,
                                      uint8_t index,
                                      bool enable) {
  if (index > 31) return -EINVAL;

  struct hv_shield_data_t* data = dev->data;

  // index has to be remapped a bit (first 4 bits correspond to 4-7 and next to
  // 0-3 and that for each byte)
  if (index % 8 < 4) {
    index += 4;
  } else {
    index -= 4;
  }

  if (enable) {
    data->registers.gpio_output |= 1 << index;
  } else {
    data->registers.gpio_output &= ~(1 << index);
  }

  return _hv_shield_update(dev);
}

struct hv_shield_api_t api = {
  .set_dac_gain = hvs_set_dac_gain,
  .set_gpio_output_enable = hvs_set_gpio_output_enable,
};

#define HV_SHIELD_EACH(n)                                               \
  static const struct hv_shield_config_t hv_shield_config_##n = {       \
    .spi_spec =                                                         \
        SPI_DT_SPEC_INST_GET(n, SPI_TRANSFER_MSB | SPI_WORD_SET(8), 0), \
    .oe_gpio_spec = GPIO_DT_SPEC_INST_GET(n, oe_gpios),                 \
  };                                                                    \
  struct hv_shield_data_t hv_shield_data_##n = {};                      \
  DEVICE_DT_INST_DEFINE(n, hv_shield_init, NULL, &hv_shield_data_##n,   \
                        &hv_shield_config_##n, POST_KERNEL,             \
                        CONFIG_HV_SHIELD_INIT_PRIORITY, &api);

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_EACH);
