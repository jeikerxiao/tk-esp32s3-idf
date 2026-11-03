/**
 ****************************************************************************************************
 * @file        ap3216c.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       AP3216C驱动代码
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
 
#ifndef __AP3216C_H__
#define __AP3216C_H__

#include "xl9555.h"
#include "driver/gpio.h"


/* AP3216C信息 */
typedef struct _ap3216c_value_t
{
    uint16_t ir;
    uint16_t als;
    uint16_t ps;
} ap3216c_value_t;

/* 相关参数定义 */
#define AP3216C_INT     xl9555_pin_read(AP_INT_IO)
#define AP3216C_ADDR    0X1E                                        /* AP3216C地址 */

/* 函数声明 */
void ap3216c_init(i2c_obj_t self);                                  /* 初始化AP3216C */
uint8_t ap3216c_comfig(void);                                       /* 检查AP3216C */
void ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als);  /* 读取AP3216C的数据 */

#endif
