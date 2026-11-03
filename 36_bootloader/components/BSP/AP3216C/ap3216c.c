/**
 ****************************************************************************************************
 * @file        ap3216c.c
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

#define __IIC_VERSION__  "1.0"

#include "ap3216c.h"


i2c_obj_t ap3216c_master;

/**
 * @brief       读取ap3216c的16位IO值
 * @param       data:存储区
 * @param       reg :寄存器
 * @retval      ESP_OK:读取成功；其他:读取失败
 */
static esp_err_t ap3216c_read_one_byte(uint8_t* data, uint8_t reg)
{
    uint8_t reg_buf[1];

    reg_buf[0] = reg;

    i2c_buf_t buf[2] = {
        {.len = 1, .buf = reg_buf},
        {.len = 1, .buf = data},
    };

    i2c_transfer(&ap3216c_master, AP3216C_ADDR, 2, buf, I2C_FLAG_WRITE | I2C_FLAG_READ | I2C_FLAG_STOP);
    return ESP_OK;
}

/**
 * @brief       向ap3216c写入16位IO值
 * @param       data:要写入的数据
 * @retval      ESP_OK:读取成功；其他:读取失败
 */
static esp_err_t ap3216c_write_one_byte(uint8_t reg, uint8_t data)
{
    uint8_t reg_buf[1];

    reg_buf[0] = reg;
    
    i2c_buf_t buf[2] = {
        {.len = 1, .buf = reg_buf},
        {.len = 1, .buf = &data},
    };

    i2c_transfer(&ap3216c_master, AP3216C_ADDR, 2, buf, I2C_FLAG_STOP); /* 传输传感器地址 */
    return ESP_OK;
}

/**
 * @brief       初始化AP3216C
 * @param       无
 * @retval      0, 成功;
                1, 失败;
*/
uint8_t ap3216c_comfig(void)
{
    uint8_t temp;
    
    ap3216c_write_one_byte(0x00, 0X04); /* 复位AP3216C */
    vTaskDelay(50);                     /* AP33216C复位至少10ms */
    ap3216c_write_one_byte(0x00, 0X03); /* 开启ALS、PS+IR */
    ap3216c_read_one_byte(&temp,0X00);  /* 读取刚刚写进去的0X03 */

    if (temp == 0X03)
    {
        ESP_LOGI("AP3216C", "AP3216C Success!!!");
        return 0;                       /* AP3216C正常 */
    }
    else
    {
        ESP_LOGE("AP3216C", "AP3216C Fail!!!");
        return 1;                       /* AP3216C失败 */
    }
} 

/**
 * @brief       读取AP3216C的数据
 * @note        读取原始数据，包括ALS,PS和IR
 *              如果同时打开ALS,IR+PS的话两次数据读取的时间间隔要大于112.5ms
 * @param       ir  :IR传感器值指针
 * @param       ps  :PS传感器值指针
 * @param       als :ALS传感器值指针
 * @retval      无
 */
void ap3216c_read_data(uint16_t *ir, uint16_t *ps, uint16_t *als)
{
    uint8_t buf[6] = {0};
    uint8_t i;

    for (i = 0; i < 6; i++)
    {
        ap3216c_read_one_byte(&buf[i],0X0A + i);                    /* 循环读取所有传感器数据 */
    }

    if (buf[0] & 0X80)
    {
        *ir = 0;                                                    /* IR_OF位为1,则数据无效 */
    }
    else 
    {
        *ir = ((uint16_t)buf[1] << 2) | (buf[0] & 0X03);            /* 读取IR传感器的数据   */
    }

    *als = ((uint16_t)buf[3] << 8) | buf[2];                        /* 读取ALS传感器的数据   */ 

    if (buf[4] & 0x40) 
    {
        *ps = 0;                                                    /* IR_OF位为1,则数据无效 */
    }
    else
    {
        *ps = ((uint16_t)(buf[5] & 0X3F) << 4) | (buf[4] & 0X0F);   /* 读取PS传感器的数据 */
    }
}

/**
 * @brief       初始化AP3216C
 * @param       无
 * @retval      无
 */
void ap3216c_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        iic_init(I2C_NUM_0);    /* 初始化IIC */
    }

    ap3216c_master = self;

    while (ap3216c_comfig())    /* 检测不到AP3216C */
    {
        ESP_LOGE("AP3216C", "AP3216C init Fail!!!");
        vTaskDelay(500);
    }
}
