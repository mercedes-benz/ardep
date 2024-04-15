# Copyright (c) 2024 Frickly Systems GmbH
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(lin2can-uds)

target_sources(app PRIVATE src/main.c)
