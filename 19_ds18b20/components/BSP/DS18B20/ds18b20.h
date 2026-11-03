/**
 ****************************************************************************************************
 * @file        ds18b20.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       DS18B20数字温度传感器驱动代码
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

#ifndef __DS18B20_H
#define __DS18B20_H

#include "driver/gpio.h" 
#include <esp_log.h>


/* 引脚定义 */
#define DS18B20_DQ_GPIO_PIN       GPIO_NUM_0

/* DS18B20引脚高低电平枚举 */
typedef enum 
{
    DS18B20_PIN_RESET = 0u,
    DS18B20_PIN_SET
}DS18B20_GPIO_PinState;

/* IO操作 */
#define DS18B20_DQ_IN     gpio_get_level(DS18B20_DQ_GPIO_PIN)   /* 数据端口输入 */

/* DS18B20端口定义 */
#define DS18B20_DQ_OUT(x) do{ x ?                                                   \
                            gpio_set_level(DS18B20_DQ_GPIO_PIN, DS18B20_PIN_SET) :  \
                            gpio_set_level(DS18B20_DQ_GPIO_PIN, DS18B20_PIN_RESET); \
                        }while(0)

/* 函数声明 */
void ds18b20_reset(void);               /* 复位DS18B20 */
uint8_t ds18b20_check(void);            /* 检测是否存在DS18B20 */
uint8_t ds18b20_read_bit(void);         /* 从DS18B20读取一个位 */
uint8_t ds18b20_read_byte(void);        /* 从DS18B20读取一个字节 */
void ds18b20_write_byte(uint8_t data);  /* 写一个字节到DS18B20 */
void ds18b20_start(void);               /* 开始温度转换 */
uint8_t ds18b20_init(void);             /* 初始化DS18B20 */
short ds18b20_get_temperature(void);    /* 获取温度 */

#endif
