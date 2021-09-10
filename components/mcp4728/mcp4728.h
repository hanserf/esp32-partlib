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
#ifndef __MCP4728_H__
#define __MCP4728_H__

#include <stdbool.h>
#include <i2cdev.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCP4728A0_I2C_ADDR0 0x60
#define MCP4728A0_I2C_ADDR1 0x61
#define MCP4728A1_I2C_ADDR0 0x62
#define MCP4728A1_I2C_ADDR1 0x63
#define MCP4728A2_I2C_ADDR0 0x64
#define MCP4728A2_I2C_ADDR1 0x65

#define MCP4728_MAX_VALUE 0x0fff
#define NUM_MCP4728 1
/**
 * Power mode, see datasheet
 */
typedef enum
{
    MCP4728_PM_NORMAL = 0,   //!< Normal mode
    MCP4728_PM_PD_1K,        //!< Power down, 1kOhm resistor to ground
    MCP4728_PM_PD_100K,      //!< Power down, 100kOhm resistor to ground
    MCP4728_PM_PD_500K,      //!< Power down, 500kOhm resistor to ground
} mcp4728_power_mode_t;

typedef enum
{
    MCP4728_CH_A = 0,   //!< Normal mode
    MCP4728_CH_B,        //!< Power down, 1kOhm resistor to ground
    MCP4728_CH_C,      //!< Power down, 100kOhm resistor to ground
    MCP4728_CH_D,      //!< Power down, 500kOhm resistor to ground
    MCP4728_NUM_CH,
} mcp4728_channel_t;



/**
 * @brief Initialize device descriptor
 *
 * Default SCL frequency is 1MHz
 *
 * @param dev I2C device descriptor
 * @param port I2C port number
 * @param addr I2C address,
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_init_desc(i2c_dev_t *dev, i2c_port_t port, uint8_t addr, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Free device descriptor
 *
 * @param dev I2C device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_free_desc(i2c_dev_t *dev);

/**
 * @brief Get device EEPROM status
 *
 * @param dev I2C device descriptor
 * @param busy true when EEPROM is busy
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_eeprom_busy(i2c_dev_t *dev, bool *busy);

/**
 * @brief Get power mode
 *
 * @param dev I2C device descriptor
 * @param eeprom Read power mode from EEPROM if true
 * @param[out] mode Power mode
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_get_power_mode(i2c_dev_t *dev, bool eeprom, mcp4728_power_mode_t *mode);

/**
 * @brief Set power mode
 *
 * @param dev I2C device descriptor
 * @param mode Power mode
 * @param eeprom Store mode to device EEPROM if true
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_set_power_mode(i2c_dev_t *dev, bool eeprom, mcp4728_power_mode_t mode);

/**
 * @brief Get current DAC value
 *
 * @param dev I2C device descriptor
 * @param eeprom Read value from device EEPROM if true
 * @param[out] value Raw output value, 0..4095
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_get_raw_output(i2c_dev_t *dev, bool eeprom, uint16_t *value);

/**
 * @brief Set DAC output value
 *
 * @param dev I2C device descriptor
 * @param value Raw output value, 0..4095
 * @param eeprom Store value to device EEPROM if true
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_set_raw_output(i2c_dev_t *dev, uint16_t value, bool eeprom);

/**
 * @brief Get current DAC output voltage
 *
 * @param dev I2C device descriptor
 * @param vdd Device operating voltage, volts
 * @param eeprom Read voltage from device EEPROM if true
 * @param[out] voltage Current output voltage, volts
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_get_voltage(i2c_dev_t *dev, float vdd, bool eeprom, float *voltage);

/**
 * @brief Set DAC output voltage
 *
 * @param dev I2C device descriptor
 * @param vdd Device operating voltage, volts
 * @param value Output value, volts
 * @param eeprom Store value to device EEPROM if true
 * @return `ESP_OK` on success
 */
esp_err_t mcp4728_set_voltage(i2c_dev_t *dev, float vdd, float value, bool eeprom);


esp_err_t mcp4728_fast_write(i2c_dev_t *dev, uint16_t value);
esp_err_t mcp4728_write_channel_raw(i2c_dev_t *dev,uint8_t ch, uint16_t value);


#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __MCP4728_H__ */
