idf_component_register(
    SRCS "my_provisioning.c"
    INCLUDE_DIRS . $ENV{IDF_PATH}/components/wifi_provisioning/include/wifi_provisioning/. $ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include/.
    EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/qrcode
    REQUIRES i2cdev log esp_idf_lib_helpers
)
