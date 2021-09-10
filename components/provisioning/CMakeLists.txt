idf_component_register(
    SRCS "my_provisioning.c"
    INCLUDE_DIRS . 
    REQUIRES i2cdev log esp_idf_lib_helpers qrcode
)
