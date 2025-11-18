// main/task/mp3_ui.h
#ifndef MP3_UI_H
#define MP3_UI_H

#include "lvgl.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化MP3播放器的LVGL用户界面
 */
void mp3_ui_init(void);

/**
 * @brief 更新播放/暂停按钮的图标
 * @note 此函数不是线程安全的，调用前必须使用 lvgl_port_lock()
 * @param is_playing true显示暂停图标, false显示播放图标
 */
void mp3_ui_update_play_button(bool is_playing);

/**
 * @brief 更新显示的文件名
 * @note 此函数不是线程安全的，调用前必须使用 lvgl_port_lock()
 * @param filename 要显示的音乐文件名
 */
void mp3_ui_update_filename(const char *filename);


#ifdef __cplusplus
}
#endif

#endif // MP3_UI_H

