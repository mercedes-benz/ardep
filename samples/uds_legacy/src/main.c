/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#ifndef CONFIG_UDS_LEGACY
#error "This sample requires the UDS_LEGACY config option to be enabled."
#endif

int main(void) { printk("Hello UDS_LEGACY!\n"); }
