/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_INCLUDE_DT_BINDINGS_POWER_IO_SHIELD_H_
#define ARDEP_INCLUDE_DT_BINDINGS_POWER_IO_SHIELD_H_

// see power_io_shield.c
#define POWER_IO_SHIELD_INPUT_BASE 0x00
#define POWER_IO_SHIELD_OUTPUT_BASE 0x08
#define POWER_IO_SHIELD_FAULT_BASE 0x10

// mask to extract base from pin number
#define POWER_IO_SHIELD_BASE_MASK 0xf8

#define POWER_IO_SHIELD_INPUT(pin) (POWER_IO_SHIELD_INPUT_BASE | pin)
#define POWER_IO_SHIELD_OUTPUT(pin) (POWER_IO_SHIELD_OUTPUT_BASE | pin)
#define POWER_IO_SHIELD_FAULT(pin) (POWER_IO_SHIELD_FAULT_BASE | pin)

#endif
