idf_component_register(
    SRCS "mqtt_main.c"
    INCLUDE_DIRS . $ENV{IDF_PATH}/components/wifi_provisioning/include/wifi_provisioning/. $ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include/.
    REQUIRES "nvs_flash" "esp_netif" "mqtt"
)