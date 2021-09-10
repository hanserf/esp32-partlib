idf_component_register(
    SRCS "otaserver.c"
    INCLUDE_DIRS .
		$ENV{IDF_PATH}/components/wifi_provisioning/include/wifi_provisioning/.
		$ENV{IDF_PATH}/examples/common_components/protocol_examples_common/include/.
    EMBED_FILES ota/index.html

)
