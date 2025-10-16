/*
 * Copyright (C) Frickly Systems GmbH
 * Copyright (C) MBition GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT power_io_shield

#include "hardware.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <ardep/dt-bindings/power-io-shield.h>

LOG_MODULE_REGISTER(power_io_shield, CONFIG_POWER_IO_SHIELD_LOG_LEVEL);

#define REG_IODIRA 0x00
#define REG_IOCONA 0x0A

#define REG_INTCONA 0x08
#define REG_INTFA 0x0E
#define REG_INTCAPA 0x10
#define REG_DEFVALA 0x06
#define REG_GPINTENA 0x04
#define REG_GPIOA 0x12

// pin mask for zephyr-side pin mapping
// 6 pins each for IOs and 3 for faults
#define ZEPHYR_PINS_PORT_MASK                           \
  (((1 << 6) - 1) << POWER_IO_SHIELD_INPUT_BASE) |      \
      (((1 << 6) - 1) << POWER_IO_SHIELD_OUTPUT_BASE) | \
      (((1 << 3) - 1) << POWER_IO_SHIELD_FAULT_BASE)

struct power_io_shield_config {
  struct gpio_driver_config common;
  struct i2c_dt_spec i2c;
  struct gpio_dt_spec int_gpios[2];
  uint8_t int_gpio_count;
};

struct power_io_shield_data {
  struct gpio_driver_data common;
  const struct device*
      device;  // back reference to device struct (used in work handler)
  sys_slist_t interrupt_callbacks;
  struct gpio_callback interrupt_gpio_cb;

  struct {
    uint16_t gpinten;
    uint16_t intcon;
    uint16_t defval;
    uint16_t gpio;
  } reg_cache;

  uint16_t int_trigger_rising;
  uint16_t int_trigger_falling;

  struct k_work on_interrupt_work;
  struct k_work write_interrupt_config_work;  // to keep pin_interrupt_configure
                                              // ISR safe, we have to write the
                                              // config in a work queue item if
                                              // it is called in an ISR

  struct k_sem lock;  // we use semaphore because we can use it in the ISR-safe
                      // pin_interrupt_configure
};

static void power_io_shield_write_interrupt_config_work_handler(
    struct k_work* work);

// Write two adjacent 8-bit registers as one little-endian 16-bit value
static int write_u16_reg(const struct power_io_shield_config* config,
                         uint8_t reg_addr,
                         uint16_t value) {
  uint8_t buf[3] = {reg_addr, (uint8_t)(value & 0xFF),
                    (uint8_t)((value >> 8) & 0xFF)};
  int ret = i2c_write_dt(&config->i2c, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to write register 0x%02x: %d", reg_addr, ret);
  }
  return ret;
}

// Read two adjacent 8-bit registers as one little-endian 16-bit value
static int read_u16_reg(const struct power_io_shield_config* config,
                        uint8_t reg_addr,
                        uint16_t* value) {
  uint8_t buf[2];
  int ret = i2c_write_read_dt(&config->i2c, &reg_addr, 1, buf, sizeof(buf));
  if (ret) {
    LOG_ERR("Failed to read register 0x%02x: %d", reg_addr, ret);
    return ret;
  }
  *value = buf[0] | (buf[1] << 8);
  return 0;
}

static int write_iocon(const struct power_io_shield_config* config,
                       uint8_t value) {
  return write_u16_reg(config, REG_IOCONA, (value << 8) | value);
}

// Map zephyr pin number to mcp gpio bit number. Note that the gpio api already
// verifies that the pin number is valid through the config's common field
static int power_io_shield_zephyr_pin_to_gpio_bit(uint8_t pin) {
  uint8_t pin_base = pin & POWER_IO_SHIELD_BASE_MASK;
  uint8_t pin_index = pin & ~POWER_IO_SHIELD_BASE_MASK;

  switch (pin_base) {
    case POWER_IO_SHIELD_INPUT_BASE:
      return pin_index + POWER_IO_SHIELD_INPUT_PINS_START;

    case POWER_IO_SHIELD_OUTPUT_BASE:
      return pin_index + POWER_IO_SHIELD_OUTPUT_PINS_START;

    case POWER_IO_SHIELD_FAULT_BASE:
      switch (pin_index) {
        case 0:
          return POWER_IO_SHIELD_FAULT0_PIN;
        case 1:
          return POWER_IO_SHIELD_FAULT1_PIN;
        case 2:
          return POWER_IO_SHIELD_FAULT2_PIN;
        default:
          break;
      }

    default:
      break;
  }

  return 0;
}

// Other direction as power_io_shield_zephyr_pin_to_gpio_bit, gets a bitmask of
// mcp pins and returns a bitmask of zephyr pins
static inline uint32_t power_io_shield_gpio_bits_to_zephyr_bits(
    uint16_t mcp_pin_bits) {
  // map mcp_pin_bits to 0..5 (0..2 for faults)
  const uint32_t input_values =
      (mcp_pin_bits & POWER_IO_SHIELD_INPUT_PINS_MASK) >>
      POWER_IO_SHIELD_INPUT_PINS_START;
  const uint32_t output_values =
      (mcp_pin_bits & POWER_IO_SHIELD_OUTPUT_PINS_MASK) >>
      POWER_IO_SHIELD_OUTPUT_PINS_START;
  const uint32_t fault_values =
      ((mcp_pin_bits >> POWER_IO_SHIELD_FAULT0_PIN) & 0x1) |
      (((mcp_pin_bits >> POWER_IO_SHIELD_FAULT1_PIN) & 0x1) << 1) |
      (((mcp_pin_bits >> POWER_IO_SHIELD_FAULT2_PIN) & 0x1) << 2);

  // map bits to zephyr pin positions
  return (input_values << POWER_IO_SHIELD_INPUT_BASE) |
         (output_values << POWER_IO_SHIELD_OUTPUT_BASE) |
         (fault_values << POWER_IO_SHIELD_FAULT_BASE);
}

static void power_io_shield_int_gpio_handler(const struct device* port,
                                             struct gpio_callback* cb,
                                             gpio_port_pins_t pins) {
  struct power_io_shield_data* data =
      CONTAINER_OF(cb, struct power_io_shield_data, interrupt_gpio_cb);

  k_work_submit(&data->on_interrupt_work);
}

static void power_io_shield_interrupt_work_handler(struct k_work* work) {
  struct power_io_shield_data* data =
      CONTAINER_OF(work, struct power_io_shield_data, on_interrupt_work);
  const struct device* dev = data->device;
  const struct power_io_shield_config* config = dev->config;

  k_sem_take(&data->lock, K_FOREVER);

  uint16_t intf = 0;
  int err = read_u16_reg(config, REG_INTFA, &intf);
  if (err) {
    LOG_ERR("Error handling interrupt; could not read register: %d", err);
    goto cleanup;
  }

  if (!intf) {
    LOG_DBG("Interrupt was not for this IC");
    goto cleanup;
  }

  uint16_t intcap = 0;
  err = read_u16_reg(config, REG_INTCAPA, &intcap);
  if (err) {
    LOG_ERR("Error handling interrupt; could not read INTCAP register: %d",
            err);
    goto cleanup;
  }

  // note that these are ANDed with intf and gpinten later
  const uint16_t level_interrupts = data->reg_cache.intcon;
  const uint16_t rising_edge_interrupts = intcap & data->int_trigger_rising;
  const uint16_t falling_edge_interrupts =
      (~intcap) & data->int_trigger_falling;

  const uint16_t ints =
      data->reg_cache.gpinten & intf &
      (level_interrupts | rising_edge_interrupts | falling_edge_interrupts);

  gpio_fire_callbacks(&data->interrupt_callbacks, dev,
                      power_io_shield_gpio_bits_to_zephyr_bits(ints));

  // If any level interrupt is active, resubmit work to check if the level is
  // still active. If yes the callbacks are fired again, looping until the
  // level goes inactive
  if (level_interrupts & ints) {
    k_work_submit(&data->on_interrupt_work);
  }

cleanup:
  k_sem_give(&data->lock);
}

static int power_io_shield_port_get_raw(const struct device* port,
                                        gpio_port_value_t* value) {
  const struct power_io_shield_config* config = port->config;
  struct power_io_shield_data* data = port->data;

  k_sem_take(&data->lock, K_FOREVER);

  uint16_t reg_value;
  if (read_u16_reg(config, REG_GPIOA, &reg_value) != 0) {
    k_sem_give(&data->lock);
    return -EIO;
  }

  data->reg_cache.gpio = reg_value;
  *value = power_io_shield_gpio_bits_to_zephyr_bits(reg_value);

  k_sem_give(&data->lock);
  return 0;
}

static int power_io_shield_gpio_set_masked_raw(const struct device* port,
                                               gpio_port_pins_t mask,
                                               gpio_port_value_t value) {
  const struct power_io_shield_config* config = port->config;
  struct power_io_shield_data* data = port->data;

  // extract output bits from mask and value
  const uint8_t output_mask = mask >> POWER_IO_SHIELD_OUTPUT_BASE;
  const uint8_t output_value = value >> POWER_IO_SHIELD_OUTPUT_BASE;
  // shift to correct mcp pin bits and finally mask
  const uint16_t mapped_mask =
      (output_mask << POWER_IO_SHIELD_OUTPUT_PINS_START) &
      POWER_IO_SHIELD_OUTPUT_PINS_MASK;
  const uint16_t mapped_value =
      (output_value << POWER_IO_SHIELD_OUTPUT_PINS_START) &
      POWER_IO_SHIELD_OUTPUT_PINS_MASK;

  k_sem_take(&data->lock, K_FOREVER);

  // apply new values where mask is set
  const uint16_t new_gpio =
      (data->reg_cache.gpio & ~mapped_mask) | (mapped_value & mapped_mask);

  if (write_u16_reg(config, REG_GPIOA, new_gpio) != 0) {
    k_sem_give(&data->lock);
    return -EIO;
  }

  data->reg_cache.gpio = new_gpio;

  k_sem_give(&data->lock);
  return 0;
}

static int power_io_shield_gpio_set_bits_raw(const struct device* port,
                                             gpio_port_pins_t mask) {
  return power_io_shield_gpio_set_masked_raw(port, mask, mask);
}

static int power_io_shield_gpio_clear_bits_raw(const struct device* port,
                                               gpio_port_pins_t mask) {
  return power_io_shield_gpio_set_masked_raw(port, mask, 0);
}

// Note that configuring does nothing on this device
static int power_io_shield_pin_configure(const struct device* dev,
                                         gpio_pin_t pin,
                                         gpio_flags_t flags) {
  const struct power_io_shield_config* config = dev->config;
  struct power_io_shield_data* data = dev->data;

  uint8_t bit = power_io_shield_zephyr_pin_to_gpio_bit(pin);
  if (bit < 0) {
    return bit;
  }

  uint8_t pin_base = pin & POWER_IO_SHIELD_BASE_MASK;
  uint8_t pin_index = pin & ~POWER_IO_SHIELD_BASE_MASK;

  switch (pin_base) {
    case POWER_IO_SHIELD_INPUT_BASE:
      if (flags & GPIO_OUTPUT) {
        LOG_ERR("Cannot configure input pin %d as output", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on input pin %d", pin_index);
        return -ENOTSUP;
      }
      break;

    case POWER_IO_SHIELD_OUTPUT_BASE:
      if (flags & GPIO_INPUT) {
        LOG_ERR("Cannot configure output pin %d as input", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on output pin %d", pin_index);
        return -ENOTSUP;
      }
      if (flags & GPIO_SINGLE_ENDED) {
        LOG_ERR("Open drain/source not supported on output pin %d", pin_index);
        return -ENOTSUP;
      }

      k_sem_take(&data->lock, K_FOREVER);

      // write gpio if it should be initialized high/low
      if (flags & GPIO_OUTPUT_INIT_HIGH) {
        data->reg_cache.gpio |= (1 << bit);
      } else {
        data->reg_cache.gpio &= ~(1 << bit);
      }

      int ret = write_u16_reg(config, REG_GPIOA, data->reg_cache.gpio);

      k_sem_give(&data->lock);

      if (ret != 0) {
        return -EIO;
      }

      break;

    case POWER_IO_SHIELD_FAULT_BASE:  // Fault (treated as input)
      if (flags & GPIO_OUTPUT) {
        LOG_ERR("Cannot configure fault pin %d as output", pin_index);
        return -ENOTSUP;
      }
      if ((flags & GPIO_PULL_DOWN) | (flags & GPIO_PULL_UP)) {
        LOG_ERR("Pull up/down not supported on fault pin %d", pin_index);
        return -ENOTSUP;
      }
      break;
    default:
      return -ENOTSUP;
  }

  return 0;
}

static int power_io_shield_manage_callback(const struct device* dev,
                                           struct gpio_callback* cb,
                                           bool set) {
  struct power_io_shield_data* data = dev->data;
  return gpio_manage_callback(&data->interrupt_callbacks, cb, set);
}

static int power_io_shield_pin_interrupt_configure(const struct device* port,
                                                   gpio_pin_t pin,
                                                   enum gpio_int_mode mode,
                                                   enum gpio_int_trig trig) {
  struct power_io_shield_data* data = port->data;
  if ((pin & POWER_IO_SHIELD_BASE_MASK) != POWER_IO_SHIELD_INPUT_BASE &&
      (pin & POWER_IO_SHIELD_BASE_MASK) != POWER_IO_SHIELD_FAULT_BASE) {
    LOG_ERR("Interrupts can only be configured on input and fault pins");
    return -ENOTSUP;
  }

  // verify input to prevent gotos later
  if (mode == GPIO_INT_MODE_LEVEL) {
    switch (trig) {
      case GPIO_INT_TRIG_HIGH:
      case GPIO_INT_TRIG_LOW:
        break;
      default:
        LOG_ERR("Invalid trigger for level mode interrupt");
        return -EINVAL;
    }
  } else if (mode == GPIO_INT_MODE_EDGE) {
    switch (trig) {
      case GPIO_INT_TRIG_HIGH:
      case GPIO_INT_TRIG_LOW:
      case GPIO_INT_TRIG_BOTH:
        break;
      default:
        LOG_ERR("Invalid trigger for edge interrupt");
        return -EINVAL;
    }
  } else if (mode != GPIO_INT_MODE_DISABLED) {
    LOG_ERR("Invalid mode provided");
    return -EINVAL;
  }

  int err = k_sem_take(&data->lock, k_is_in_isr() ? K_NO_WAIT : K_FOREVER);
  if (err < 0) {
    return err;
  }

  uint8_t bit = power_io_shield_zephyr_pin_to_gpio_bit(pin);

  uint16_t defval = data->reg_cache.defval;
  uint16_t intcon =
      data->reg_cache.intcon;  // 0 = on change, 1 = compare to defval
  uint16_t gpinten = data->reg_cache.gpinten;  // 0 = disabled, 1 = enabled

  switch (mode) {
    case GPIO_INT_MODE_DISABLED:
      gpinten &= ~BIT(bit);
      intcon &= ~BIT(bit);  // reset to default of on change
      defval &= ~BIT(bit);  // reset defval to 0
      break;
    case GPIO_INT_MODE_LEVEL:
      gpinten |= BIT(bit);
      intcon |= BIT(bit);
      switch (trig) {
        case GPIO_INT_TRIG_HIGH:
          defval &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_LOW:
          defval |= BIT(bit);
          break;
        default:
          // unreachable
          break;
      }
      break;
    case GPIO_INT_MODE_EDGE:
      gpinten |= BIT(bit);
      intcon &= ~BIT(bit);
      switch (trig) {
        case GPIO_INT_TRIG_HIGH:  // rising edge
          data->int_trigger_rising |= BIT(bit);
          data->int_trigger_falling &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_LOW:  // falling edge
          data->int_trigger_falling |= BIT(bit);
          data->int_trigger_rising &= ~BIT(bit);
          break;
        case GPIO_INT_TRIG_BOTH:
          data->int_trigger_rising |= BIT(bit);
          data->int_trigger_falling |= BIT(bit);
          break;
        default:
          // unreachable
          break;
      }
      break;
    default:
      // unreachable
      break;
  }

  data->reg_cache.defval = defval;
  data->reg_cache.intcon = intcon;
  data->reg_cache.gpinten = gpinten;

  k_sem_give(&data->lock);

  int ret = 0;
  // call power_io_shield_write_interrupt_config_work_handler either through
  // workqueue (if we are in isr) or directly
  if (k_is_in_isr()) {
    int ret = k_work_submit(&data->write_interrupt_config_work);
    // ignore warnings
    if (ret > 0) {
      ret = 0;
    }
  } else {
    power_io_shield_write_interrupt_config_work_handler(
        &data->write_interrupt_config_work);
  }

  return ret;
}

static void power_io_shield_write_interrupt_config_work_handler(
    struct k_work* work) {
  struct power_io_shield_data* data = CONTAINER_OF(
      work, struct power_io_shield_data, write_interrupt_config_work);
  const struct device* dev = data->device;
  const struct power_io_shield_config* config = dev->config;

  k_sem_take(&data->lock, K_FOREVER);

  int err = write_u16_reg(config, REG_DEFVALA, data->reg_cache.defval);
  if (err != 0) {
    LOG_ERR("Could not write register: %d", err);
  }
  err = write_u16_reg(config, REG_INTCONA, data->reg_cache.intcon);
  if (err != 0) {
    LOG_ERR("Could not write register: %d", err);
  }
  err = write_u16_reg(config, REG_GPINTENA, data->reg_cache.gpinten);
  if (err != 0) {
    LOG_ERR("Could not write register: %d", err);
  }

  k_sem_give(&data->lock);
}

static int power_io_shield_init(const struct device* dev) {
  const struct power_io_shield_config* config = dev->config;
  struct power_io_shield_data* data = dev->data;

  data->device = dev;

  if (!device_is_ready(config->i2c.bus)) {
    LOG_ERR("I2C bus %s not ready", config->i2c.bus->name);
    return -ENODEV;
  }

  for (int i = 0; i < config->int_gpio_count; i++) {
    if (!device_is_ready(config->int_gpios[i].port)) {
      LOG_ERR("INT GPIO %s not ready", config->int_gpios[i].port->name);
      return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&config->int_gpios[i], GPIO_INPUT);
    if (ret != 0) {
      LOG_ERR("Failed to configure INT GPIO pin %d: %d",
              config->int_gpios[i].pin, ret);
      return ret;
    }
  }

  if (config->int_gpio_count > 0) {
    gpio_init_callback(&data->interrupt_gpio_cb,
                       power_io_shield_int_gpio_handler,
                       BIT(config->int_gpios[0].pin));
    int ret =
        gpio_add_callback(config->int_gpios[0].port, &data->interrupt_gpio_cb);
    if (ret != 0) {
      LOG_ERR("Failed to add INT GPIO callback: %d", ret);
      return ret;
    }
    ret = gpio_pin_interrupt_configure_dt(&config->int_gpios[0],
                                          GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
      LOG_ERR("Failed to configure INT GPIO interrupt: %d", ret);
      return ret;
    }
  }

  // Note, that this driver uses IOCON.BANK=0, which is the default state
  // after reset.

  // Set INT pins as open drain output, also mirror interrupts to use only one
  // interrupt pin (ODR=1, MIRROR=1)
  if (write_iocon(config, (1 << 2) | (1 << 6)) != 0) {
    return -EIO;
  }

  // Write IODIR registers (1 is input, 0 is output)
  // note the 0x7f7f mask as the pin 7 of each port is not allowed to be set as
  // input
  static const uint16_t iodir_value =
      (POWER_IO_SHIELD_INPUT_PINS_MASK | BIT(POWER_IO_SHIELD_FAULT0_PIN) |
       BIT(POWER_IO_SHIELD_FAULT1_PIN) | BIT(POWER_IO_SHIELD_FAULT2_PIN)) &
      0x7f7f;

  if (write_u16_reg(config, REG_IODIRA, iodir_value) != 0) {
    LOG_ERR("Failed to write IODIR registers");
    return -EIO;
  }

  LOG_INF("HV Shield v2 initialized on I2C address 0x%02x", config->i2c.addr);

  k_work_init(&data->on_interrupt_work, power_io_shield_interrupt_work_handler);
  k_work_init(&data->write_interrupt_config_work,
              power_io_shield_write_interrupt_config_work_handler);

  int ret = k_sem_init(&data->lock, 1, 1);
  if (ret < 0) {
    LOG_ERR("Could not initialize semaphore");
    return ret;
  }

  return 0;
}

DEVICE_API(gpio, power_io_shield_api) = {
  .pin_configure = power_io_shield_pin_configure,
  .port_get_raw = power_io_shield_port_get_raw,
  .port_set_masked_raw = power_io_shield_gpio_set_masked_raw,
  .port_set_bits_raw = power_io_shield_gpio_set_bits_raw,
  .port_clear_bits_raw = power_io_shield_gpio_clear_bits_raw,
  .manage_callback = power_io_shield_manage_callback,
  .pin_interrupt_configure = power_io_shield_pin_interrupt_configure,
};

#define POWER_IO_SHIELD_INIT(x)                                               \
  BUILD_ASSERT(DT_INST_PROP_LEN_OR(x, int_gpios, 0) <= 2);                    \
  static const struct power_io_shield_config power_io_shield_##x##_config = { \
    .common = {.port_pin_mask = ZEPHYR_PINS_PORT_MASK},                       \
    .i2c = I2C_DT_SPEC_INST_GET(x),                                           \
    .int_gpios =                                                              \
        {                                                                     \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 0, {0}),              \
          GPIO_DT_SPEC_INST_GET_BY_IDX_OR(x, int_gpios, 1, {0}),              \
        },                                                                    \
    .int_gpio_count = DT_INST_PROP_LEN_OR(x, int_gpios, 0),                   \
  };                                                                          \
  static struct power_io_shield_data power_io_shield_##x##_data = {           \
    .common = {.invert = 0},                                                  \
    .reg_cache =                                                              \
        {                                                                     \
          .gpinten = 0,                                                       \
          .intcon = 0,                                                        \
          .defval = 0,                                                        \
          .gpio = 0,                                                          \
        },                                                                    \
  };                                                                          \
  DEVICE_DT_INST_DEFINE(                                                      \
      x, power_io_shield_init, NULL, &power_io_shield_##x##_data,             \
      &power_io_shield_##x##_config, POST_KERNEL,                             \
      CONFIG_POWER_IO_SHIELD_INIT_PRIORITY, &power_io_shield_api)

DT_INST_FOREACH_STATUS_OKAY(POWER_IO_SHIELD_INIT)
