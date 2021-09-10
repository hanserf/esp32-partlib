set(COMPONENT_SRCS "my_provisioning.c")
set(COMPONENT_ADD_INCLUDEDIRS . )
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/qrcode)

register_component()
