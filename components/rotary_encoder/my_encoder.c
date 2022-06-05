#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <encoder.h>
#include <esp_idf_lib_helpers.h>
#include <esp_log.h>
#include <my_encoder.h>

// Connect common encoder pin to ground

#define EV_QUEUE_LEN 5

static const char *TAG = "my_encoder";

static struct {
    my_encoder_callback_t event_callback;
    QueueHandle_t event_queue;
    rotary_encoder_t re;
    TaskHandle_t task;
    /*Store current state as local global.
    This may be changed by mqtt/rs485/etc */
    int32_t ctl_d;
    int32_t ctl_q;
    
}MY_ENC;

static inline void __set_bit(int32_t *x, int bitNum) {
    *x |= (1L << bitNum);
}
static inline void __clear_bit(int32_t *x, int bitNum) {
    *x &= ~(1 << (bitNum));
}


/*This task only runs in response to an encoder interrupt*/
void encoder_task(void *arg)
{
    
    rotary_encoder_event_t e;
    my_encoder_event_t event;
    bool valid; 
    while (1)
    {
        ESP_ERROR_CHECK((xQueueReceive(MY_ENC.event_queue, &e, portMAX_DELAY) == pdPASS)?ESP_OK:ESP_FAIL);
        valid = true;
        switch (e.type)
        {
            case RE_ET_BTN_PRESSED:
                ESP_LOGV(TAG, "Button pressed");
                event = MYENC_BTN_PUSHED;
                break;
            case RE_ET_BTN_RELEASED:
                ESP_LOGV(TAG, "Button released");
                event = MYENC_BTN_RELEASED;
                break;
            case RE_ET_BTN_CLICKED:
                ESP_LOGV(TAG, "Button clicked");
                event = MYENC_BTN_CLICKED;
                break;
            case RE_ET_BTN_LONG_PRESSED:
                ESP_LOGV(TAG, "Looooong pressed button");
                event = MYENC_BTN_LONG;
                break;
            case RE_ET_CHANGED:
                MY_ENC.ctl_d += e.diff;
                if(MY_ENC.ctl_d>MY_ENC.ctl_q){
                    event = MYENC_POS_INC;
                }
                else{
                    event = MYENC_POS_DEC;
                }
                ESP_LOGV(TAG, "Value = %d", MY_ENC.ctl_d);
                break;
            default:
                valid = false;
                break;
        }
        if(valid){
            if(MY_ENC.event_callback != NULL){
                MY_ENC.event_callback(event,&MY_ENC.ctl_d);
            }
        }
        MY_ENC.ctl_q = MY_ENC.ctl_d;
    }
}

void init_encoder(int pina,int pinb, int pin_btn,my_encoder_callback_t encoder_event_callback)
{
    static StaticTask_t tcb;
    static StackType_t stack[ configMINIMAL_STACK_SIZE * 4 ];
    const uint32_t stack_size = ( sizeof( stack ) / sizeof( stack[ 0 ] ) );
    // Create event queue for rotary encoders
    if(MY_ENC.event_queue == NULL){
        MY_ENC.event_queue = xQueueCreate(EV_QUEUE_LEN, sizeof(rotary_encoder_event_t));
        ESP_ERROR_CHECK((MY_ENC.event_queue != NULL)? ESP_OK:ESP_FAIL);
        ESP_ERROR_CHECK(rotary_encoder_init(MY_ENC.event_queue));
    }
    // Setup rotary encoder library
    // Add one encoder
    memset(&MY_ENC.re, 0, sizeof(rotary_encoder_t));
    MY_ENC.re.pin_a = pina;
    MY_ENC.re.pin_b = pinb;
    MY_ENC.re.pin_btn = pin_btn;
    MY_ENC.ctl_d = 0;
    MY_ENC.ctl_q = 0;
    ESP_ERROR_CHECK(rotary_encoder_add(&MY_ENC.re));
    MY_ENC.event_callback = encoder_event_callback;
    /*Camera task is dedicated to run on core #1, it has priority 5 */
    MY_ENC.task = xTaskCreateStaticPinnedToCore(& encoder_task, (const char *) TAG, stack_size, NULL, 5, stack, &tcb,0);
    ESP_ERROR_CHECK((MY_ENC.task != NULL) ? ESP_OK : ESP_FAIL);
}

void destroy_encoder(){
    vTaskDelete(MY_ENC.task);
    ESP_ERROR_CHECK(rotary_encoder_remove(&MY_ENC.re));
}

void my_encoder_set_current_encreg(int32_t pos){
    MY_ENC.ctl_d = pos;
    MY_ENC.ctl_q = pos;
}