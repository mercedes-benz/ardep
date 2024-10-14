#ifndef ARDEP_INCLUDE_DRIVERS_HV_SHIELD_H_
#define ARDEP_INCLUDE_DRIVERS_HV_SHIELD_H_

#include <zephyr/device.h>

enum hv_shield_dac_gains_t {
  HV_SHIELD_DAC_GAIN_1 = 0b0000,
  HV_SHIELD_DAC_GAIN_2 = 0b0001,
  HV_SHIELD_DAC_GAIN_4 = 0b0011,
  HV_SHIELD_DAC_GAIN_8 = 0b0111,
  HV_SHIELD_DAC_GAIN_16 = 0b1111,
};

struct hv_shield_api_t {
  int (*set_dac_gain)(const struct device* dev,
                      uint8_t dac,
                      enum hv_shield_dac_gains_t gain);
  int (*set_gpio_output_enable)(const struct device* dev,
                                uint8_t index,
                                bool enable);
};

/**
 * @brief Set Gain for DAC
 *
 * @param dev hv-shield device
 * @param dac the index of the dac channel, 0 or 1
 * @param gain DAC gain
 * @retval 0 if successful
 * @retval -EINVAL if the arguments are invalid
 * @retval other if spi write or gpio set failed
 */
__syscall int hv_shield_set_dac_gain(const struct device* dev,
                                     uint8_t dac,
                                     enum hv_shield_dac_gains_t gain);

static int z_impl_hv_shield_set_dac_gain(const struct device* dev,
                                         uint8_t dac,
                                         enum hv_shield_dac_gains_t gain) {
  const struct hv_shield_api_t* api = dev->api;
  return api->set_dac_gain(dev, dac, gain);
}

/**
 * @brief Set one of the hv gpios to either be an input or an output
 *
 * @param dev hv-shield device
 * @param index index of the gpio
 * @param enable true if gpio should be an output, false if it should be an
 * input
 * @retval 0 if successful
 * @retval -EINVAL if the arguments are invalid
 * @retval other if spi write or gpio set failed
 */
__syscall int hv_shield_set_gpio_output_enable(const struct device* dev,
                                               uint8_t index,
                                               bool enable);

static int z_impl_hv_shield_set_gpio_output_enable(const struct device* dev,
                                                   uint8_t index,
                                                   bool enable) {
  const struct hv_shield_api_t* api = dev->api;
  return api->set_gpio_output_enable(dev, index, enable);
}

#include <syscalls/hv_shield.h>

#endif
