# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(firmware_loader)

file(GLOB SRC_FILES src/*.c)
target_sources(app PRIVATE ${SRC_FILES})
