/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       光环境传感器实验
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
#include "nvs_flash.h"
#include "led.h"
#include "lcd.h"
#include "spi.h"
#include "iic.h"
#include "xl9555.h"
#include "ap3216c.h"


i2c_obj_t i2c0_master;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    uint16_t ir, als, ps;
    esp_err_t ret;
    
    ret = nvs_flash_init();                             /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                         /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);                  /* 初始化IIC0 */
    spi2_init();                                        /* 初始化SPI2 */
    xl9555_init(i2c0_master);                           /* 初始化XL9555 */
    lcd_init();                                         /* 初始化LCD */
    ap3216c_init(i2c0_master);                          /* 初始化AP3216C */

    lcd_show_string(10, 50, 200, 16, 16, "ESP32", RED);
    lcd_show_string(10, 70, 200, 16, 16, "AP3216C TEST", RED);
    lcd_show_string(10, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);

    lcd_show_string(10, 130, 200, 16, 16, "AP3216C Ready!", RED);
    lcd_show_string(10, 150, 200, 16, 16, "IR :", RED);
    lcd_show_string(10, 170, 200, 16, 16, "PS :", RED);
    lcd_show_string(10, 190, 200, 16, 16, "ALS:", RED);

    while(1)
    {
        ap3216c_read_data(&ir, &ps, &als);              /* 读取数据  */
        lcd_show_num(10 + 40, 150, ir, 5, 16, BLUE);    /* 显示IR数据 */
        lcd_show_num(10 + 40, 170, ps, 5, 16, BLUE);    /* 显示PS数据 */
        lcd_show_num(10 + 40, 190, als, 5, 16, BLUE);   /* 显示ALS数据  */

        LED_TOGGLE();                                   /* 提示系统正在运行 */
        vTaskDelay(200);
    }
}
