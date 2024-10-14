#define DT_DRV_COMPAT hv_shield_dac
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(hv_shield_dac, CONFIG_HV_SHIELD_LOG_LEVEL);

#include <ardep/drivers/hv_shield.h>

struct hv_shield_dac_config_t {
  const struct device* hv_shield;

  uint8_t channel_count;

  struct {
    const struct device** dac_devs;
    const int* dac_channels;
  } mapping;

  const uint8_t* gains;
};

static int hvs_dac_channel_setup(const struct device* dev,
                                 const struct dac_channel_cfg* channel_cfg) {
  const struct hv_shield_dac_config_t* config = dev->config;

  if (channel_cfg->channel_id >= config->channel_count) {
    LOG_ERR("Invalid channel id %d in setup (%d known)",
            channel_cfg->channel_id, config->channel_count);
    return -EINVAL;
  }

  const struct device* dac = config->mapping.dac_devs[channel_cfg->channel_id];

  struct dac_channel_cfg channel_cfg_for_dac = {
    .channel_id = config->mapping
                      .dac_channels[channel_cfg->channel_id],  // mapped channel
    .resolution = channel_cfg->resolution,
    .buffered = channel_cfg->buffered,
  };

  LOG_DBG("Channel setup mapped from channel %d to dac (%s) channel %d",
          channel_cfg->channel_id, dac->name, channel_cfg_for_dac.channel_id);

  return dac_channel_setup(dac, &channel_cfg_for_dac);
}

/*
 * Type definition of DAC API function for setting a write request.
 * See dac_write_value() for argument descriptions.
 */
static int hvs_dac_write_value(const struct device* dev,
                               uint8_t channel,
                               uint32_t value) {
  const struct hv_shield_dac_config_t* config = dev->config;
  if (channel >= config->channel_count) {
    LOG_ERR("Invalid channel id %d in setup (%d known)", channel,
            config->channel_count);
    return -EINVAL;
  }

  const struct device* dac = config->mapping.dac_devs[channel];
  uint8_t mapped_channel = config->mapping.dac_channels[channel];

  LOG_DBG("Channel write mapped from channel %d to dac (%s) channel %d",
          channel, dac->name, mapped_channel);

  return dac_write_value(dac, mapped_channel, value);
}

static int hvs_dac_convert_gain(int raw_gain,
                                enum hv_shield_dac_gains_t* gain) {
  switch (raw_gain) {
    case 1:
      *gain = HV_SHIELD_DAC_GAIN_1;
      return 0;
    case 2:
      *gain = HV_SHIELD_DAC_GAIN_2;
      return 0;
    case 4:
      *gain = HV_SHIELD_DAC_GAIN_4;
      return 0;
    case 8:
      *gain = HV_SHIELD_DAC_GAIN_8;
      return 0;
    case 16:
      *gain = HV_SHIELD_DAC_GAIN_16;
      return 0;
    default:
      return -ENOTSUP;
  }
}

static int hv_shield_dac_init(const struct device* dev) {  //
  const struct hv_shield_dac_config_t* config = dev->config;

  for (int i = 0; i < config->channel_count; i++) {
    enum hv_shield_dac_gains_t gain;
    int rc = hvs_dac_convert_gain(config->gains[i], &gain);
    if (rc) {
      LOG_ERR("Invalid gain %d for channel %d", config->gains[i], i);
      return rc;
    }

    rc = hv_shield_set_dac_gain(config->hv_shield, i, gain);
    if (rc) {
      LOG_ERR("Failed to set gain %d for channel %d", config->gains[i], i);
      return rc;
    }
  }

  return 0;
}

static struct dac_driver_api hv_shield_dac_api = {
  .channel_setup = hvs_dac_channel_setup,
  .write_value = hvs_dac_write_value,
};

#define DACS_GET_DEVICE(n, prop, index) \
  DEVICE_DT_GET(DT_PHANDLE_BY_IDX(n, prop, index))

#define HV_SHIELD_DAC_INIT(n)                                              \
  static const struct device* hv_shield_mapped_dacs_##n[] = {              \
    DT_INST_FOREACH_PROP_ELEM_SEP(n, io_channels, DACS_GET_DEVICE, (, )),  \
  };                                                                       \
  static const int hv_shield_mapped_dac_channels_##n[] =                   \
      DT_INST_PROP(n, io_channels_channel);                                \
                                                                           \
  static const uint8_t hv_shield_dac_gains_##n[] = DT_INST_PROP(n, gains); \
                                                                           \
  const struct hv_shield_dac_config_t hv_shield_dac_config_##n = {         \
    .hv_shield = DEVICE_DT_GET(DT_INST_BUS(n)),                            \
    .channel_count = DT_INST_PROP_LEN(n, io_channels),                     \
    .mapping.dac_devs = hv_shield_mapped_dacs_##n,                         \
    .mapping.dac_channels = hv_shield_mapped_dac_channels_##n,             \
    .gains = hv_shield_dac_gains_##n,                                      \
  };                                                                       \
                                                                           \
  DEVICE_DT_INST_DEFINE(                                                   \
      n, hv_shield_dac_init, NULL, NULL, &hv_shield_dac_config_##n,        \
      POST_KERNEL, CONFIG_HV_SHIELD_DAC_INIT_PRIORITY, &hv_shield_dac_api);

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_DAC_INIT);
