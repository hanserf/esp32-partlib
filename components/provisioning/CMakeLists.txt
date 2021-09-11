set(COMPONENT_SRCS "my_provisioning.c" 
                    $ENV{IDF_PATH}/examples/common_components/qrcode/esp_qrcode_main.c 
                    $ENV{IDF_PATH}/examples/common_components/qrcode/esp_qrcode_wrapper.c
                    $ENV{IDF_PATH}/examples/common_components/qrcode/qrcodegen.c
                    )
set(COMPONENT_ADD_INCLUDEDIRS .  $ENV{IDF_PATH}/examples/common_components/qrcode/include)
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/qrcode)
set(COMPONENT_REQUIRES "nvs_flash" "wifi_provisioning" )
register_component()
