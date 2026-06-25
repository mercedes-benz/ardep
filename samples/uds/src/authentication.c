/*
 * SPDX-FileCopyrightText: Copyright (C) Frickly Systems GmbH
 * SPDX-FileCopyrightText: Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(uds_sample, LOG_LEVEL_DBG);

#include "iso14229.h"
#include "uds.h"

#include <zephyr/kernel.h>

#include <ardep/uds.h>
#include <psa/crypto.h>

const unsigned char uds_aes_key_bin[] = {0xc3, 0x6a, 0xd3, 0x5c, 0xa2, 0xb7,
                                         0x0e, 0x82, 0xf7, 0xc3, 0x7a, 0xfa,
                                         0x62, 0xad, 0xae, 0xc2};
static const unsigned char *const uds_aes_key = uds_aes_key_bin;

struct authentication_data auth_data = {
  // Whether we are authenticated or not
  .authenticated = false,
  // Current seed we send to the client
  .seeed = {0},
  // BER encoded OID for AES-128 CBC (OID 2.16.840.1.101.3.4.1.2)
  // https://oidref.com/2.16.840.1.101.3.4.1.2
  .algorithm_authenticator = {0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03,
                              0x04, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00},
};

// AES-128 single-block ECB via the PSA Crypto API. The legacy mbedtls_aes_*
// low-level API was removed from the public surface in Mbed TLS 4.x.
static int aes_ecb_crypt(const uint8_t *key, const uint8_t *input,
                         uint8_t *output, bool encrypt) {
  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_key_id_t key_id;
  psa_status_t status;
  size_t output_len;

  if (psa_crypto_init() != PSA_SUCCESS) {
    return -1;
  }

  psa_set_key_usage_flags(
      &attr, encrypt ? PSA_KEY_USAGE_ENCRYPT : PSA_KEY_USAGE_DECRYPT);
  psa_set_key_algorithm(&attr, PSA_ALG_ECB_NO_PADDING);
  psa_set_key_type(&attr, PSA_KEY_TYPE_AES);
  psa_set_key_bits(&attr, 128);

  status = psa_import_key(&attr, key, 16, &key_id);
  if (status != PSA_SUCCESS) {
    return -1;
  }

  if (encrypt) {
    status = psa_cipher_encrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, 16,
                                output, 16, &output_len);
  } else {
    status = psa_cipher_decrypt(key_id, PSA_ALG_ECB_NO_PADDING, input, 16,
                                output, 16, &output_len);
  }

  psa_destroy_key(key_id);

  return (status == PSA_SUCCESS && output_len == 16) ? 0 : -1;
}

int aes_encrypt_ecb(const uint8_t *key, const uint8_t *input, uint8_t *output) {
  return aes_ecb_crypt(key, input, output, true);
}

int aes_decrypt_ecb(const uint8_t *key, const uint8_t *input, uint8_t *output) {
  return aes_ecb_crypt(key, input, output, false);
}

int generate_random_seed(uint8_t *seed) {
  if (psa_crypto_init() != PSA_SUCCESS) {
    return -1;
  }

  return (psa_generate_random(seed, 16) == PSA_SUCCESS) ? 0 : -1;
}

static UDSErr_t auth_check(const struct uds_context *const context,
                           bool *apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t auth_action(struct uds_context *const context,
                            bool *consume_event) {
  UDSAuthArgs_t *args = context->arg;
  struct authentication_data *auth = context->registration->auth.user_context;

  switch (args->type) {
    case UDS_LEV_AT_RCFA: {  // requestChallengeForAuthentication (0x05)
      LOG_INF("Authentication request received");

      if (memcmp(args->subFuncArgs.reqChallengeArgs.algoInd,
                 auth->algorithm_authenticator, 16) != 0) {
        LOG_ERR("Unsupported algorithm indicator");
        return UDS_NRC_ConditionsNotCorrect;
      }

      if (generate_random_seed(auth->seeed) != 0) {
        LOG_ERR("Failed to generate random seed");
        return UDS_NRC_GeneralReject;
      }

      LOG_HEXDUMP_INF(auth->seeed, 16, "Generated seed");

      uint8_t seed_len[] = {0x00, 0x10};  // 16 bytes
      args->copy(context->server, seed_len, sizeof(seed_len));
      args->copy(context->server, auth->seeed, sizeof(auth->seeed));

      uint8_t additional_param_len[] = {0x00, 0x00};
      args->copy(context->server, additional_param_len,
                 sizeof(additional_param_len));

      return args->set_auth_state(context->server, UDS_AT_RA);
    }
    case UDS_LEV_AT_VPOWNU: {  // verifyProofOfOwnershipUnidirectional (0x06)
      LOG_INF("Verify Proof Of Ownership Unidirectional");

      if (memcmp(args->subFuncArgs.verifyPownArgs.algoInd,
                 auth->algorithm_authenticator, 16) != 0) {
        LOG_ERR("Unsupported algorithm indicator\n");
        return UDS_NRC_ConditionsNotCorrect;
      }

      // Extract proof of ownership (encrypted seed)
      if (args->subFuncArgs.verifyPownArgs.pownLen != 16) {
        LOG_ERR("Invalid proof of ownership length: %d (expected 16)",
                args->subFuncArgs.verifyPownArgs.pownLen);
        return UDS_NRC_IncorrectMessageLengthOrInvalidFormat;
      }

      uint8_t *encrypted_proof =
          (uint8_t *)args->subFuncArgs.verifyPownArgs.pown;
      LOG_HEXDUMP_INF(encrypted_proof, 16, "Received encrypted proof:");

      // Decrypt the proof using our AES key
      uint8_t decrypted_proof[16];
      if (aes_decrypt_ecb(uds_aes_key, encrypted_proof, decrypted_proof) != 0) {
        LOG_ERR("Failed to decrypt proof of ownership");
        return UDS_NRC_GeneralReject;
      }

      LOG_HEXDUMP_INF(decrypted_proof, 16, "Decrypted proof:");
      LOG_HEXDUMP_INF(auth->seeed, 16, "Expected seed:");

      // Compare decrypted proof with the seed we sent
      if (memcmp(decrypted_proof, auth->seeed, 16) != 0) {
        LOG_ERR(
            "Proof verification failed - decrypted proof does not match "
            "seed\n");
        auth->authenticated = false;
        return UDS_NRC_InvalidKey;
      }

      LOG_INF("Proof verification successful! Client is now authenticated.");
      auth->authenticated = true;

      uint8_t session_key_len[] = {0x00, 0x00};
      args->copy(context->server, session_key_len, sizeof(session_key_len));
      return args->set_auth_state(context->server, UDS_AT_OVAC);
    }
    case UDS_LEV_AT_DA: {  // deAuthenticate (0x00)
      LOG_INF("De-authenticate request");
      auth->authenticated = false;

      UDSErr_t ret = args->set_auth_state(context->server, UDS_AT_DAS);
      if (ret != UDS_OK) {
        LOG_INF("Failed to copy de-authentication response");
        return ret;
      }

      return UDS_OK;
    }
    default:
      return UDS_NRC_SubFunctionNotSupported;
  }
}

static UDSErr_t auth_timeout_check(const struct uds_context *const context,
                                   bool *apply_action) {
  *apply_action = true;
  return UDS_OK;
}

static UDSErr_t auth_timeout_action(struct uds_context *const context,
                                    bool *consume_event) {
  struct authentication_data *auth = context->registration->auth.user_context;
  auth->authenticated = false;

  LOG_INF("Authentication timeout - user de-authenticated");

  *consume_event = true;
  return UDS_PositiveResponse;
}

UDS_REGISTER_AUTHENTICATION_HANDLER(&uds_default_instance,
                                    auth_check,
                                    auth_action,
                                    auth_timeout_check,
                                    auth_timeout_action,
                                    &auth_data);
