idf_component_register(
    SRCS "encoder.c" "my_encoder.c"
    INCLUDE_DIRS .
    REQUIRES log esp_idf_lib_helpers
)
