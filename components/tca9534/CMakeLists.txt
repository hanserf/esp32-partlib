idf_component_register(
    SRCS "tca9534.c" "i2c_exp.c"
    INCLUDE_DIRS .
    REQUIRES i2cdev log esp_idf_lib_helpers
)
