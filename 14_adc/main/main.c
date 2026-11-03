/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       ADC实验
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
#include "iic.h"
#include "xl9555.h"
#include "lcd.h"
#include "adc1.h"


i2c_obj_t i2c0_master;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    uint16_t adcdata;
    float voltage;
    esp_err_t ret;
    
    
    ret = nvs_flash_init();                                     /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                                 /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);                          /* 初始化IIC0 */
    spi2_init();                                                /* 初始化SPI2 */
    xl9555_init(i2c0_master);                                   /* 初始化XL9555 */
    lcd_init();                                                 /* 初始化LCD */
    adc_init();                                                 /* 初始化ADC */

    lcd_show_string(10, 50, 200, 16, 16, "ESP32", RED);
    lcd_show_string(10, 70, 200, 16, 16, "ADC TEST", RED);
    lcd_show_string(10, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);
    
    lcd_show_string(10, 110, 200, 16, 16, "ADC1_CH7_VAL:0", BLUE);
    lcd_show_string(10, 130, 200, 16, 16, "ADC1_CH7_VOL:0.000V", BLUE);
    
    while(1)
    {
        adcdata = adc_get_result_average(ADC_ADCX_CHY, 20);
        lcd_show_xnum(114, 110, adcdata, 5, 16, 0, BLUE);       /* 显示ADC采样后的原始值 */

        voltage = (float)adcdata * (3.3 / 4095);                /* 获取计算后的带小数的实际电压值 */
        adcdata = voltage;                                      /* 赋值整数部分给adcx变量 */
        lcd_show_xnum(114, 130, adcdata, 1, 16, 0, BLUE);       /* 显示电压值的整数部分 */
        
        voltage -= adcdata;                                     /* 把已经显示的整数部分去掉，留下小数部分 */
        voltage *= 1000;                                        /* 小数部分乘以1000 */
        lcd_show_xnum(130, 130, voltage, 3, 16, 0x80, BLUE);    /* 显示小数部分 */
    
        LED_TOGGLE();                                           /* LED翻转函数 */
        vTaskDelay(100);
    }
}
