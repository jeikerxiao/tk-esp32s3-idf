/**
 ****************************************************************************************************
 * @file        videoplay.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       视频播放器 应用代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#ifndef __VIDEOPLAYER_H
#define __VIDEOPLAYER_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "avi.h"
#include "xl9555.h"
#include "lcd.h"
#include "ff.h"
#include "esptim.h"
#include "mjpeg.h"
#include "es8388.h"
#include "driver/i2s.h"
#include "i2s.h"
#include "text.h"


/* 缓存空间相关定义 */
#define AVI_VIDEO_BUF_SIZE      (60 * 1024)

/* 声明函数 */
void video_play(void);  /* 播放视频 */
uint8_t video_seek(FIL *favi, AVI_INFO *aviinfo, uint8_t *mbuf);
#endif