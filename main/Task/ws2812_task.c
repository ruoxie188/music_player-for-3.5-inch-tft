#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

// 引入 BSP 头文件以使用 WS2812 驱动
#include "esp32_s3_hpy.h"
// 引入自己的头文件
#include "ws2812_task.h"

static const char *TAG = "WS2812_TASK";

/**
 * @brief HSV 颜色空间转换为 RGB 颜色空间
 *
 * @param h 色调 (0-359)
 * @param s 饱和度 (0-255)
 * @param v 亮度 (0-255)
 * @param r 指向红色分量的指针
 * @param g 指向绿色分量的指针
 * @param b 指向蓝色分量的指针
 */
static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h %= 360; // h is given on the circle of 360 degrees
    uint16_t i = h / 60;
    uint16_t f = (h % 60) * 255 / 60;
    uint8_t p = (v * (255 - s)) / 255;
    uint8_t q = (v * (255 - (s * f) / 255)) / 255;
    uint8_t t = (v * (255 - (s * (255 - f)) / 255)) / 255;

    switch (i) {
    case 0: *r = v; *g = t; *b = p; break;
    case 1: *r = q; *g = v; *b = p; break;
    case 2: *r = p; *g = v; *b = t; break;
    case 3: *r = p; *g = q; *b = v; break;
    case 4: *r = t; *g = p; *b = v; break;
    case 5: *r = v; *g = p; *b = q; break;
    default: *r = 0; *g = 0; *b = 0; break;
    }
}

/**
 * @brief WS2812 任务入口函数
 *
 * @param pvParameters 任务参数 (未使用)
 */
static void ws2812_task_entry(void *pvParameters)
{
    ESP_LOGI(TAG, "WS2812 任务已启动，开始显示彩虹效果。");

    uint16_t hue = 0; // 色调起始值
    uint8_t r, g, b;

    // 无限循环，实现动态效果
    while (1) {
        // 循环为每个 LED 设置颜色
        for (int i = 0; i < BSP_WS2812_LED_COUNT; i++) {
            // 计算当前 LED 的色调，使其产生彩虹带效果
            uint16_t led_hue = (hue + i * 360 / BSP_WS2812_LED_COUNT) % 360;
            // 将 HSV 转换为 RGB
            hsv_to_rgb(led_hue, 255, 255, &r, &g, &b);
            // 设置像素颜色
            bsp_ws2812_set_pixel(i, r, g, b);
        }

        // 刷新灯带以显示颜色
        bsp_ws2812_refresh();

        // 增加起始色调，使彩虹流动起来
        hue++;
        if (hue >= 360) {
            hue = 0;
        }

        // 延时 20 毫秒，控制动画速度，并让出 CPU 给其他任务
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * @brief 创建并启动 WS2812 任务
 */
esp_err_t ws2812_task_create(void)
{
    BaseType_t ret = xTaskCreate(
        ws2812_task_entry,      // 任务函数
        "ws2812_task",          // 任务名称
        2048,                   // 任务栈大小 (bytes)
        NULL,                   // 任务参数
        5,                      // 任务优先级
        NULL                    // 任务句柄 (不需要)
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "创建 WS2812 任务失败");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

