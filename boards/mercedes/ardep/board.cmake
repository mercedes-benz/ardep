# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32G474VE" "--speed=4000" "--reset-after-load")

# TODO: validate settings
board_runner_args(pyocd "--target=stm32g474retx")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")


if(BOARD_REVISION STREQUAL "1.0.0")
    include(${CMAKE_CURRENT_LIST_DIR}/../../common/ardep.board.cmake)
elseif(BOARD_REVISION STREQUAL "2.0.0")
    include(${ZEPHYR_BASE}/boards/common/blackmagicprobe.board.cmake)
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
