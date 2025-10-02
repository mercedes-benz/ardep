/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDEP_SAMPLES_UDS_SRC_UDS_H
#define ARDEP_SAMPLES_UDS_SRC_UDS_H

#include <ardep/uds.h>

extern struct uds_instance_t instance;

struct authentication_data {
  bool authenticated;
  uint8_t seeed[16];
  uint8_t algorithm_authenticator[16];
};

// Authentication data get assigned as the instance user context
extern struct authentication_data auth_data;

#endif  // ARDEP_SAMPLES_UDS_SRC_UDS_H