/**
 ****************************************************************************************************
 * @file        iwdt.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       看门狗驱动代码
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


#ifndef __WDT_H_
#define __WDT_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"


/* 参数定义 */
#define TWDT_TIMEOUT_MS         3000
#define TASK_RESET_PERIOD_MS    2000
#define MAIN_DELAY_MS           10000

/* 函数声明 */
void wdt_init(uint16_t arr, uint64_t tps);  /* 初始化独立看门狗 */
void restart_timer(uint64_t timeout);       /*  */
void IRAM_ATTR wdt_isr_handler(void *arg);

#endif
