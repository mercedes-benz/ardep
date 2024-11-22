/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
*/

#include <zephyr/canbus/isotp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

const struct device *can_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus));

const struct isotp_fc_opts fc_opts = {.bs = 8, .stmin = 0};
const struct isotp_msg_id rx_addr = {
  .std_id = 0x80,
};
const struct isotp_msg_id tx_addr = {
  .std_id = 0x180,
};

struct isotp_recv_ctx recv_ctx;
struct isotp_send_ctx send_ctx;

K_SEM_DEFINE(can_ready_sem, 0, 1);

const char tx_data[] =
    "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy "
    "eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam "
    "voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet "
    "clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit "
    "amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam "
    "nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, "
    "sed diam voluptua. At vero eos et accusam et justo duo dolores et ea "
    "rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum "
    "dolor sit amet.\n\n";

void receiver_thread(void *arg1, void *arg2, void *arg3) {
  k_sem_take(&can_ready_sem, K_FOREVER);

  struct net_buf *buf;

  int err =
      isotp_bind(&recv_ctx, can_dev, &tx_addr, &rx_addr, &fc_opts, K_FOREVER);
  if (err != ISOTP_N_OK) {
    LOG_ERR("Failed to bind isotp (%d)", err);
    return;
  }

  int remaining;
  int received;
  while (1) {
    received = 0;
    do {
      remaining = isotp_recv_net(&recv_ctx, &buf, K_MSEC(2000));
      if (remaining < 0) {
        LOG_ERR("Receiving error (%d)", remaining);
        break;
      }

      while (buf != NULL) {
        received += buf->len;
        printk("%.*s", buf->len, buf->data);
        buf = net_buf_frag_del(NULL, buf);
      }
    } while (remaining);
    LOG_INF("Got %d bytes in total", received);
  }
}

K_THREAD_DEFINE(
    receiver_thread_h, 2048, receiver_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
  int err = 0;

  if (!device_is_ready(can_dev)) {
    LOG_ERR("CAN not ready");
    return 0;
  }

  err = can_set_mode(can_dev, 0);
  if (err != 0) {
    LOG_ERR("Failed to set mode (%d)", err);
    return 0;
  }

  err = can_start(can_dev);
  if (err != 0) {
    LOG_ERR("Error starting CAN (%d)", err);
    return 0;
  }

  k_sem_give(&can_ready_sem);

  LOG_INF("Starting to send data");

  while (1) {
    k_msleep(1000);

    err = isotp_send(&send_ctx, can_dev, tx_data, sizeof(tx_data), &tx_addr,
                     &rx_addr, NULL, NULL);
    if (err != ISOTP_N_OK) {
      printk("Error while sending data (%d)\n", err);
    }
  }
}
