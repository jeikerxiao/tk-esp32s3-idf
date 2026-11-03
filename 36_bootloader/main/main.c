/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       ESP32-S3板载外设测试实验
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
#include "iic.h"
#include "xl9555.h"
#include "lvgl_demo.h"
#include "spi.h"
#include "key.h"
#include "led.h"
#include "24cxx.h"
#include "manage.h"
#include "app_test.h"
#include "adc1.h"
#include "qma6100p.h"
#include "es8388.h"
#include "i2s.h"
#include "exfuns.h"
#include "esp_rtc.h"


i2c_obj_t i2c0_master;

/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t ret;
    
    ret = nvs_flash_init();             /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    i2c0_master = iic_init(I2C_NUM_0);  /* 初始化IIC0 */
    spi2_init();                        /* 初始化SPI */
    led_init();                         /* 初始化LED */
    key_init();                         /* 初始化KEY */
    xl9555_init(i2c0_master);           /* IO扩展芯片初始化 */
    at24cxx_init(i2c0_master);          /* 初始化24CXX */
    ap3216c_init(i2c0_master);          /* 初始化AP3216C */
    qma6100p_init(i2c0_master);         /* 初始化三轴加速度计 */
    es8388_init(i2c0_master);           /* ES8388初始化 */
    rtc_set_time(2023,12,26,11,32,00);  /* 设置RTC时间 */
    lcd_init();                         /* LCD屏初始化 */
    exfuns_init();                      /* 为fatfs相关变量申请内存 */
    func_test();                        /* 板载功能测试 */
    lvgl_demo();
}
