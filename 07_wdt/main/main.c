/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       看门狗实验
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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "led.h"
#include "key.h"
#include "wdt.h"


/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t rets;
    
    rets = nvs_flash_init();            /* 初始化NVS */

    if (rets == ESP_ERR_NVS_NO_FREE_PAGES || rets == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rets = nvs_flash_init();
    }

    led_init();                         /* 初始化LED */
    key_init();                         /* 初始化按键 */
    wdt_init(5000, 1000000);            /* 初始化定时器 */
    LED(0);                             /* LED灯常亮 */

    while (1)
    {
        if (key_scan(0) == BOOT_PRES)   /* 如果BOOT按下则喂狗 */
        {
            restart_timer(1000000);     /* 喂狗 */
        }

        vTaskDelay(10);                 /* LED闪烁 */
    }
}
