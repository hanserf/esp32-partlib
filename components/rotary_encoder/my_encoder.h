#pragma once

#define ROTINC_MSB 31
#define ROTINC_LONG 30

typedef enum{
    MYENC_POS_INC,
    MYENC_POS_DEC,
    MYENC_BTN_CLICKED,
    MYENC_BTN_PUSHED,
    MYENC_BTN_RELEASED,
    MYENC_BTN_LONG
}my_encoder_event_t;

typedef void (*my_encoder_callback_t)(my_encoder_event_t event, int32_t *current);

void init_encoder(int pina,int pinb, int pin_btn,my_encoder_callback_t encoder_event_callback);
void destroy_encoder();

