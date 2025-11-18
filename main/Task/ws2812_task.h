#ifndef WS2812_TASK_H
#define WS2812_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 创建并启动 WS2812 彩灯效果任务
 *
 * @return
 *      - ESP_OK: 任务创建成功
 *      - 其他: 任务创建失败
 */
esp_err_t ws2812_task_create(void);

#ifdef __cplusplus
}
#endif

#endif // WS2812_TASK_H
