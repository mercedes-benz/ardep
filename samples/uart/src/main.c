/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>

#include <zephyr/console/console.h>
#include <zephyr/kernel.h>

int main(void) {
  console_getline_init();

  while (true) {
    printk("Enter a number:\n");

    char *s = console_getline();

    double d = strtod(s, NULL);

    printk("The square of your number: %f\n", d * d);
  }

  return 0;
}
