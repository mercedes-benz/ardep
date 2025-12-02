/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Defines for Power I/O Shield hardware pin mapping

#define POWER_IO_SHIELD_OUTPUT_PINS_START 0
#define POWER_IO_SHIELD_OUTPUT_PINS_MASK 0x003F

#define POWER_IO_SHIELD_INPUT_PINS_START 8
#define POWER_IO_SHIELD_INPUT_PINS_MASK 0x3F00

#define POWER_IO_SHIELD_FAULT0_PIN 6
#define POWER_IO_SHIELD_FAULT1_PIN 14
#define POWER_IO_SHIELD_FAULT2_PIN 15

#define POWER_IO_SHIELD_HARD_INVERT_PINS POWER_IO_SHIELD_INPUT_PINS_MASK
