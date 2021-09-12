#pragma once
#include "mcp4728.h"
void init_mcp4728(int sda, int scl);
void dac_write_channel(uint8_t ch,uint16_t value);