/**
 * @file mcp4728.h
 * @defgroup mcp4728 mcp4728
 * @{
 *
 * ESP-IDF Driver for 12-bit DAC MCP4728
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 * EDIT Serf
 * BSD Licensed as described in the file LICENSE
 */

#include <esp_log.h>
#include <esp_idf_lib_helpers.h>
#include "mcp4728.h"

static const char *TAG = "mcp4728";

#define I2C_FREQ_HZ 1000000 // Max 1MHz for esp-idf, but device supports up to 3.4Mhz

#define MCP4728_I2CADDR_DEFAULT 0x60 ///< MCP4728 default i2c address
#define MCP4728_I2CADDR_MAX 0x68
#define MCP4728_CMD_FASTWRITE		0x00
#define MCP4728_CMD_DACWRITE_MULTI	0x40
#define MCP4728_CMD_DACWRITE_SEQ	0x50
#define MCP4728_CMD_DACWRITE_SINGLE	0x58
#define MCP4728_CMD_ADDRWRITE		0x60
#define	MCP4728_CMD_VREFWRITE		0x80
#define MCP4728_CMD_GAINWRITE		0xC0
#define MCP4728_CMD_PWRDWNWRITE		0xA0

#define MCP4728_UDAC_UPLOAD			0x1
#define MCP4728_UDAC_NOLOAD			0x0


#define BIT_READY  0x80

#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)


typedef struct {
	uint8_t id;
	uint16_t vdd;
	uint16_t valueraw[MCP4728_NUM_CH];
	uint8_t gain[MCP4728_NUM_CH];
	uint16_t vref[MCP4728_NUM_CH];
	uint8_t powerdown[MCP4728_NUM_CH];
	uint16_t evalueraw[MCP4728_NUM_CH];
	uint8_t egain[MCP4728_NUM_CH];
	uint16_t evref[MCP4728_NUM_CH];
	uint8_t epowerdown[MCP4728_NUM_CH];
} mcp4728_array_t;


static esp_err_t read_data(i2c_dev_t *dev, void *data, uint8_t size)
{
    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_read(dev, NULL, 0, data, size));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t mcp4728_init_desc(i2c_dev_t *dev, i2c_port_t port, uint8_t addr, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);
    if (addr < MCP4728_I2CADDR_DEFAULT || addr > MCP4728_I2CADDR_MAX)
    {
        ESP_LOGE(TAG, "Invalid device address: 0x%02x", addr);
        return ESP_ERR_INVALID_ARG;
    }

    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
#if HELPER_TARGET_IS_ESP32
    dev->cfg.master.clk_speed = I2C_FREQ_HZ;
#endif

    return i2c_dev_create_mutex(dev);
}

esp_err_t mcp4728_free_desc(i2c_dev_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(dev);
}

esp_err_t mcp4728_eeprom_busy(i2c_dev_t *dev, bool *busy)
{
    CHECK_ARG(dev && busy);

    uint8_t res;
    CHECK(read_data(dev, &res, 1));

    *busy = !(res & BIT_READY);

    return ESP_OK;
}

esp_err_t mcp4728_get_power_mode(i2c_dev_t *dev, bool eeprom, mcp4728_power_mode_t *mode)
{
    CHECK_ARG(dev && mode);

    uint8_t buf[4];
    CHECK(read_data(dev, buf, eeprom ? 4 : 1));

    *mode = (eeprom ? buf[3] >> 5 : buf[0] >> 1) & 0x03;

    return ESP_OK;
}

esp_err_t mcp4728_set_power_mode(i2c_dev_t *dev, bool eeprom, mcp4728_power_mode_t mode)
{
    CHECK_ARG(dev);

    uint16_t value;
    CHECK(mcp4728_get_raw_output(dev, eeprom, &value));

    uint8_t data[] = {
        (MCP4728_CMD_PWRDWNWRITE) | (((uint8_t)mode & 3) << 1),
        value >> 4,
        value << 4
    };

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, data, 3));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t mcp4728_get_raw_output(i2c_dev_t *dev, bool eeprom, uint16_t *value)
{
    CHECK_ARG(dev && value);

    uint8_t buf[5];
    CHECK(read_data(dev, buf, eeprom ? 5 : 3));

    *value = eeprom
        ? ((uint16_t)(buf[3] & 0x0f) << 8) | buf[4]
        : ((uint16_t)buf[0] << 4) | (buf[1] >> 4);

    return ESP_OK;
}

esp_err_t mcp4728_set_raw_output(i2c_dev_t *dev, uint16_t value, bool eeprom)
{
    CHECK_ARG(dev);

    uint8_t data[] = {
        (eeprom ? MCP4728_CMD_DACWRITE_SEQ : MCP4728_CMD_FASTWRITE),
        value >> 4,
        value << 4
    };

    ESP_LOGV(TAG, "Set output value to %u", value);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, data, 3));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t mcp4728_fast_write(i2c_dev_t *dev, uint16_t value)
{
    CHECK_ARG(dev);
    uint8_t data[] = {
        (value >> 8) & 0x0F,
        value & 0xFF
    };  

    ESP_LOGV(TAG, "Set output value to %u", value);

    I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, data, 2));
    I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

esp_err_t mcp4728_get_voltage(i2c_dev_t *dev, float vdd, bool eeprom, float *voltage)
{
    CHECK_ARG(voltage);

    uint16_t value;
    CHECK(mcp4728_get_raw_output(dev, eeprom, &value));

    *voltage = vdd / MCP4728_MAX_VALUE * value;

    return ESP_OK;
}

esp_err_t mcp4728_set_voltage(i2c_dev_t *dev, float vdd, float value, bool eeprom)
{
    return mcp4728_set_raw_output(dev, MCP4728_MAX_VALUE / vdd * value, eeprom);
}

esp_err_t mcp4728_write_channel_raw(i2c_dev_t *dev,uint8_t ch, uint16_t value)
{
    esp_err_t test = ESP_ERR_INVALID_ARG;
    CHECK_ARG(dev);
    if(ch < 0x03){
        test = ESP_OK;
        uint8_t data[] = {
            MCP4728_CMD_DACWRITE_SINGLE | (ch <<1),
            (1<<4)|((value >> 8) & 0x0F),
            value & 0xFF
        };  

        ESP_LOGI(TAG, "Set output value to %x , [0x%x] , [0x%x] ", data[0],data[1],data[2]);

        I2C_DEV_TAKE_MUTEX(dev);
        I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, data, 3));
        I2C_DEV_GIVE_MUTEX(dev);
    }
    return test;
}
