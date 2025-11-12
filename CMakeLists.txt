# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

# CMakeLists for the module

add_subdirectory(drivers)
add_subdirectory(lib)
zephyr_include_directories(include)

list(APPEND SYSCALL_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include/ardep/drivers)
set(SYSCALL_INCLUDE_DIRS ${SYSCALL_INCLUDE_DIRS} PARENT_SCOPE)
