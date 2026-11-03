/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       DHT11数字温湿度传感器实验
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
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "led.h"
#include "iic.h"
#include "spi.h"
#include "lcd.h"
#include "xl9555.h"
#include "dht11.h"


i2c_obj_t i2c0_master;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    uint8_t err;
    uint8_t t = 0;
    uint8_t temperature;
    uint8_t humidity;
    esp_err_t ret;
    
    ret = nvs_flash_init();                                         /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                                     /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);                              /* 初始化IIC0 */
    spi2_init();                                                    /* 初始化SPI2 */
    xl9555_init(i2c0_master);                                       /* 初始化XL9555 */
    lcd_init();                                                     /* 初始化LCD */

    lcd_show_string(30, 50, 200, 16, 16, "ESP32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "DHT11 TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);

    /* 初始化DHT11数字温湿度传感器 */
    err = dht11_init();
    
    if (err != 0)
    {
        while (1)
        {
            lcd_show_string(30, 110, 200, 16, 16, "DHT11 Error", RED);
            vTaskDelay(200);
            lcd_fill(30, 110, 239, 130 + 16, WHITE);
            vTaskDelay(200);
        }
    }

    lcd_show_string(30, 110, 200, 16, 16, "DHT11 OK", RED);
    lcd_show_string(30, 130, 200, 16, 16, "Temp:  C", BLUE);
    lcd_show_string(30, 150, 200, 16, 16, "Humi:  %", BLUE);
    
    while (1)
    {
        if (t % 10 == 0)                                            /* 每100ms读取一次 */
        {
            dht11_read_data(&temperature, &humidity);               /* 读取温湿度值 */
            lcd_show_num(30 + 40, 130, temperature, 2, 16, BLUE);   /* 显示温度 */
            lcd_show_num(30 + 40, 150, humidity, 2, 16, BLUE);      /* 显示湿度 */
        }

        vTaskDelay(10);
        t++;

        if (t == 20)
        {
            t = 0;
            LED_TOGGLE();                                           /* LED闪烁 */
        }
    }
}
