#ifndef ESP32_S3_HPY_H
#define ESP32_S3_HPY_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "sdmmc_cmd.h"
#include "driver/i2s_std.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 ****************************** BSP 通用宏定义 *********************************
 *******************************************************************************/
#define BSP_ERROR_CHECK_RETURN_ERR(x)    ESP_ERROR_CHECK(x)
#define BSP_ERROR_CHECK_RETURN_NULL(x)   do { if ((x) != ESP_OK) return NULL; } while(0)


/*******************************************************************************
 ******************************* 显示屏 & 触摸 *********************************
 *******************************************************************************/
// -- 显示屏引脚和参数 --
#define BSP_LCD_PIXEL_CLOCK_HZ      (10 * 1000 * 1000)
#define BSP_LCD_H_RES               (480)
#define BSP_LCD_V_RES               (320)
#define BSP_LCD_DRAW_BUF_HEIGHT     (50)
#define BSP_LCD_BK_LIGHT_ON_LEVEL   (1)

#define BSP_LCD_PIN_DATA0           (15)
#define BSP_LCD_PIN_DATA1           (16)
#define BSP_LCD_PIN_DATA2           (17)
#define BSP_LCD_PIN_DATA3           (18)
#define BSP_LCD_PIN_DATA4           (21)
#define BSP_LCD_PIN_DATA5           (40)
#define BSP_LCD_PIN_DATA6           (39)
#define BSP_LCD_PIN_DATA7           (38)

#define BSP_LCD_PIN_PCLK            (42)
#define BSP_LCD_PIN_CS              (46)
#define BSP_LCD_PIN_DC              (45)
#define BSP_LCD_PIN_RST             (-1)
#define BSP_LCD_PIN_BK_LIGHT        (47)
#define BSP_LCD_PIN_RD              (41)

// -- 触摸屏引脚 --
#define BSP_TOUCH_I2C_NUM           (I2C_NUM_0)
#define BSP_TOUCH_I2C_SCL           (GPIO_NUM_2)
#define BSP_TOUCH_I2C_SDA           (GPIO_NUM_1)
#define BSP_TOUCH_INT               (GPIO_NUM_3)
#define BSP_TOUCH_RST               (-1)

// -- 公共句柄 --
extern esp_lcd_panel_handle_t g_lcd_panel_handle;
extern esp_lcd_panel_io_handle_t g_io_handle;
extern esp_lcd_touch_handle_t g_tp_handle;

/**
 * @brief 初始化显示屏 (不包含触摸)
 */
esp_err_t bsp_display_init(void);

/**
 * @brief 初始化触摸屏
 */
esp_err_t bsp_touch_init(void);

/**
 * @brief 初始化LVGL库，并注册显示和触摸驱动
 * @note  必须在 bsp_display_init() 和 bsp_touch_init() 之后调用
 */
esp_err_t bsp_lvgl_init(void);


/*******************************************************************************
 *********************************** SD 卡 ***********************************
 *******************************************************************************/
#define BSP_SD_MOUNT_POINT      "/sdcard"
#define BSP_SD_CLK              (GPIO_NUM_5)
#define BSP_SD_CMD              (GPIO_NUM_4)
#define BSP_SD_D0               (GPIO_NUM_6)

esp_err_t bsp_sdcard_init(sdmmc_card_t **card);
esp_err_t bsp_sdcard_deinit(void);


/*******************************************************************************
 ******************************** WS2812 彩灯 ********************************
 *******************************************************************************/
#define BSP_WS2812_PIN          (GPIO_NUM_48)
#define BSP_WS2812_LED_COUNT    (8)

esp_err_t bsp_ws2812_init(void);
esp_err_t bsp_ws2812_set_pixel(uint32_t index, uint8_t red, uint8_t green, uint8_t blue);
esp_err_t bsp_ws2812_refresh(void);
esp_err_t bsp_ws2812_clear(void);
esp_err_t bsp_ws2812_deinit(void);


/*******************************************************************************
 ******************************** 音频 (ES8311) ********************************
 *******************************************************************************/
#define BSP_AUDIO_I2C_NUM           (I2C_NUM_1)
#define BSP_AUDIO_I2C_SCL           (GPIO_NUM_8)
#define BSP_AUDIO_I2C_SDA           (GPIO_NUM_7)

#define BSP_AUDIO_I2S_NUM           (I2S_NUM_0)
#define BSP_AUDIO_I2S_MCK           (GPIO_NUM_9)
#define BSP_AUDIO_I2S_BCK           (GPIO_NUM_13)
#define BSP_AUDIO_I2S_WS            (GPIO_NUM_11)
#define BSP_AUDIO_I2S_DO            (GPIO_NUM_10)
#define BSP_AUDIO_I2S_DI            (-1)

#define BSP_AUDIO_SAMPLE_RATE       (16000)
#define BSP_AUDIO_MCLK_MULTIPLE     (384)
#define BSP_AUDIO_MCLK_FREQ_HZ      (BSP_AUDIO_SAMPLE_RATE * BSP_AUDIO_MCLK_MULTIPLE)
#define BSP_AUDIO_DEFAULT_VOLUME    (70)

esp_err_t bsp_audio_init(void);
esp_err_t bsp_audio_play(const void *data, size_t len, size_t *bytes_written, uint32_t ticks_to_wait);
esp_err_t bsp_audio_set_volume(int volume);
esp_err_t bsp_audio_deinit(void);

/**
 * @brief 获取 I2S TX 通道句柄
 *
 * @return i2s_chan_handle_t I2S TX 通道句柄
 */
i2s_chan_handle_t bsp_get_i2s_tx_handle(void);

#ifdef __cplusplus
}
#endif

#endif // ESP32_S3_HPY_H

