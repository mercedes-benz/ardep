# Copyright (c) 2024 Frickly Systems GmbH
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=STM32G474VE" "--speed=4000" "--reset-after-load")

# TODO: validate settings
board_runner_args(pyocd "--target=stm32g474retx")
board_runner_args(stm32cubeprogrammer "--port=swd" "--reset-mode=hw")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/stm32cubeprogrammer.board.cmake)

include(${ZEPHYR_BASE}/boards/common/dfu-util.board.cmake)
