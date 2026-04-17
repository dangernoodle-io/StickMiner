idf_component_register(
    INCLUDE_DIRS "include"
    SRCS "src/n8t_stub_miner.c" "src/asic_stubs.c"
    REQUIRES freertos mining esp_driver_i2c
)
