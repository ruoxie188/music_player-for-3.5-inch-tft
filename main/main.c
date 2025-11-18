// main/main.c
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "ws2812_task.h"
#include "esp32_s3_hpy.h"
#include "mp3_task.h"

static const char *TAG = "MAIN_APP";

void app_main(void)
{
    ESP_LOGI(TAG, "===== 启动 BSP 硬件初始化 =====");
    ESP_ERROR_CHECK(bsp_display_init());
    ESP_ERROR_CHECK(bsp_touch_init());
    ESP_ERROR_CHECK(bsp_sdcard_init(NULL));
    ESP_ERROR_CHECK(bsp_audio_init());
    ESP_ERROR_CHECK(bsp_ws2812_init());

    ESP_LOGI(TAG, "===== 初始化 LVGL =====");
    ESP_ERROR_CHECK(bsp_lvgl_init());

    ESP_LOGI(TAG, "===== 启动 MP3 播放器 =====");
    ESP_ERROR_CHECK(mp3_player_init());
    
    ESP_LOGI(TAG, "正在启用WS2812灯带特效...");
    ESP_ERROR_CHECK(ws2812_task_create());

    ESP_LOGI(TAG, "===== 初始化完成 =====");
}

