#pragma once
#include "my_bme280.h"
typedef enum{
    CB_INFO,
    CB_WARNING,
    CB_ERROR
}cb_log_level_t;
typedef void (*log_callback_funtion_t)(cb_log_level_t level, const char *buf);
void register_log_callback(log_callback_funtion_t log_callback);
void start_console(my_bme280e_t *inner,my_bme280e_t *outer);
void tcp_console_socket_deinit();


