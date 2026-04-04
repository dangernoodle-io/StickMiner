idf_component_register(
    SRCS "src/asic_task.c" "src/crc.c" "src/pll.c" "src/bm1370.c"
    INCLUDE_DIRS "include"
    REQUIRES mining freertos driver esp_timer
)
