/**
 ****************************************************************************************************
 * @file        oled.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       OLED驱动代码
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
 
#ifndef __OLED_H__
#define __OLED_H__

#include "iic.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "xl9555.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  OLED_PIN_RESET = 0u,
  OLED_PIN_SET
} OLED_PinState;

#define OLED_ADDR       0X3C    /* OLED地址 */
#define OLED_CMD        0x00    /* 写命令 */
#define OLED_DATA       0x40    /* 写数据 */

/* 引脚定义 */
#define OLED_SCL_PIN    4
#define OLED_SDA_PIN    5
#define OLED_D2_PIN     6
#define OLED_DC_PIN     38

/* 端口定义 */
#define OLED_DC(x)      do{ x ?                                          \
                            gpio_set_level(OLED_DC_PIN, OLED_PIN_SET) :  \
                            gpio_set_level(OLED_DC_PIN, OLED_PIN_RESET); \
                          }while(0) 

#define OLED_RST(x)     do{ x ?                                            \
                            xl9555_pin_write(OV_RESET_IO, OLED_PIN_SET) :  \
                            xl9555_pin_write(OV_RESET_IO, OLED_PIN_RESET); \
                          }while(0)

/* 函数声明 */
void oled_init(i2c_obj_t self);                                                     /* 初始化OLED */
void oled_on(void);                                                                 /* 打开OLED */
void oled_off(void);                                                                /* 关闭OLED */
void oled_clear(void);                                                              /* 清屏 */
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot);                            /* OLED画点 */
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot);        /* OLED填充区域填充 */
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode); /* 在指定位置显示一个字符,包括部分字符  */
uint32_t oled_pow(uint8_t m, uint8_t n);                                     /* 平方函数, m^n */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);  /* 显示len个数字 */
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size);           /* 显示字符串 */
void oled_refresh_gram(void);

#endif

#ifdef  __cplusplus
}

#endif /*  __cplusplus */
