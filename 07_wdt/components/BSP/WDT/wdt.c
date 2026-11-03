/**
 ****************************************************************************************************
 * @file        wdt.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       任务看门狗驱动代码
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

#include "wdt.h"

esp_timer_handle_t esp_tim_handle;                          /* 定时器回调函数句柄 */

/**
 * @brief       初始化任务看门狗计时器
 * @param       arr: 自动重装载值
 *              tps: 定时器周期
 */
void wdt_init(uint16_t arr, uint64_t tps)
{
    /* 定义一个定时器结构体 */
    esp_timer_create_args_t tim_periodic_arg = {
    .callback =	&wdt_isr_handler,                           /* 设置回调函数 */
    .arg = NULL,                                            /* 不携带参数 */
    };

    /* 创建定时器事件 */
    esp_timer_create(&tim_periodic_arg, &esp_tim_handle);   /* 创建一个事件 */
    esp_timer_start_periodic(esp_tim_handle, tps);          /* 每周期内触发一次 */
}

/**
 * @brief       重新启动当前运行的计时器
 * @param       timeout: 定时器超时时间，该超时时间以微妙作为基本计算单位，故而设置超时时间为1s，则需要转换为微妙（μs），即timeout = 1s = 1000000μs
 * @retval      无
 */
void restart_timer(uint64_t timeout)
{
    esp_timer_restart(esp_tim_handle, timeout);             /* 重新启动当前运行的计时器，用以模拟喂狗过程 */
}

/**
 * @brief       看门狗回调函数
 * @param       arg: 无参数传入
 * @retval      无
 */
void IRAM_ATTR wdt_isr_handler(void *arg)
{
    esp_restart();                                          /* 若没有及时进行喂狗，那么芯片将一直进行复位 */
}
