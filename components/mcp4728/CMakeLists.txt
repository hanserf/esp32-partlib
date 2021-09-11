idf_component_register(
    SRCS "mcp4728.c" "my_i2cdac.c"
    INCLUDE_DIRS .
    REQUIRES i2cdev log esp_idf_lib_helpers
)
