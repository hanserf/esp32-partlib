EXTRA_COMPONENT_DIRS = $(IDF_PATH)/examples/common_components/protocol_examples_common

COMPONENT_ADD_INCLUDEDIRS = .
COMPONENT_DEPENDS = nvs_flash esp_netif mqtt

include $(IDF_PATH)/make/project.mk
