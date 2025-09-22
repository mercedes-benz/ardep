/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_LIB_UDS_UDS_H
#define ARDEP_LIB_UDS_UDS_H

#include <ardep/iso14229.h>
#include <ardep/uds.h>
#include <iso14229.h>

/**
 * @brief Associated events with other data required to handle them
 */
struct uds_event_handler_data {
  UDSEvent_t event;
  uds_get_check_fn get_check;
  uds_get_action_fn get_action;
  UDSErr_t default_nrc;
  enum uds_registration_type_t registration_type;
};

#endif  // ARDEP_LIB_UDS_UDS_H