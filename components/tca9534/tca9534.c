/**
 * @file tca9534.c
 *
 * ESP-IDF Functions for I2C IO expander
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 * EDIT Serf
 * BSD Licensed as described in the file LICENSE
 */

#include <esp_idf_lib_helpers.h>
#include "tca9534.h"
#include <esp_log.h>
#define I2C_FREQ_HZ 400000

#define REG_IN0   0x00
#define REG_OUT0  0x01
#define REG_POL0  0x02
#define REG_CONF0 0x03

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define BV(x) (1 << (x))
static const char *TAG = "tca9534";

static esp_err_t read_reg_8(i2c_dev_t *dev, uint8_t reg, uint8_t *val)
{
    CHECK_ARG(dev && val);
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read_reg(dev, reg, val, 1));
    I2C_DEV_GIVE_MUTEX(dev);
    ESP_LOGI(TAG,"Reading reg: 0x%x val:0x%x",reg,*val);
    
    return ESP_OK;
}

static esp_err_t write_reg_8(i2c_dev_t *dev, uint8_t reg, uint8_t val)
{
    CHECK_ARG(dev);

    I2C_DEV_TAKE_MUTEX(dev);
    ESP_LOGI(TAG,"Writing reg: 0x%x val:0x%x",reg,val);
    I2C_DEV_CHECK(dev, i2c_dev_write_reg(dev, reg, &val, 1));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t tca9534_init_desc(i2c_dev_t *dev, i2c_port_t port, uint8_t addr, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev && (addr & TCA9534_I2C_ADDR_BASE));

    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(dev);
}

esp_err_t tca9534_free_desc(i2c_dev_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(dev);
}

esp_err_t tca9534_port_get_mode(i2c_dev_t *dev, uint8_t *mode)
{
    return read_reg_8(dev, REG_CONF0, mode);
}

esp_err_t tca9534_port_set_mode(i2c_dev_t *dev, uint8_t mode)
{
    return write_reg_8(dev, REG_CONF0, mode);
}

esp_err_t tca9534_port_get_polarity(i2c_dev_t *dev, uint8_t *polarity)
{
    return read_reg_8(dev, REG_POL0, polarity);
}

esp_err_t tca9534_port_set_polarity(i2c_dev_t *dev, uint8_t polarity)
{
    return write_reg_8(dev, REG_POL0, polarity);
}

esp_err_t tca9534_port_read(i2c_dev_t *dev, uint8_t *val)
{
    return read_reg_8(dev, REG_IN0, val);
}

esp_err_t tca9534_port_write(i2c_dev_t *dev, uint8_t val)
{
    return write_reg_8(dev, REG_OUT0, val);
}

esp_err_t tca9534_get_level(i2c_dev_t *dev, uint8_t pin, uint8_t *val)
{
    uint8_t v;
    CHECK(read_reg_8(dev, REG_IN0, &v));
    *val = v & BV(pin) ? 1 : 0;

    return ESP_OK;
}

esp_err_t tca9534_set_level(i2c_dev_t *dev, uint8_t pin, uint8_t val)
{
    uint8_t v;

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read_reg(dev, REG_OUT0, &v, 1));
    v = (v & ~BV(pin)) | (val ? BV(pin) : 0);
    I2C_DEV_CHECK(dev, i2c_dev_write_reg(dev, REG_OUT0, &v, 1));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

