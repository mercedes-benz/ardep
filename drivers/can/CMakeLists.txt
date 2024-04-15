# Copyright (c) 2024 Frickly Systems GmbH
# SPDX-License-Identifier: Apache-2.0


if(CONFIG_CAN_TERMINATION_GPIO)
    zephyr_library()
    zephyr_library_sources(can_termination_gpio.c)
endif()

add_subdirectory_ifdef(CONFIG_LIN2CAN lin2can)
