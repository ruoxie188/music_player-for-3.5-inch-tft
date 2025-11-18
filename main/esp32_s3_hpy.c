#include "esp32_s3_hpy.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c.h"

// LCD & 触摸相关头文件
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch_ft5x06.h"

// SD 卡相关头文件
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"

// WS2812 相关头文件
#include "led_strip.h"

// 音频相关头文件
#include "es8311.h"

// LVGL Port 头文件
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "ESP32_S3_HPY_BSP";

// 全局句柄定义
esp_lcd_panel_handle_t g_lcd_panel_handle = NULL;
esp_lcd_panel_io_handle_t g_io_handle = NULL;
esp_lcd_touch_handle_t g_tp_handle = NULL;

static sdmmc_card_t *g_sd_card = NULL;
static led_strip_handle_t g_led_strip = NULL;
static i2s_chan_handle_t g_i2s_tx_handle = NULL;
static es8311_handle_t g_es8311_handle = NULL;


/*******************************************************************************
 ******************************* 显示屏 & 触摸 *********************************
 *******************************************************************************/

esp_err_t bsp_display_init(void)
{
    ESP_LOGI(TAG, "正在初始化显示屏...");

    gpio_config_t bk_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << BSP_LCD_PIN_BK_LIGHT};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
    gpio_config_t rd_gpio_config = {.mode = GPIO_MODE_OUTPUT, .pin_bit_mask = 1ULL << BSP_LCD_PIN_RD};
    ESP_ERROR_CHECK(gpio_config(&rd_gpio_config));
    ESP_ERROR_CHECK(gpio_set_level(BSP_LCD_PIN_RD, 1));

    esp_lcd_i80_bus_handle_t i80_bus = NULL;
    esp_lcd_i80_bus_config_t bus_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT, .dc_gpio_num = BSP_LCD_PIN_DC, .wr_gpio_num = BSP_LCD_PIN_PCLK,
        .data_gpio_nums = {
            BSP_LCD_PIN_DATA0, BSP_LCD_PIN_DATA1, BSP_LCD_PIN_DATA2, BSP_LCD_PIN_DATA3,
            BSP_LCD_PIN_DATA4, BSP_LCD_PIN_DATA5, BSP_LCD_PIN_DATA6, BSP_LCD_PIN_DATA7,
        },
        .bus_width = 8, .max_transfer_bytes = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT * sizeof(uint16_t),
        .psram_trans_align = 64, .sram_trans_align = 4,
    };
    ESP_ERROR_CHECK(esp_lcd_new_i80_bus(&bus_config, &i80_bus));

    esp_lcd_panel_io_i80_config_t io_config = {
        .cs_gpio_num = BSP_LCD_PIN_CS, .pclk_hz = BSP_LCD_PIXEL_CLOCK_HZ, .trans_queue_depth = 10,
        .dc_levels = {.dc_idle_level = 0, .dc_cmd_level = 0, .dc_dummy_level = 0, .dc_data_level = 1},
        .flags.swap_color_bytes = true, .lcd_cmd_bits = 8, .lcd_param_bits = 8,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i80(i80_bus, &io_config, &g_io_handle));

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BSP_LCD_PIN_RST, .color_space = ESP_LCD_COLOR_SPACE_BGR, .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(g_io_handle, &panel_config, &g_lcd_panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(g_lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(g_lcd_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(g_lcd_panel_handle, true));

    ESP_LOGI(TAG, "正在打开背光");
    gpio_set_level(BSP_LCD_PIN_BK_LIGHT, BSP_LCD_BK_LIGHT_ON_LEVEL);
    
    return ESP_OK;
}

esp_err_t bsp_touch_init(void)
{
    ESP_LOGI(TAG, "正在初始化触摸屏 (I2C_NUM_0)...");

    i2c_config_t i2c_conf_touch = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_TOUCH_I2C_SDA,
        .scl_io_num = BSP_TOUCH_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(BSP_TOUCH_I2C_NUM, &i2c_conf_touch));
    ESP_ERROR_CHECK(i2c_driver_install(BSP_TOUCH_I2C_NUM, i2c_conf_touch.mode, 0, 0, 0));

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)BSP_TOUCH_I2C_NUM, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t tp_cfg = {
        .x_max = BSP_LCD_V_RES, .y_max = BSP_LCD_H_RES,
        .rst_gpio_num = BSP_TOUCH_RST, .int_gpio_num = BSP_TOUCH_INT,
        .levels = { .reset = 0, .interrupt = 0, },
        .flags = { .swap_xy = true, .mirror_x = true, .mirror_y = false, },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &tp_cfg, &g_tp_handle));

    return ESP_OK;
}

esp_err_t bsp_lvgl_init(void)
{
    ESP_LOGI(TAG, "初始化 LVGL Port...");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));
    
    ESP_LOGI(TAG, "注册 LVGL 显示设备...");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = g_io_handle,
        .panel_handle = g_lcd_panel_handle,
        .buffer_size = BSP_LCD_H_RES * BSP_LCD_DRAW_BUF_HEIGHT,
        .double_buffer = true,
        .hres = BSP_LCD_H_RES,
        .vres = BSP_LCD_V_RES,
        .monochrome = false,
        .rotation = { .swap_xy = true, .mirror_x = false, .mirror_y = false, },
        .flags = { .buff_dma = true, }
    };
    lv_disp_t * disp = lvgl_port_add_disp(&disp_cfg);

    ESP_LOGI(TAG, "注册 LVGL 触摸设备...");
    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = g_tp_handle,
    };
    lvgl_port_add_touch(&touch_cfg);

    return ESP_OK;
}


/*******************************************************************************
 *********************************** SD 卡 ***********************************
 *******************************************************************************/
