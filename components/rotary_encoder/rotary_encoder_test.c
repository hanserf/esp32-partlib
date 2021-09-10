  
#include "my_encoder.h"
#include "stdint.h"
#include <esp_log.h>

static const char* TAG = "encoder_test";

static void my_encoder_callback(my_encoder_event_t event, int32_t current);

void init_encoder(){
  init_encoder(14,5,13,my_encoder_callback);
  while(true){
      vTaskDelay(pdMS_TO_TICKS(3000));            
}
}
   
  

/*We need to supply the "my_encoder_callback_t" function to encoder class.
This function determines what action code should take based on rotary encoder event */
static void my_encoder_callback(my_encoder_event_t event, int32_t current){
  switch (event)
  {
  case MYENC_BTN_CLICKED:

    ESP_LOGI(TAG, "Button clicked");
    
    break;
  case MYENC_BTN_LONG:
    digitalWrite(led1_pin, !digitalRead(led1_pin));
    ESP_LOGI(TAG,"BTN Held down");
    break;
  case MYENC_BTN_PUSHED:
    ESP_LOGI(TAG,"BTN Pushed down");
    break;
  case MYENC_BTN_RELEASED:
    ESP_LOGI(TAG,"BTN Released");    
    break;
  case MYENC_POS_DEC:
    ESP_LOGI(TAG,"Encoder Rotation Decrease");
    break;
  case MYENC_POS_INC:
    ESP_LOGI(TAG,"Encoder Rotation Increase");
    break;
  default:
    break;
  }

}
