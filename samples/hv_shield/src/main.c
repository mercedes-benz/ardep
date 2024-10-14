#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

static const struct gpio_dt_spec gpio =
    GPIO_DT_SPEC_GET(DT_PATH(zephyr_user), gpios);

static const struct device* dac =
    DEVICE_DT_GET(DT_PHANDLE(DT_PATH(zephyr_user), io_channels));

static struct dac_channel_cfg channel_cfg = {
  .channel_id = DT_PROP(DT_PATH(zephyr_user), io_channels_channel),
  .buffered = true,
  .resolution = 12,
};

int main() {
  int err = gpio_pin_configure_dt(&gpio, GPIO_OUTPUT);
  if (err) {
    LOG_ERR("Error setting up on GPIO: %d", err);
    return err;
  }

  err = dac_channel_setup(dac, &channel_cfg);
  if (err) {
    LOG_ERR("Error setting up DAC: %d", err);
    return err;
  }

  uint16_t dac_val = 0;
  bool output = false;
  for (;;) {
    if (dac_val % 1000 == 0) {
      err = gpio_pin_set_dt(&gpio, output);
      if (err) {
        LOG_ERR("Error setting output: %d", err);
      }
      output = !output;
    }

    err = dac_write_value(dac, channel_cfg.channel_id, dac_val++);
    if (err) {
      LOG_ERR("Error writing dac value: %d", err);
    }

    // 12 bit resolution -> max val = 0xfff
    if (dac_val == 0x1000) {
      dac_val = 0;
    }

    k_msleep(10);
  }
}
