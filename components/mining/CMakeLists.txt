idf_component_register(
    SRCS "src/mining.c" "src/sha256.c" "src/work.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer
)
