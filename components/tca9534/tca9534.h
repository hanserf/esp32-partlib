/*
 * ESP-IDF Functions for 8-ch I2C IO Expander tca9534
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 * EDIT Serf
 * BSD Licensed as described in the file LICENSE
 */
#ifndef __TCA9534_H__
#define __TCA9534_H__

#include <stddef.h>
#include <i2cdev.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TCA9534_I2C_ADDR_BASE 0x20

/**
 * @brief Initialize device descriptor
 *
 * Default SCL frequency is 400kHz
 *
 * @param dev Pointer to I2C device descriptor
 * @param port I2C port number
 * @param addr I2C address (`0b0100<A2><A1><A0>`)
 * @param sda_gpio SDA GPIO
 * @param scl_gpio SCL GPIO
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_init_desc(i2c_dev_t *dev, i2c_port_t port, uint8_t addr, gpio_num_t sda_gpio, gpio_num_t scl_gpio);

/**
 * @brief Free device descriptor
 * @param dev Pointer to I2C device descriptor
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_free_desc(i2c_dev_t *dev);

/**
 * @brief Get GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param[out] mode Buffer to store mode, 0 bit for P0.0 .. 15 bit for P1.7
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_port_get_mode(i2c_dev_t *dev, uint8_t *mode);

/**
 * @brief Set GPIO pins mode
 *
 * 0 - output, 1 - input for each bit in `val`
 *
 * @param dev Pointer to device descriptor
 * @param mode Mode, 0 bit for P0.0 .. 15 bit for P1.7
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_port_set_mode(i2c_dev_t *dev, uint8_t mode);

/**
 * @brief Read GPIO port value
 *
 * @param dev Pointer to I2C device descriptor
 * @param val 16-bit GPIO port value, 0 bit for P0.0 .. 15 bit for P1.7
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_port_read(i2c_dev_t *dev, uint8_t *val);

/**
 * @brief Write value to GPIO port
 *
 * @param dev Pointer to I2C device descriptor
 * @param val GPIO port value, 0 bit for P0.0 .. 15 bit for P1.7
 * @return ESP_OK on success
 */
esp_err_t tca9534_port_write(i2c_dev_t *dev, uint8_t val);

/**
 * @brief Read GPIO pin level
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for P0.0 .. 15 for P1.7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_get_level(i2c_dev_t *dev, uint8_t pin, uint8_t *val);

/**
 * @brief Set GPIO pin level
 *
 * Pin must be set up as output
 *
 * @param dev Pointer to device descriptor
 * @param pin Pin number, 0 for P0.0 .. 15 for P1.7
 * @param[out] val `true` if pin currently in high state
 * @return `ESP_OK` on success
 */
esp_err_t tca9534_set_level(i2c_dev_t *dev, uint8_t pin, uint8_t val);

#ifdef __cplusplus
}
#endif

/**@}*/

#endif /* __tca9534_H__ */
