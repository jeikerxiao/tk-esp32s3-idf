/**
 ****************************************************************************************************
 * @file        pwm.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       PWM驱动代码
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

#ifndef __PWM_H_
#define __PWM_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "driver/gpio.h"


/* 引脚以及重要参数定义 */
#define LEDC_PWM_TIMER          LEDC_TIMER_0        /* 使用定时器0 */
#define LEDC_PWM_MODE           LEDC_LOW_SPEED_MODE /* 模式设定必须使用LEDC低速模式 */
#define LEDC_PWM_CH0_GPIO       GPIO_NUM_1          /* LED控制器通道对应GPIO */
#define LEDC_PWM_CH0_CHANNEL    LEDC_CHANNEL_0      /* LED控制器通道号 */
#define LEDC_PWM_DUTY           8000                /* 渐变的变大最终目标占空比 */
#define LEDC_PWM_FADE_TIME      3000                /* 变化时长 */

/* 函数声明 */
void pwm_init(uint8_t resolution, uint16_t freq);   /* 初始化PWM */
void pwm_set_duty(uint16_t duty);                   /* PWM占空比设置 */

#endif