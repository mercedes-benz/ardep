# Copyright (c) 2024 Frickly Systems GmbH
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_UDS uds)
add_subdirectory_ifdef(CONFIG_CAN_ROUTER can_router)
