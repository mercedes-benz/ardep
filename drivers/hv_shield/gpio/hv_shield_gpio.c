#define DT_DRV_COMPAT hv_shield_gpio

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <ardep/drivers/hv_shield.h>

LOG_MODULE_DECLARE(hv_shield);

#define PIN_IN_RANGE_CHECK(pin, config)                                     \
  if (pin >= config->lv_gpios_count) {                                      \
    LOG_ERR("Pin outside the range of given gpios (pin %d, known %d)", pin, \
            config->lv_gpios_count);                                        \
    return -EINVAL;                                                         \
  }

struct hv_shield_gpio_config_t {
  // needed by gpio api:
  struct gpio_driver_config gpio_driver;

  const struct device* main_dev;
  const struct gpio_dt_spec* lv_gpios;
  size_t lv_gpios_count;
};

struct hv_shield_gpio_data_t {
  // needed by gpio api:
  struct gpio_driver_data gpio_driver;

  uint32_t input_map;
  uint32_t output_map;
};

static int hvs_gpio_pin_configure(const struct device* dev,
                                  gpio_pin_t pin,
                                  gpio_flags_t flags) {
  const struct hv_shield_gpio_config_t* config = dev->config;
  struct hv_shield_gpio_data_t* data = dev->data;

  PIN_IN_RANGE_CHECK(pin, config);

  // configure underlying gpio
  int err = gpio_pin_configure_dt(&config->lv_gpios[pin], flags);
  if (err) {
    LOG_ERR("Error pin config failed (%d)", err);
    return err;
  }

  // set pin direction in the shield controller
  err = hv_shield_set_gpio_output_enable(config->main_dev, pin,
                                         flags & GPIO_OUTPUT);
  if (err) {
    LOG_ERR("Error shield set output enable failed (%d)", err);
    return err;
  }

  if (flags & GPIO_INPUT) {
    data->input_map |= BIT(pin);
  } else {
    data->input_map &= ~BIT(pin);
  }

  if (flags & GPIO_OUTPUT) {
    data->output_map |= BIT(pin);
  } else {
    data->output_map &= ~BIT(pin);
  }

  return 0;
}

static int hvs_gpio_init(const struct device* dev) {
  struct hv_shield_gpio_data_t* data = dev->data;
  data->input_map = 0;
  return 0;
}

static int hvs_pin_interrupt_configure(const struct device* dev,
                                       gpio_pin_t pin,
                                       enum gpio_int_mode mode,
                                       enum gpio_int_trig trig) {
  const struct hv_shield_gpio_config_t* config = dev->config;

  PIN_IN_RANGE_CHECK(pin, config);

  const struct device* port = config->lv_gpios[pin].port;
  const int port_pin = config->lv_gpios[pin].pin;
  const struct gpio_driver_api* port_api = port->api;

  return port_api->pin_interrupt_configure(port, port_pin, mode, trig);
}

#if defined(CONFIG_GPIO_GET_CONFIG)
static int hvs_gpio_pin_get_config(const struct device* dev,
                                   gpio_pin_t pin,
                                   gpio_flags_t* flags) {
  const struct hv_shield_gpio_config_t* config = dev->config;
  PIN_IN_RANGE_CHECK(pin, config);

  return gpio_pin_get_config_dt(&config->lv_gpios[pin], flags);
}
#endif

static int hvs_gpio_port_get_raw(const struct device* port,
                                 gpio_port_value_t* value) {
  const struct hv_shield_gpio_config_t* config = port->config;
  struct hv_shield_gpio_data_t* data = port->data;

  if (!value) {
    return -EINVAL;
  }
  *value = 0;

  for (int i = 0; i < config->lv_gpios_count; i++) {
    // if not set as input skip
    if (!(data->input_map & BIT(i))) continue;

    // read gpio value
    int ret = gpio_pin_get_dt(&config->lv_gpios[i]);
    if (ret < 0) {
      LOG_ERR("Error reading gpio %d (error %d)", i, ret);
      return ret;
    }

    if (ret == 1) {
      *value |= BIT(i);
    }
  }

  return 0;
}

