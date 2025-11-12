/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/drivers/emul.h>

// Set a single 8-Bit register
void power_io_shield_emul_set_reg(const struct emul* target,
                                  uint8_t reg_addr,
                                  const uint8_t val);

// Set 2 adjacent 8-Bit registers in little endian format
void power_io_shield_emul_set_u16_reg(const struct emul* target,
                                      uint8_t reg_addr,
                                      const uint16_t val);

// Get a single 8-Bit register
uint8_t power_io_shield_emul_get_reg(const struct emul* target,
                                     uint8_t reg_addr);

// Get 2 adjacent 8-Bit registers in little endian format
uint16_t power_io_shield_emul_get_u16_reg(const struct emul* target,
                                          uint8_t reg_addr);
