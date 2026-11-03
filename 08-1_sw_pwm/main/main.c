/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       软件改变PWM占空比实验
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

#include "nvs_flash.h"
#include "pwm.h"


/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t ret;
    uint8_t dir = 1;
    uint16_t ledpwmval = 0;

    ret = nvs_flash_init(); /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    pwm_init(10, 1000);     /* 初始化PWM */

    while(1) 
    {
        vTaskDelay(10);

        if (dir == 1)
        {
            ledpwmval += 5; /* dir==1 ledpwmval递增 */
        }
        else
        {
            ledpwmval -= 5; /* dir==0 ledpwmval递减 */
        }

        if (ledpwmval > 1005)
        {
            dir = 0;        /* ledpwmval到达1005后，方向为递减 */
        }

        if (ledpwmval < 5)
        {
            dir = 1;        /* ledpwmval递减到5后，方向改为递增 */
        }

        /* 设置占空比 */
        pwm_set_duty(ledpwmval);
    }
}
