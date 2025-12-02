# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

add_subdirectory_ifdef(CONFIG_CAN_ROUTER can_router)
add_subdirectory_ifdef(CONFIG_ISO14229 iso14229)
add_subdirectory_ifdef(CONFIG_UDS uds)
add_subdirectory_ifdef(CONFIG_UDS_LEGACY uds_legacy)
add_subdirectory_ifdef(CONFIG_CAN_LOG can_log)
