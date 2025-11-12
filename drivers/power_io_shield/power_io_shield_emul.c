/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT power_io_shield

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <ardep/drivers/emul/power_io_shield.h>

LOG_MODULE_REGISTER(power_io_shield_emul, CONFIG_POWER_IO_SHIELD_LOG_LEVEL);

#define NUM_REGS 27

struct power_io_shield_emul_data {
  uint8_t reg[NUM_REGS];
};

struct power_io_shield_emul_cfg {};

void power_io_shield_emul_set_reg(const struct emul* target,
                                  uint8_t reg_addr,
                                  const uint8_t val) {
  struct power_io_shield_emul_data* data = target->data;

  __ASSERT_NO_MSG(reg_addr <= NUM_REGS);
  data->reg[reg_addr] = val;
}

void power_io_shield_emul_set_u16_reg(const struct emul* target,
                                      uint8_t reg_addr,
                                      const uint16_t val) {
  struct power_io_shield_emul_data* data = target->data;

  __ASSERT_NO_MSG(reg_addr + 1 <= NUM_REGS);
  data->reg[reg_addr] = val & 0xFF;
  data->reg[reg_addr + 1] = (val >> 8) & 0xFF;
}

uint8_t power_io_shield_emul_get_reg(const struct emul* target,
                                     uint8_t reg_addr) {
  struct power_io_shield_emul_data* data = target->data;

  __ASSERT_NO_MSG(reg_addr <= NUM_REGS);
  return data->reg[reg_addr];
}

uint16_t power_io_shield_emul_get_u16_reg(const struct emul* target,
                                          uint8_t reg_addr) {
  struct power_io_shield_emul_data* data = target->data;

  __ASSERT_NO_MSG(reg_addr + 1 <= NUM_REGS);
  return (data->reg[reg_addr + 1] << 8) | data->reg[reg_addr];
}

static int power_io_shield_emul_transfer_i2c(const struct emul* target,
                                             struct i2c_msg* msgs,
                                             int num_msgs,
                                             int addr) {
  struct power_io_shield_emul_data* data = target->data;

  i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

  if (num_msgs < 1) {
    LOG_ERR("Invalid number of messages: %d", num_msgs);
    return -EIO;
  }

  int msg_idx = 0;
  uint8_t reg_addr = 0;
  bool stop_seen = true;

  while (msg_idx < num_msgs) {
    struct i2c_msg* msg = &msgs[msg_idx];

    if (stop_seen) {
      if (msg->flags & I2C_MSG_READ) {
        LOG_ERR("Unexpected read without prior register address write");
        return -EIO;
      }
      reg_addr = msg->buf[0];
      if (msg->len > 1) {
        // write following bytes
        for (int i = 1; i < msg->len; i++) {
          data->reg[reg_addr] = msg->buf[i];
          reg_addr++;
        }
      }
    } else {
      if (msg->flags & I2C_MSG_READ) {
        // read bytes
        for (int i = 0; i < msg->len; i++) {
          msg->buf[i] = data->reg[reg_addr];
          reg_addr++;
        }
      } else {
        // write bytes
        for (int i = 0; i < msg->len; i++) {
          data->reg[reg_addr] = msg->buf[i];
          reg_addr++;
        }
      }
    }
    stop_seen = msg->flags & I2C_MSG_STOP;
    msg_idx++;
  }

  return 0;
};

static int power_io_shield_emul_init(const struct emul* target,
                                     const struct device* parent) {
  ARG_UNUSED(parent);

  struct power_io_shield_emul_data* data = target->data;

  memset(data->reg, 0, NUM_REGS);

  return 0;
}

static const struct i2c_emul_api power_io_shield_emul_api_i2c = {
  .transfer = power_io_shield_emul_transfer_i2c,
};

#define ADLTC2990_EMUL(n)                                             \
  const struct power_io_shield_emul_cfg power_io_shield_emul_cfg_##n; \
  struct power_io_shield_emul_data power_io_shield_emul_data_##n;     \
  EMUL_DT_INST_DEFINE(                                                \
      n, power_io_shield_emul_init, &power_io_shield_emul_data_##n,   \
      &power_io_shield_emul_cfg_##n, &power_io_shield_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(ADLTC2990_EMUL)
