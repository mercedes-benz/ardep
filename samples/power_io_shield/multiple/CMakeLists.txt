# Copyright (C) Frickly Systems GmbH
# Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(SHIELD power_io_shield_0;power_io_shield_1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(power_io_shield_multiple)

target_sources(app PRIVATE src/main.c)
