/**
 ****************************************************************************************************
 * @file        i2s.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-1
 * @brief       I2S驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "i2s.h"


/* I2S默认配置 */
#define I2S_CONFIG_DEFAULT() { \
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),      \
    .sample_rate = SAMPLE_RATE,                                             \
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,                           \
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           \
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,                      \
    .intr_alloc_flags = 0,                                                  \
    .dma_buf_count = 8,                                                     \
    .dma_buf_len = 256,                                                     \
    .use_apll = false                                                       \
}

/**
 * @brief       初始化I2S
 * @param       无
 * @retval      ESP_OK:初始化成功;其他:失败
 */
esp_err_t i2s_init(void)
{
    esp_err_t ret_val = ESP_OK;

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO,
        .mck_io_num = IS2_MCLK_IO,
    };
    
    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();
    i2s_config.sample_rate = SAMPLE_RATE;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.use_apll = true;
    ret_val |= i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(I2S_NUM, &pin_config);
    ret_val |= i2s_zero_dma_buffer(I2S_NUM);
    return ret_val;
}

/**
 * @brief       I2S TRX启动
 * @param       无
 * @retval      无
 */
void i2s_trx_start(void)
{
    i2s_start(I2S_NUM);
}

/**
 * @brief       I2S TRX停止
 * @param       无
 * @retval      无
 */
void i2s_trx_stop(void)
{
    i2s_stop(I2S_NUM);
}

/**
 * @brief       I2S卸载
 * @param       无
 * @retval      无
 */
void i2s_deinit(void)
{
    i2s_driver_uninstall(I2S_NUM);
}

/**
 * @brief       设置采样率
 * @param       sampleRate  : 采样率
 * @param       bits_sample :位宽
 * @retval      无
 */
void i2s_set_samplerate_bits_sample(int samplerate,int bits_sample)
{
    i2s_set_clk(I2S_NUM,samplerate,bits_sample,I2S_CHANNEL_STEREO);
}

/**
 * @brief       I2S传输数据
 * @param       buffer: 数据存储区的首地址
 * @param       frame_size: 数据大小
 * @retval      无
 */
size_t i2s_tx_write(uint8_t *buffer, uint32_t frame_size)
{
    size_t bytes_written;
    i2s_write(I2S_NUM, buffer, frame_size, &bytes_written, 100);
    return bytes_written;
}

/**
 * @brief       I2S读取数据
 * @param       buffer: 读取数据存储区的首地址
 * @param       frame_size: 读取数据大小
 * @retval      无
 */
size_t i2s_rx_read(uint8_t *buffer, uint32_t frame_size)
{
    size_t bytes_written;
    i2s_read(I2S_NUM, buffer, frame_size, &bytes_written, 1000);
    return bytes_written;
}
