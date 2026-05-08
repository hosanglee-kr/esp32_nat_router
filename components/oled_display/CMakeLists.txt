# components/oled_display/CMakeLists.txt

if(IDF_TARGET STREQUAL "esp32c3" OR IDF_TARGET STREQUAL "esp32s3")
    set(OLED_SRCS "oled_display.c")
    # 수정: esp_driver_i2c와 esp_driver_gpio를 지우고 driver 하나로 통합
    set(OLED_REQUIRES driver nvs_flash freertos log lwip esp_netif esp_wifi)
else()
    set(OLED_SRCS "")
    set(OLED_REQUIRES "")
endif()

idf_component_register(
    SRCS ${OLED_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES ${OLED_REQUIRES}
)
