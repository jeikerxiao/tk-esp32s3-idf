/**
 ****************************************************************************************************
 * @file        i2s.h
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

#ifndef _I2S_H
#define _I2S_H

#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "esp_err.h"
#include "esp_log.h"
#include "es8388.h"
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"


#define I2S_NUM                 (I2S_NUM_0)                 /* I2S端口 */
#define I2S_BCK_IO              (GPIO_NUM_46)               /* 设置串行时钟引脚，ES8388_SCLK */
#define I2S_WS_IO               (GPIO_NUM_9)                /* 设置左右声道的时钟引脚，ES8388_LRCK */
#define I2S_DO_IO               (GPIO_NUM_10)               /* ES8388_SDOUT */
#define I2S_DI_IO               (GPIO_NUM_14)               /* ES8388_SDIN */
#define IS2_MCLK_IO             (GPIO_NUM_3)                /* ES8388_MCLK */
#define SAMPLE_RATE             (44100)                     /* 采样率 */

/* 声明函数 */
esp_err_t i2s_init(void);                                           /* I2S初始化 */
void i2s_trx_start(void);                                           /* 启动I2S */
void i2s_trx_stop(void);                                            /* 停止I2S */
void i2s_deinit(void);                                              /* 卸载I2S */
void i2s_set_samplerate_bits_sample(int samplerate,int bits_sample);/* 设置采样率及位宽 */
size_t i2s_tx_write(uint8_t *buffer, uint32_t frame_size);          /* 写数据 */
size_t i2s_rx_read(uint8_t *buffer, uint32_t frame_size);           /* 读数据 */

#endif
