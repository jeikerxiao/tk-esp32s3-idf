/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       OLED实验
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

#include "driver/gpio.h"
#include "led.h"
#include "iic.h"
#include "xl9555.h"
#include "oled.h"


i2c_obj_t i2c0_master;
i2c_obj_t i2c1_master;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    uint8_t t = 0;

    led_init();                             /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);      /* 初始化IIC0 */
    i2c1_master = iic_init(I2C_NUM_1);      /* 初始化IIC1 */
    xl9555_init(i2c0_master);               /* 初始化XL9555 */
    oled_init(i2c1_master);                 /* 初始化OLED */

    oled_show_string(0, 0, "ALIENTEK", 24);
    oled_show_string(0, 24, "0.96' OLED TEST", 16);
    oled_show_string(0, 40, "ATOM 2023/8/26", 12);
    oled_show_string(0, 52, "ASCII:", 12);
    oled_show_string(64, 52, "CODE:", 12);
    oled_refresh_gram();                    /* 更新显示到OLED */
    
    t = ' ';
    while(1)
    {
        oled_show_char(36, 52, t, 12, 1);   /* 显示ASCII字符 */
        oled_show_num(94, 52, t, 3, 12);    /* 显示ASCII字符的码值 */
        oled_refresh_gram();                /* 更新显示到OLED */
        t++;

        if (t > '~')
        {
            t = ' ';
        }

        vTaskDelay(500);
        LED_TOGGLE();                       /* LED闪烁 */
    }
}
