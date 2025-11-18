// main/task/mp3_task.h
#ifndef MP3_TASK_H
#define MP3_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化MP3播放器后端逻辑和UI界面
 */
esp_err_t mp3_player_init(void);

#ifdef __cplusplus
}
#endif

#endif // MP3_TASK_H