static int hvs_set_masked_raw(const struct device* port,
                              gpio_port_pins_t mask,
                              gpio_port_value_t value) {
  const struct hv_shield_gpio_config_t* config = port->config;

  for (int i = 0; i < config->lv_gpios_count; i++) {
    // if not enabled in mask, skip
    if (!(mask & BIT(i))) continue;

    int ret = gpio_pin_set(config->lv_gpios[i].port, config->lv_gpios[i].pin,
                           !!(value & BIT(i)));
    if (ret < 0) {
      LOG_ERR("Error settings gpio %d (error %d)", i, ret);
      return ret;
    }
  }

  return 0;
}

/**
 * @brief Internal helper macro to loop through all pins of the driver and run a
 * function with signature int(const struct device*,gpio_port_pins_t mask) for
 * all pins that have a bit set in given mask. Runs given function with
 * underlying port device and mask with single bit set for underlying port pin
 * @param hv_shield_gpio_dev hv_shield_gpio device
 * @param pin_mask mask with bits set for all pins to be looped through
 * @param api_func_name name of the gpio_driver_api function that has to be
 * called
 * @param error_msg string that describes the function in error messages (e.g.
 * "setting dir")
 */
#define _HVS_GPIO_RUN_LL_API_FOREACH_MASK(hv_shield_gpio_dev, pin_mask,        \
                                          api_func_name, error_msg)            \
  {                                                                            \
    const struct hv_shield_gpio_config_t* config = hv_shield_gpio_dev->config; \
    for (int i = 0; i < config->lv_gpios_count; i++) {                         \
      /* Skip if not in mask */                                                \
      if (!(pin_mask & BIT(i))) continue;                                      \
      const struct device* port = config->lv_gpios[i].port;                    \
      const int port_pin = config->lv_gpios[i].pin;                            \
      struct gpio_driver_api* port_api = (struct gpio_driver_api*)port->api;   \
                                                                               \
      if (!port_api->api_func_name) {                                          \
        LOG_ERR("Port %s does not implement function " #api_func_name,         \
                port->name);                                                   \
      }                                                                        \
                                                                               \
      int ret = port_api->api_func_name(port, BIT(port_pin));                  \
      if (ret < 0) {                                                           \
        LOG_ERR("Error " error_msg " %d (error %d)", i, ret);                  \
        return ret;                                                            \
      }                                                                        \
    }                                                                          \
  }

static int hvs_gpio_clear_pins(const struct device* port,
                               gpio_port_pins_t pins) {
  _HVS_GPIO_RUN_LL_API_FOREACH_MASK(port, pins, port_clear_bits_raw,
                                    "clearing gpio");

  return 0;
}

static int hvs_gpio_set_pins(const struct device* port, gpio_port_pins_t pins) {
  _HVS_GPIO_RUN_LL_API_FOREACH_MASK(port, pins, port_set_bits_raw,
                                    "settings gpio");

  return 0;
}

static int hvs_gpio_toggle_pins(const struct device* port,
                                gpio_port_pins_t pins) {
  _HVS_GPIO_RUN_LL_API_FOREACH_MASK(port, pins, port_toggle_bits,
                                    "toggling gpio");

  return 0;
}

static int hvs_gpio_manage_callback(const struct device* port,
                                    struct gpio_callback* callback,
                                    bool set) {
  // todo: implement

  // go through all known ports (important: not through all pins!) and "manage"
  // callback for each port

  // problem: how can we store the ports we have added a callback with neither a
  // list of ports nor the number of ports (for a static list) -> would require
  // a dynamic list

  return -ENOSYS;
}

