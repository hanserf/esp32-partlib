/*
 * ESP-IDF Functions for 12-bit DAC MCP4728
 *
 * Ported from esp-open-rtos
 *
 * Copyright (C) 2021 Hans Erik Fjeld <hanse.fjeld@gmail.com>
 *  
 * BSD Licensed as described in the file LICENSE
 */
#include <stdio.h>
#include <tca9534.h>
#include <string.h>
#include <esp_log.h>
#define I2C_ADDR 0x38
// A0, A1, A2 pins are grounded

#define BIT_SET(a,b) ((a) |= (1ULL<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1ULL<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1ULL<<(b)))
#define BIT_CHECK(a,b) (!!((a) & (1ULL<<(b))))        // '!!' to make sure this returns 0 or 1

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK_ALL(x,y) (!(~(x) & (y)))
#define BITMASK_CHECK_ANY(x,y) ((x) & (y))

static const char *TAG = "i2c-exp";

static i2c_dev_t tca9534;
static uint8_t port_setup;
static uint8_t port_value;

static inline void SET_U8(uint8_t *reg, uint8_t *val){
    *reg = ((*reg & ~0xFF) | (*val & 0xFF));
}


void i2c_exp_init(int sda,int scl){
    uint8_t val = 0;
    memset(&tca9534, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(tca9534_init_desc(&tca9534, 0, I2C_ADDR, sda, scl));
    tca9534_port_read(&tca9534,&val);
    ESP_LOGI(TAG,"READ Ports 0x%x",val);
    /* Initialize everything as outputs */
    val = 0;
    SET_U8(&port_setup,&val);
    tca9534_port_set_mode(&tca9534,port_setup);
    /*All values off*/
    SET_U8(&port_value,&val);
    tca9534_port_write(&tca9534,port_value);
}

int i2c_exp_set_pin(int pin,int state){
    /*bit 4 is led*/
    bool ok = false;
    uint8_t enable;
    ok = (pin >= 0 && pin<=4)?true:false;
    switch (state)
    {
    case 0 :
        ok = true;
        enable = 0;
        break;
    case 1 :
        ok = true;
        enable = 1;
        break;

    default:
        ok = false;
        break;
    }
    if(!ok){
        return -1;
    }
    tca9534_set_level(&tca9534,pin,enable);
    return 0;
}



void i2c_exp_deinit(){
    tca9534_free_desc(&tca9534);
}
