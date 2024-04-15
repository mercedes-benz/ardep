/*
 * Copyright (c) 2024 Frickly Systems GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/


#include <zephyr/kernel.h>

#if !CONFIG_UDS
#error "This sample requires UDS"
#endif

int main(void) { printk("Hello UDS!\n"); }