esp_err_t bsp_sdcard_init(sdmmc_card_t **card)
{
    ESP_LOGI(TAG, "正在初始化 SD 卡");
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = BSP_SD_CLK;
    slot_config.cmd = BSP_SD_CMD;
    slot_config.d0 = BSP_SD_D0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(BSP_SD_MOUNT_POINT, &host, &slot_config, &mount_config, &g_sd_card);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "挂载文件系统失败. 错误: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "SD卡成功挂载于 %s", BSP_SD_MOUNT_POINT);
    if (card) {
        *card = g_sd_card;
    }
    sdmmc_card_print_info(stdout, g_sd_card);

    return ESP_OK;
}

esp_err_t bsp_sdcard_deinit(void)
{
    if (g_sd_card) {
        esp_err_t ret = esp_vfs_fat_sdcard_unmount(BSP_SD_MOUNT_POINT, g_sd_card);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "SD卡已卸载");
            g_sd_card = NULL;
        } else {
            ESP_LOGE(TAG, "卸载SD卡失败. 错误: %s", esp_err_to_name(ret));
        }
        return ret;
    }
    return ESP_OK;
}

/*******************************************************************************
 ******************************** WS2812 彩灯 ********************************
 *******************************************************************************/
esp_err_t bsp_ws2812_init(void)
{
    ESP_LOGI(TAG, "正在初始化 WS2812");
    led_strip_config_t strip_config = {
        .strip_gpio_num = BSP_WS2812_PIN,
        .max_leds = BSP_WS2812_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = {
            .format = {
                .r_pos = 1, .g_pos = 0, .b_pos = 2, .num_components = 3,
            },
        },
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &g_led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "初始化 WS2812 灯带失败: %s", esp_err_to_name(ret));
    }
    return ret;
}

esp_err_t bsp_ws2812_set_pixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (!g_led_strip) { return ESP_ERR_INVALID_STATE; }
    return led_strip_set_pixel(g_led_strip, index, red, green, blue);
}

esp_err_t bsp_ws2812_refresh(void)
{
    if (!g_led_strip) { return ESP_ERR_INVALID_STATE; }
    return led_strip_refresh(g_led_strip);
}

esp_err_t bsp_ws2812_clear(void)
{
    if (!g_led_strip) { return ESP_ERR_INVALID_STATE; }
    return led_strip_clear(g_led_strip);
}

esp_err_t bsp_ws2812_deinit(void)
{
    if (g_led_strip) {
        esp_err_t ret = led_strip_del(g_led_strip);
        g_led_strip = NULL;
        return ret;
    }
    return ESP_OK;
}

/*******************************************************************************
 ******************************** 音频 (ES8311) ********************************
 *******************************************************************************/
esp_err_t bsp_audio_init(void)
{
    ESP_LOGI(TAG, "正在初始化音频 (I2C_NUM_1)...");

    i2c_config_t i2c_conf_audio = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = BSP_AUDIO_I2C_SDA, .scl_io_num = BSP_AUDIO_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(BSP_AUDIO_I2C_NUM, &i2c_conf_audio));
    ESP_ERROR_CHECK(i2c_driver_install(BSP_AUDIO_I2C_NUM, i2c_conf_audio.mode, 0, 0, 0));

    g_es8311_handle = es8311_create(BSP_AUDIO_I2C_NUM, ES8311_ADDRRES_0);
    if (!g_es8311_handle) return ESP_FAIL;

    const es8311_clock_config_t es_clk = {
        .mclk_inverted = false, .sclk_inverted = false,
        .mclk_from_mclk_pin = true,
        .mclk_frequency = BSP_AUDIO_MCLK_FREQ_HZ,
        .sample_frequency = BSP_AUDIO_SAMPLE_RATE
    };
    ESP_ERROR_CHECK(es8311_init(g_es8311_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
    ESP_ERROR_CHECK(es8311_voice_volume_set(g_es8311_handle, BSP_AUDIO_DEFAULT_VOLUME, NULL));
    ESP_ERROR_CHECK(es8311_microphone_config(g_es8311_handle, false));

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(BSP_AUDIO_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &g_i2s_tx_handle, NULL));
    
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(BSP_AUDIO_SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = BSP_AUDIO_I2S_MCK, .bclk = BSP_AUDIO_I2S_BCK, .ws = BSP_AUDIO_I2S_WS,
            .dout = BSP_AUDIO_I2S_DO, .din = BSP_AUDIO_I2S_DI,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false, },
        },
    };
    std_cfg.clk_cfg.mclk_multiple = BSP_AUDIO_MCLK_MULTIPLE;
    
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(g_i2s_tx_handle, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(g_i2s_tx_handle));

    return ESP_OK;
}

esp_err_t bsp_audio_play(const void *data, size_t len, size_t *bytes_written, uint32_t ticks_to_wait)
{
    if (!g_i2s_tx_handle) { return ESP_ERR_INVALID_STATE; }
    return i2s_channel_write(g_i2s_tx_handle, data, len, bytes_written, ticks_to_wait);
}

esp_err_t bsp_audio_set_volume(int volume)
{
    if (!g_es8311_handle) { return ESP_ERR_INVALID_STATE; }
    return es8311_voice_volume_set(g_es8311_handle, volume, NULL);
}

esp_err_t bsp_audio_deinit(void)
{
    if (g_i2s_tx_handle) { i2s_del_channel(g_i2s_tx_handle); g_i2s_tx_handle = NULL; }
    if (g_es8311_handle) { es8311_delete(g_es8311_handle); g_es8311_handle = NULL; }
    return ESP_OK;
}

i2s_chan_handle_t bsp_get_i2s_tx_handle(void)
{
    return g_i2s_tx_handle;
}