static uint32_t hvs_gpio_get_pending_int(const struct device* port) {
  // problem: we can only check if one of the underlying ports has a pending
  // interrupt, not if this pending interrupt is set for one of the pins known

  // solution: die one death and just return if one of the underlying ports has
  // a pending interrupt

  const struct hv_shield_gpio_config_t* config = port->config;
  for (int i = 0; i < config->lv_gpios_count; i++) {
    const struct device* port = config->lv_gpios[i].port;
    struct gpio_driver_api* port_api = (struct gpio_driver_api*)port->api;

    if (!port_api->get_pending_int) {
      LOG_ERR("Pending interrupts are not supported for port %s", port->name);
      return -ENOSYS;
    }

    uint32_t ret = port_api->get_pending_int(port);
    if (ret == -ENOSYS) {
      LOG_ERR(
          "Error getting pending interrupt for port %s, driver does not "
          "support it",
          port->name);
      return ret;
    }

    if (ret > 0) return ret;
  }

  return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int hvs_gpio_get_direction(const struct device* port,
                                  gpio_port_pins_t map,
                                  gpio_port_pins_t* inputs,
                                  gpio_port_pins_t* outputs) {
  struct hv_shield_gpio_data_t* data = port->data;

  if (inputs) {
    *inputs = data->input_map & map;
  }

  if (outputs) {
    *outputs = data->output_map & map;
  }

  return 0;
}
#endif

static struct gpio_driver_api hvs_gpio_api = {
  .pin_configure = hvs_gpio_pin_configure,
#if defined(CONFIG_GPIO_GET_CONFIG)
  .pin_get_config = hvs_gpio_pin_get_config,
#endif
  .port_get_raw = hvs_gpio_port_get_raw,
  .port_set_masked_raw = hvs_set_masked_raw,
  .port_set_bits_raw = hvs_gpio_set_pins,
  .port_clear_bits_raw = hvs_gpio_clear_pins,
  .port_toggle_bits = hvs_gpio_toggle_pins,
  .pin_interrupt_configure = hvs_pin_interrupt_configure,
  .manage_callback = hvs_gpio_manage_callback,
  .get_pending_int = hvs_gpio_get_pending_int,
#ifdef CONFIG_GPIO_GET_DIRECTION
  .port_get_direction = hvs_gpio_get_direction,
#endif
};

#define HV_SHIELD_GPIO_INIT(n)                                          \
  static const struct gpio_dt_spec hv_shield_lv_gpios_##n[] = {         \
    DT_INST_FOREACH_PROP_ELEM_SEP(n, low_voltage_gpios,                 \
                                  GPIO_DT_SPEC_GET_BY_IDX, (, )),       \
  };                                                                    \
  BUILD_ASSERT(ARRAY_SIZE(hv_shield_lv_gpios_##n) <= 32,                \
               "HV Shield GPIO has too many low voltage gpios");        \
                                                                        \
  static const struct hv_shield_gpio_config_t hv_shield_config_##n = {  \
    .gpio_driver.port_pin_mask = GPIO_DT_INST_PORT_PIN_MASK_NGPIOS_EXC( \
        n, DT_INST_PROP_LEN(n, low_voltage_gpios)),                     \
    .main_dev = DEVICE_DT_GET(DT_INST_BUS(n)),                          \
    .lv_gpios = hv_shield_lv_gpios_##n,                                 \
    .lv_gpios_count = ARRAY_SIZE(hv_shield_lv_gpios_##n),               \
  };                                                                    \
                                                                        \
  static struct hv_shield_gpio_data_t hv_shield_data_##n = {            \
    .gpio_driver.invert = 0,                                            \
  };                                                                    \
                                                                        \
  DEVICE_DT_INST_DEFINE(n, hvs_gpio_init, NULL, &hv_shield_data_##n,    \
                        &hv_shield_config_##n, POST_KERNEL,             \
                        CONFIG_HV_SHIELD_GPIO_INIT_PRIORITY,            \
                        &hvs_gpio_api);  // todo: config for init priority

DT_INST_FOREACH_STATUS_OKAY(HV_SHIELD_GPIO_INIT)
