/*
 * ESP-IDF Functions for 12-bit DAC MCP4728
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 * EDIT Serf
 * BSD Licensed as described in the file LICENSE
 */
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mcp4728.h>
#include <string.h>
#include "my_i2cdac.h"

//#define CONFIG_MCP4728TEST 1
#ifdef CONFIG_MCP4728_LIMITRANGE
    #define VDD (double)(CONFIG_MCP4728_OUTMAX/1000)
#else
    #define VDD (double)(CONFIG_MCP4728_VDD/1000)
#endif /* CONFIG_MCP4728_LIMITRANGE */
    

#define ADDR MCP4728A0_I2C_ADDR0
static i2c_dev_t dev;
static TaskHandle_t dac_task;
static void dac_test_task(void *pvParameters);

static void wait_for_eeprom(i2c_dev_t *dev)
{
    bool busy;
    while (true)
    {
        ESP_ERROR_CHECK(mcp4728_eeprom_busy(dev, &busy));
        if (!busy)
            return;
        printf("...DAC is busy, waiting...\n");
        vTaskDelay(1);
    }
}

void init_mcp4728(int sda, int scl){
    memset(&dev, 0, sizeof(i2c_dev_t));
    
    // Init device descriptor
    ESP_ERROR_CHECK(mcp4728_init_desc(&dev, 0, ADDR, sda, scl));

    mcp4728_power_mode_t pm;
    ESP_ERROR_CHECK(mcp4728_get_power_mode(&dev, true, &pm));
    if (pm != MCP4728_PM_NORMAL)
    {
        printf("DAC was sleeping... Wake up Neo!\n");
        ESP_ERROR_CHECK(mcp4728_set_power_mode(&dev, true, MCP4728_PM_NORMAL));
        wait_for_eeprom(&dev);
    }
    #ifdef CONFIG_MCP4728_TEST
        xTaskCreate(dac_test_task, "dac_task", configMINIMAL_STACK_SIZE * 3, NULL, 4, dac_task);
    #endif /* CONFIG_EXAMPLE_PROV_TRANSPORT_BLE */

    
    
}

static void dac_test_task(void *pvParameters)
{
    (void)pvParameters;
    printf("Set default DAC output value to MAX...\n");
    ESP_ERROR_CHECK(mcp4728_set_raw_output(&dev, MCP4728_MAX_VALUE, false));
    wait_for_eeprom(&dev);

    printf("Now let's generate the sawtooth wave in slow manner\n");
    uint16_t i = MCP4728_MAX_VALUE;
    printf("MAX value is : %d",i);
    float vout = 0;
    while (1)
    {
        vout += 0.1;
        if (vout > VDD) vout = 0;
        //printf("Vout: %.02f\n", vout);
        //ESP_ERROR_CHECK(mcp4728_set_voltage(&dev, VDD, vout, false));
        i--;
        if(i == 0){
            i = MCP4728_MAX_VALUE;
        }
        printf("Raw DAC Value: %d\n", i);
        ESP_ERROR_CHECK(mcp4728_write_channel_raw(&dev,0, i));
        // It will be very low freq wave
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

