idf_component_register(
    SRCS "src/stratum.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "../../src"
    REQUIRES mining json esp_netif
)
