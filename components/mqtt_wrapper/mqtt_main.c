/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <protocol_examples_common.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <esp_netif.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_main.h"

#define TOPIC_MAX_LEN (128)
static const char *TAG = "MQTT_MAIN";

TaskHandle_t mqtt_pub;

typedef struct {
    int qos;
    char topic[TOPIC_MAX_LEN];
    mqtt_topics_sub_callback sub_callback;
}mqtt_topic_t;


typedef struct {
    SemaphoreHandle_t mutex;
    mqtt_topic_t mqtt_node;
}mqtt_node_t;

static struct {
    bool running;
    esp_mqtt_client_handle_t client;
    mqtt_is_up_function_t if_updown;
    mqtt_node_t my_endpoints[TOPIC_NUM_TOPICS];
}my_mqtt;


mqtt_topics_setup_function_t do_mqtt_init_from_top;

static void parse_mqtt_topic(esp_mqtt_event_handle_t event);

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    ESP_ERROR_CHECK((my_mqtt.client == event->client)?ESP_OK : ESP_FAIL);
    int i;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        my_mqtt.running = true;
        if(my_mqtt.if_updown != NULL){
            my_mqtt.if_updown(true);
        
        } 
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        if(do_mqtt_init_from_top != NULL){
            do_mqtt_init_from_top();
        }
        else{
            ESP_LOGW(TAG,"No top level init function defined....");
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        my_mqtt.running = false;
        if(my_mqtt.if_updown != NULL){
            my_mqtt.if_updown(false);
        
        } 
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        for(i=0;i<TOPIC_NUM_TOPICS;i++){
            if(my_mqtt.my_endpoints[i].mqtt_node.sub_callback != NULL){
                ESP_LOGI(TAG,"SUBSCRIBING TO TOPIC [%s]",my_mqtt.my_endpoints[i].mqtt_node.topic);
                esp_mqtt_client_unsubscribe(my_mqtt.client,my_mqtt.my_endpoints[i].mqtt_node.topic);
            }
        }
        do_mqtt_init_from_top=NULL;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, TOPIC=%.*s", event->msg_id, event->topic_len, event->topic);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d, TOPIC=%.*s", event->msg_id, event->topic_len, event->topic);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d, TOPIC=%.*s", event->msg_id,event->topic_len, event->topic);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        parse_mqtt_topic(event);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


static void parse_mqtt_topic(esp_mqtt_event_handle_t event){
    int i = 0;
    ESP_LOGD(TAG,"Parsing Received : [%s]",event->topic);
    for(i=0;i<TOPIC_NUM_TOPICS;i++){
        /* Match current topic to one of our registered topics */
        ESP_LOGD(TAG,"Against known topics : [%s]",my_mqtt.my_endpoints[i].mqtt_node.topic);
        if (!strncmp(event->topic, my_mqtt.my_endpoints[i].mqtt_node.topic, event->topic_len)){
            /*HIT! Is this something we subscribe to?*/
            if(my_mqtt.my_endpoints[i].mqtt_node.sub_callback != NULL){
                ESP_ERROR_CHECK((xSemaphoreTake(my_mqtt.my_endpoints[i].mutex,portMAX_DELAY) == pdPASS) ? ESP_OK : ESP_FAIL);
                my_mqtt.my_endpoints[i].mqtt_node.sub_callback(event->data,event->data_len);
                ESP_ERROR_CHECK((xSemaphoreGive(my_mqtt.my_endpoints[i].mutex) == pdPASS) ? ESP_OK : ESP_FAIL);
            }
            
        }
    }
}

static void mqtt_app_start(void)
{
    int i = 0;
    my_mqtt.running = false;
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URL,
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKinit_mqtt_publisherER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
    my_mqtt.client = client;
    my_mqtt.running = false;
    for(i = 0; i < TOPIC_NUM_TOPICS;i++){
        my_mqtt.my_endpoints[i].mutex = xSemaphoreCreateMutex();    
        ESP_ERROR_CHECK((my_mqtt.my_endpoints[i].mutex != NULL)? ESP_OK: ESP_OK);
        my_mqtt.my_endpoints->mqtt_node.sub_callback = NULL;
    }
}

void mqtt_init(mqtt_topics_setup_function_t top_level_init_fun,mqtt_is_up_function_t is_updown_cb)
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    if(top_level_init_fun != NULL){
        do_mqtt_init_from_top = top_level_init_fun;
        ESP_ERROR_CHECK((do_mqtt_init_from_top != NULL)?ESP_OK:ESP_FAIL);
    }
    if(is_updown_cb != NULL){
        my_mqtt.if_updown = is_updown_cb;
    }
    mqtt_app_start();
}


void register_mqtt_topic(my_mqtt_topics_t enum_topic, char *topic_string, int qos,mqtt_topics_sub_callback sub_cb_fun){
    char *p;
    if(enum_topic<TOPIC_NUM_TOPICS){
        p = my_mqtt.my_endpoints[enum_topic].mqtt_node.topic;
        p += sprintf(p,"%s",topic_string);
        *p++ = 0;
        my_mqtt.my_endpoints[enum_topic].mqtt_node.qos = qos;
        if(sub_cb_fun != NULL){
            my_mqtt.my_endpoints[enum_topic].mqtt_node.sub_callback = sub_cb_fun;
            esp_mqtt_client_subscribe(my_mqtt.client,my_mqtt.my_endpoints[enum_topic].mqtt_node.topic,1);

        }
        ESP_LOGI(TAG,"Topic:[%s] registered",my_mqtt.my_endpoints[enum_topic].mqtt_node.topic);
    
    }
}


void publish_mqtt_topic(my_mqtt_topics_t topic,char *buffer,size_t len){
    char * p = my_mqtt.my_endpoints[topic].mqtt_node.topic;
    if(my_mqtt.running){
        ESP_ERROR_CHECK((topic<TOPIC_NUM_TOPICS)?ESP_OK:ESP_FAIL);
        int msg_id = 0;
        if(xSemaphoreTake(my_mqtt.my_endpoints[topic].mutex,portMAX_DELAY)==pdPASS){
            msg_id = esp_mqtt_client_publish(my_mqtt.client, p, buffer, len, my_mqtt.my_endpoints[topic].mqtt_node.qos, 0);
            ESP_LOGI(TAG, "MSG_ID = %d, MSG:%s\n",msg_id,buffer);
            ESP_ERROR_CHECK((xSemaphoreGive(my_mqtt.my_endpoints[topic].mutex)==pdPASS)?ESP_OK:ESP_FAIL);
        }
    }
}

