// main/task/mp3_task.c
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <strings.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "audio_player.h"
#include "esp32_s3_hpy.h"
#include "driver/i2s_std.h"

#include "mp3_task.h"
#include "mp3_ui.h"
#include "esp_lvgl_port.h"

static const char *TAG = "MP3_LOGIC";

#define MAX_MP3_FILES 50
#define MAX_FILENAME_LEN 256

static char *playlist[MAX_MP3_FILES];
static int file_count = 0;
static int current_file_index = 0;

/* =========================== 文件扫描 (已修复) =========================== */
static void scan_mp3_files(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", dir_path);
        return;
    }
    struct dirent *entry;
    file_count = 0;
    while ((entry = readdir(dir)) != NULL && file_count < MAX_MP3_FILES) {
        if (entry->d_type == DT_REG) {
            char *dot = strrchr(entry->d_name, '.');
            if (dot && (strcasecmp(dot, ".mp3") == 0)) {
                char *filepath = malloc(MAX_FILENAME_LEN);
                if (filepath) {
                    // [已修复] 增加对 snprintf 返回值的安全检查
                    int written_len = snprintf(filepath, MAX_FILENAME_LEN, "%s/%s", dir_path, entry->d_name);
                    if (written_len < 0 || written_len >= MAX_FILENAME_LEN) {
                        ESP_LOGE(TAG, "File path too long, skipping: %s/%s", dir_path, entry->d_name);
                        free(filepath);
                        continue;
                    }
                    playlist[file_count++] = filepath;
                    ESP_LOGI(TAG, "Found MP3: %s", filepath);
                } else {
                    ESP_LOGE(TAG, "Failed to allocate memory for filepath");
                    break;
                }
            }
        }
    }
    closedir(dir);
}

/* =========================== 播放器核心函数 =========================== */
void play_music_by_index(int index) {
    if (file_count == 0 || index < 0 || index >= file_count) {
        return;
    }
    current_file_index = index;
    const char *filepath = playlist[current_file_index];

    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open: %s", filepath);
        return;
    }

    ESP_LOGI(TAG, "Playing: %s", filepath);
    audio_player_play(fp);

    char *filename_only = strrchr(filepath, '/');
    filename_only = filename_only ? filename_only + 1 : (char *)filepath;
    
    lvgl_port_lock(0);
    mp3_ui_update_filename(filename_only);
    mp3_ui_update_play_button(true);
    lvgl_port_unlock();
}

int get_current_music_index() {
    return current_file_index;
}

int get_music_file_count() {
    return file_count;
}

const char* get_music_filename_by_index(int index) {
    if (index < 0 || index >= file_count) {
        return "Invalid Index";
    }
    const char *filepath = playlist[index];
    char *filename_only = strrchr(filepath, '/');
    return filename_only ? filename_only + 1 : (char *)filepath;
}

/* =========================== 播放器硬件回调 =========================== */
static esp_err_t i2s_write_fn(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms) {
    return bsp_audio_play(audio_buffer, len, bytes_written, pdMS_TO_TICKS(timeout_ms));
}
static esp_err_t i2s_reconfig_clk_fn(uint32_t rate, uint32_t bits_per_sample, i2s_slot_mode_t ch) {
    i2s_chan_handle_t tx_handle = bsp_get_i2s_tx_handle();
    if (tx_handle == NULL) return ESP_FAIL;
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG((i2s_data_bit_width_t)bits_per_sample, ch),
        .gpio_cfg = { .mclk = BSP_AUDIO_I2S_MCK, .bclk = BSP_AUDIO_I2S_BCK, .ws = BSP_AUDIO_I2S_WS, .dout = BSP_AUDIO_I2S_DO, .din = BSP_AUDIO_I2S_DI, .invert_flags = {0} },
    };
    std_cfg.clk_cfg.mclk_multiple = BSP_AUDIO_MCLK_MULTIPLE;
    i2s_channel_disable(tx_handle);
    i2s_channel_reconfig_std_slot(tx_handle, &std_cfg.slot_cfg);
    i2s_channel_reconfig_std_clock(tx_handle, &std_cfg.clk_cfg);
    i2s_channel_enable(tx_handle);
    return ESP_OK;
}
static esp_err_t mute_fn(AUDIO_PLAYER_MUTE_SETTING setting) {
    bsp_audio_set_volume(setting == AUDIO_PLAYER_MUTE ? 0 : BSP_AUDIO_DEFAULT_VOLUME);
    return ESP_OK;
}

/* =========================== 播放器状态回调 =========================== */
static void audio_player_status_cb(audio_player_cb_ctx_t *ctx) {
    switch (ctx->audio_event) {
        case AUDIO_PLAYER_CALLBACK_EVENT_PLAYING:
            ESP_LOGI(TAG, "Event: PLAYING");
            break;
        case AUDIO_PLAYER_CALLBACK_EVENT_PAUSE:
            ESP_LOGI(TAG, "Event: PAUSE");
            lvgl_port_lock(0);
            mp3_ui_update_play_button(false);
            lvgl_port_unlock();
            break;
        case AUDIO_PLAYER_CALLBACK_EVENT_IDLE:
            ESP_LOGI(TAG, "Event: IDLE (Song Finished)");
            current_file_index++;
            if (current_file_index >= file_count) {
                current_file_index = 0;
            }
            play_music_by_index(current_file_index);
            break;
        default:
            break;
    }
}

/* =========================== 初始化函数 =========================== */
esp_err_t mp3_player_init(void) {
    ESP_LOGI(TAG, "Initializing Audio Player...");
    audio_player_config_t player_config = {
        .write_fn = i2s_write_fn,
        .clk_set_fn = i2s_reconfig_clk_fn,
        .mute_fn = mute_fn,
        .priority = 5,
        .coreID = 0,
    };
    ESP_ERROR_CHECK(audio_player_new(player_config));
    ESP_ERROR_CHECK(audio_player_callback_register(audio_player_status_cb, NULL));

    ESP_LOGI(TAG, "Scanning SD card for MP3 files...");
    scan_mp3_files(BSP_SD_MOUNT_POINT);

    ESP_LOGI(TAG, "Initializing MP3 Player UI...");
    lvgl_port_lock(0);
    mp3_ui_init();
    lvgl_port_unlock();
    
    lvgl_port_lock(0);
    if (file_count > 0) {
        mp3_ui_update_filename(get_music_filename_by_index(0));
    } else {
        mp3_ui_update_filename("No MP3 files found!");
    }
    lvgl_port_unlock();
    
    return ESP_OK;
}

