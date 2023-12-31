#pragma once
#include <stddef.h>
#include <stdbool.h>
typedef enum {
    /* EDIT These based on application */
    TOPIC_PUB_DIM_FACTOR,
    TOPIC_PUB_K1,
    TOPIC_SUB_K1,
    TOPIC_SUB_TIMESTAMP,
    TOPIC_SUB_DIM,
    /* DO NOT EDIT THIS LINE. THIS MUST BE LAST!*/
    TOPIC_NUM_TOPICS    
}my_mqtt_topics_t;

/* Function to register all enumerated MQTT Topics */
typedef void (*mqtt_topics_setup_function_t)();
/* Signal to indicate alive */
typedef void (*mqtt_is_up_function_t)(bool state);

/* Subscribed topic will change some system state. Register a function to do this according to this prototype */
typedef void (*mqtt_topics_sub_callback)(char * data_buf, int data_len);

void mqtt_init(mqtt_topics_setup_function_t top_level_init_fun ,mqtt_is_up_function_t isupdown_cb);

void register_mqtt_topic(my_mqtt_topics_t enum_topic, char *topic_string, int qos,mqtt_topics_sub_callback sub_cb_fun);
void publish_mqtt_topic(my_mqtt_topics_t topic,char *buffer,size_t len);
