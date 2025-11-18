// main/task/mp3_ui.c
#include "mp3_ui.h"
#include "audio_player.h"
#include "esp32_s3_hpy.h"  // [新增] 引入BSP头文件以获取默认音量

// 引入后端逻辑函数
extern void play_music_by_index(int index);
extern int get_current_music_index();
extern int get_music_file_count();

// [新增] 引入BSP的音量设置函数
extern esp_err_t bsp_audio_set_volume(int volume);

// UI组件句柄
static lv_obj_t *play_label;
static lv_obj_t *file_label;

// [新增] 全局变量，用于保存当前音量值
static uint8_t g_current_volume = BSP_AUDIO_DEFAULT_VOLUME;


/* =========================== 事件回调函数 =========================== */

// [新增] 音量滑条的事件回调函数
static void volume_slider_event_handler(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    // 从滑条获取0-100的百分比值
    int volume_percent = lv_slider_get_value(slider);
    
    // 更新全局音量变量
    g_current_volume = (uint8_t)volume_percent;
    
    // 调用BSP函数，实际设置硬件音量
    bsp_audio_set_volume(g_current_volume);
}


// 按钮的事件回调函数 (保持不变)
static void button_event_handler(lv_event_t *e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    uintptr_t btn_id = (uintptr_t)lv_event_get_user_data(e);
    int current_index = get_current_music_index();
    int total_files = get_music_file_count();

    if (total_files == 0) return;

    if (btn_id == 0) { // 上一首
        current_index--;
        if (current_index < 0) current_index = total_files - 1;
        play_music_by_index(current_index);
    } else if (btn_id == 1) { // 播放/暂停
        audio_player_state_t state = audio_player_get_state();
        if (state == AUDIO_PLAYER_STATE_PLAYING) {
            audio_player_pause();
        } else if (state == AUDIO_PLAYER_STATE_PAUSE) {
            audio_player_resume();
        } else {
            play_music_by_index(current_index);
        }
    } else if (btn_id == 2) { // 下一首
        current_index++;
        if (current_index >= total_files) current_index = 0;
        play_music_by_index(current_index);
    }
}


/* =========================== UI 更新函数 =========================== */

void mp3_ui_update_play_button(bool is_playing)
{
    if (play_label) {
        lv_label_set_text(play_label, is_playing ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

void mp3_ui_update_filename(const char *filename)
{
    if (file_label && filename) {
        lv_label_set_text(file_label, filename);
    }
}


/* =========================== UI 界面初始化 (已修改) =========================== */
void mp3_ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* 1. 创建文件名标签 (顶部) */
    file_label = lv_label_create(scr);
    lv_label_set_long_mode(file_label, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(file_label, lv_pct(80));
    lv_obj_align(file_label, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(file_label, "Starting...");
    lv_obj_set_style_text_align(file_label, LV_TEXT_ALIGN_CENTER, 0);

    /* 2. 创建播放控制按钮容器 (中部) */
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, lv_pct(100), 80);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 20); // [修改] 垂直居中，并稍微向下偏移
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);

    // 上一首按钮
    lv_obj_t *prev_btn = lv_btn_create(cont);
    lv_obj_add_event_cb(prev_btn, button_event_handler, LV_EVENT_CLICKED, (void *)0);
    lv_obj_t *prev_label = lv_label_create(prev_btn);
    lv_label_set_text(prev_label, LV_SYMBOL_PREV);
    lv_obj_center(prev_label);

    // 播放暂停按钮
    lv_obj_t *play_btn = lv_btn_create(cont);
    lv_obj_add_event_cb(play_btn, button_event_handler, LV_EVENT_CLICKED, (void *)1);
    play_label = lv_label_create(play_btn);
    lv_label_set_text(play_label, LV_SYMBOL_PLAY);
    lv_obj_center(play_label);

    // 下一首按钮
    lv_obj_t *next_btn = lv_btn_create(cont);
    lv_obj_add_event_cb(next_btn, button_event_handler, LV_EVENT_CLICKED, (void *)2);
    lv_obj_t *next_label = lv_label_create(next_btn);
    lv_label_set_text(next_label, LV_SYMBOL_NEXT);
    lv_obj_center(next_label);
    
    /* 3. [新增] 创建音量调节滑条 (底部) */
    lv_obj_t *volume_slider = lv_slider_create(scr);
    lv_obj_set_width(volume_slider, lv_pct(60)); // 宽度为屏幕的60%
    lv_obj_align(volume_slider, LV_ALIGN_BOTTOM_MID, 0, -40); // 对齐到底部中央，向上偏移40
    lv_slider_set_range(volume_slider, 0, 100); // 设置范围为 0 - 100
    lv_slider_set_value(volume_slider, g_current_volume, LV_ANIM_OFF); // 设置初始值为默认音量
    lv_obj_add_event_cb(volume_slider, volume_slider_event_handler, LV_EVENT_VALUE_CHANGED, NULL);

    // 在滑条左侧添加音量小图标
    lv_obj_t *label_vol_min = lv_label_create(scr);
    lv_label_set_text(label_vol_min, LV_SYMBOL_VOLUME_MID);
    lv_obj_align_to(label_vol_min, volume_slider, LV_ALIGN_OUT_LEFT_MID, -10, 0);

    // 在滑条右侧添加音量大图标
    lv_obj_t *label_vol_max = lv_label_create(scr);
    lv_label_set_text(label_vol_max, LV_SYMBOL_VOLUME_MAX);
    lv_obj_align_to(label_vol_max, volume_slider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

