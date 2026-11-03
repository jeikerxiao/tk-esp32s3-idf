/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       QMA6100P三轴加速度计传感器实验
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

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "xl9555.h"
#include "lcd.h"
#include "qma6100p.h"


i2c_obj_t i2c0_master;

/**
 * @brief       显示原始数据
 * @param       x, y : 坐标
 * @param       title: 标题
 * @param       val  : 值
 * @retval      无
 */
void user_show_mag(uint16_t x, uint16_t y, char *title, float val)
{
    char buf[20];

    sprintf(buf,"%s%3.1f", title, val);                 /* 格式化输出 */
    lcd_fill(x + 30, y + 16, x + 160, y + 16, WHITE);   /* 清除上次数据(最多显示20个字符,20*8=160) */
    lcd_show_string(x, y, 160, 16, 16, buf, BLUE);      /* 显示字符串 */
}

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    uint8_t t;
    qma6100p_rawdata_t xyz_rawdata;
    esp_err_t ret;
    
    ret = nvs_flash_init();             /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                         /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);  /* 初始化IIC0 */
    spi2_init();                        /* 初始化SPI2 */
    xl9555_init(i2c0_master);           /* 初始化XL9555 */
    lcd_init();                         /* 初始化LCD */
    qma6100p_init(i2c0_master);         /* 初始化三轴加速度计 */
    
    lcd_show_string(30, 50, 200, 16, 16, "ESP32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "QMA6100P TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    lcd_show_string(30, 110, 200, 16, 16, " ACC_X :", RED);
    lcd_show_string(30, 130, 200, 16, 16, " ACC_Y :", RED);
    lcd_show_string(30, 150, 200, 16, 16, " ACC_Z :", RED);
    lcd_show_string(30, 170, 200, 16, 16, " Pitch :", RED);
    lcd_show_string(30, 190, 200, 16, 16, " Roll  :", RED);

    while (1)
    {
        vTaskDelay(10);
        t++;

        if (t == 20)                    /* 0.2秒左右更新一次三轴原始值 */
        {   
            qma6100p_read_rawdata(&xyz_rawdata);
            
            user_show_mag(30, 110, "ACC_X :", xyz_rawdata.acc_x);
            user_show_mag(30, 130, "ACC_Y :", xyz_rawdata.acc_y);
            user_show_mag(30, 150, "ACC_Z :", xyz_rawdata.acc_z);
            user_show_mag(30, 170, "Pitch :", xyz_rawdata.pitch);
            user_show_mag(30, 190, "Roll  :", xyz_rawdata.roll);
            
            t = 0;
            LED_TOGGLE();
        }
    }
}
