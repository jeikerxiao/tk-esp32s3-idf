/**
 ****************************************************************************************************
 * @file        renc.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       红外遥控解码驱动代码
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

#ifndef __RENC_H__
#define __RENC_H__

#include <stdint.h>
#include "driver/rmt_encoder.h"


/* 红外指令组成结构体 */
typedef struct {
    uint16_t address;
    uint16_t command;
} ir_nec_scan_code_t;

/* 红外 NEC 编码器配置 */
typedef struct {
    uint32_t resolution;    /* 编码器分辨率，单位：Hz */
} ir_nec_encoder_config_t;

/* 参数声明 */
esp_err_t rmt_new_ir_nec_encoder(const ir_nec_encoder_config_t *config, rmt_encoder_handle_t *ret_encoder);


#endif
