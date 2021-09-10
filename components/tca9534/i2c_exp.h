#pragma once

void i2c_exp_init(int sda,int scl);
int i2c_exp_set_pin(int pin,int state);
void i2c_exp_deinit();