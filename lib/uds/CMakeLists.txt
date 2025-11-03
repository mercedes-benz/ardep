# SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
# SPDX-FileCopyrightText: Copyright (C) MBition GmbH
#
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources(authentication.c)
zephyr_library_sources(clear_diag_info.c)
zephyr_library_sources(comm_ctrl.c)
zephyr_library_sources(control_dtc_setting.c)
zephyr_library_sources(data_by_identifier.c)
zephyr_library_sources(diag_session_ctrl.c)
zephyr_library_sources(dynamically_define_data_ids.c)
zephyr_library_sources(ecu_reset.c)
zephyr_library_sources(memory_by_address.c)
zephyr_library_sources(read_dtc_info.c)
zephyr_library_sources(routine_control.c)
zephyr_library_sources(security_access.c)
zephyr_library_sources(uds.c)
zephyr_library_sources(util.c)

zephyr_library_sources_ifdef(CONFIG_UDS_FILE_TRANSFER upload_download_file_transfer.c)
zephyr_library_sources_ifdef(CONFIG_UDS_UPLOAD_DOWNLOAD_MODULE upload_download.c)
zephyr_library_sources_ifdef(CONFIG_UDS_USE_LINK_CONTROL link_control.c)

zephyr_linker_sources(SECTIONS iterables.ld)

zephyr_include_directories(.)
