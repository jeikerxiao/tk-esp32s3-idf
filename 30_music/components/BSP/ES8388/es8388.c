/**
 ****************************************************************************************************
 * @file        es8388.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       ES8388驱动代码
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

#include "es8388.h"


i2c_obj_t es8388_i2c_master;

/**
 * @brief       IIC写入函数
 * @param       slave_addr:ES8388地址
 * @param       reg_add:寄存器地址
 * @param       data:写入的数据
 * @retval      无
 */
esp_err_t es8388_write_reg(uint8_t reg_addr, uint8_t data)
{
    i2c_buf_t buf[2] = {
        {.len = 1, .buf = &reg_addr},
        {.len = 1, .buf = &data},
    };

    i2c_transfer(&es8388_i2c_master, ES8388_ADDR >> 1, 2, buf, I2C_FLAG_STOP);

    return ESP_OK;
}

/**
 * @brief       读取数据
 * @param       reg_add:寄存器地址
 * @param       p_data:读取的数据
 * @retval      无
 */
esp_err_t es8388_read_reg(uint8_t reg_addr, uint8_t *pdata)
{
    i2c_buf_t buf[2] = {
        {.len = 1, .buf = &reg_addr},
        {.len = 1, .buf = pdata},
    };

    i2c_transfer(&es8388_i2c_master, ES8388_ADDR >> 1, 2, buf, I2C_FLAG_WRITE | I2C_FLAG_READ | I2C_FLAG_STOP);
    return ESP_OK;
}

/**
 * @brief       ES8388初始化
 * @param       无
 * @retval      0,初始化正常
 *              其他,错误代码
 */
uint8_t es8388_init(i2c_obj_t self)
{
    esp_err_t ret_val = ESP_OK;

    if (self.init_flag == ESP_FAIL)
    {
        iic_init(I2C_NUM_0);        /* 初始化IIC */
    }

    es8388_i2c_master = self;

    ret_val |= es8388_write_reg(0, 0x80);       /* 软复位ES8388 */
    ret_val |= es8388_write_reg(0, 0x00);
    vTaskDelay(100);                            /* 等待复位 */

    ret_val |= es8388_write_reg(0x01, 0x58);
    ret_val |= es8388_write_reg(0x01, 0x50);
    ret_val |= es8388_write_reg(0x02, 0xF3);
    ret_val |= es8388_write_reg(0x02, 0xF0);

    ret_val |= es8388_write_reg(0x03, 0x09);    /* 麦克风偏置电源关闭 */
    ret_val |= es8388_write_reg(0x00, 0x06);    /* 使能参考 500K驱动使能 */
    ret_val |= es8388_write_reg(0x04, 0x00);    /* DAC电源管理，不打开任何通道 */
    ret_val |= es8388_write_reg(0x08, 0x00);    /* MCLK不分频 */
    ret_val |= es8388_write_reg(0x2B, 0x80);    /* DAC控制 DACLRC与ADCLRC相同 */

    ret_val |= es8388_write_reg(0x09, 0x88);    /* ADC L/R PGA增益配置为+24dB */
    ret_val |= es8388_write_reg(0x0C, 0x4C);    /* ADC	数据选择为left data = left ADC, right data = left ADC  音频数据为16bit */
    ret_val |= es8388_write_reg(0x0D, 0x02);    /* ADC配置 MCLK/采样率=256 */
    ret_val |= es8388_write_reg(0x10, 0x00);    /* ADC数字音量控制将信号衰减 L  设置为最小！！！ */
    ret_val |= es8388_write_reg(0x11, 0x00);    /* ADC数字音量控制将信号衰减 R  设置为最小！！！ */

    ret_val |= es8388_write_reg(0x17, 0x18);    /* DAC 音频数据为16bit */
    ret_val |= es8388_write_reg(0x18, 0x02);    /* DAC 配置 MCLK/采样率=256 */
    ret_val |= es8388_write_reg(0x1A, 0x00);    /* DAC数字音量控制将信号衰减 L  设置为最小！！！ */
    ret_val |= es8388_write_reg(0x1B, 0x00);    /* DAC数字音量控制将信号衰减 R  设置为最小！！！ */
    ret_val |= es8388_write_reg(0x27, 0xB8);    /* L混频器 */
    ret_val |= es8388_write_reg(0x2A, 0xB8);    /* R混频器 */
    vTaskDelay(100);

    if (ret_val != ESP_OK)
    {
        while(1)
        {
            printf("ES8388初始化失败！！！\r\n");
            vTaskDelay(500);
        }
    }
    else
    {
        printf("ES8388初始化成功！！！\r\n");
    }
    
    return 0;
}

/**
 * @brief       ES8388反初始化
 * @param       无
 * @retval      0,初始化正常
 *              其他,错误代码
 */
esp_err_t es8388_deinit(void)
{
    int res = 0;
    res = es8388_write_reg(0x02, 0xFF);  /* 复位和暂停ES8388 */
    return res;
}

/**
 * @brief       设置ES8388工作模式
 * @param       fmt : 工作模式
 *    @arg      0, 飞利浦标准I2S;
 *    @arg      1, MSB(左对齐);
 *    @arg      2, LSB(右对齐);
 *    @arg      3, PCM/DSP
 * @param       len : 数据长度
 *    @arg      0, 24bit
 *    @arg      1, 20bit
 *    @arg      2, 18bit
 *    @arg      3, 16bit
 *    @arg      4, 32bit
 * @retval      无
 */
void es8388_sai_cfg(uint8_t fmt, uint8_t len)
{
    fmt &= 0x03;
    len &= 0x07;    /* 限定范围 */
    es8388_write_reg(23, (fmt << 1) | (len << 3));  /* R23,ES8388工作模式设置 */
}

/**
 * @brief       设置耳机音量
 * @param       volume : 音量大小(0 ~ 33)
 * @retval      无
 */
void es8388_hpvol_set(uint8_t volume)
{
    if (volume > 33)
    {
        volume = 33;
    }

    es8388_write_reg(0x2E, volume);
    es8388_write_reg(0x2F, volume);
}

/**
 * @brief       设置喇叭音量
 * @param       volume : 音量大小(0 ~ 33)
 * @retval      无
 */
void es8388_spkvol_set(uint8_t volume)
{
    if (volume > 33)
    {
        volume = 33;
    }

    es8388_write_reg(0x30, volume);
    es8388_write_reg(0x31, volume);
}

/**
 * @brief       设置3D环绕声
 * @param       depth : 0 ~ 7(3D强度,0关闭,7最强)
 * @retval      无
 */
void es8388_3d_set(uint8_t depth)
{
    depth &= 0x7;       /* 限定范围 */
    es8388_write_reg(0x1D, depth << 2);    /* R7,3D环绕设置 */
}

/**
 * @brief       ES8388 DAC/ADC配置
 * @param       dacen : dac使能(1) / 关闭(0)
 * @param       adcen : adc使能(1) / 关闭(0)
 * @retval      无
 */
void es8388_adda_cfg(uint8_t dacen, uint8_t adcen)
{
    uint8_t tempreg = 0;

    tempreg |= !dacen << 0;
    tempreg |= !adcen << 1;
    tempreg |= !dacen << 2;
    tempreg |= !adcen << 3;
    es8388_write_reg(0x02, tempreg);
}

/**
 * @brief       ES8388 DAC输出通道配置
 * @param       o1en : 通道1使能(1)/禁止(0)
 * @param       o2en : 通道2使能(1)/禁止(0)
 * @retval      无
 */
void es8388_output_cfg(uint8_t o1en, uint8_t o2en)
{
    uint8_t tempreg = 0;
    tempreg |= o1en * (3 << 4);
    tempreg |= o2en * (3 << 2);
    es8388_write_reg(0x04, tempreg);
}

/**
 * @brief       ES8388 MIC增益设置(MIC PGA增益)
 * @param       gain : 0~8, 对应0~24dB  3dB/Step
 * @retval      无
 */
void es8388_mic_gain(uint8_t gain)
{
    gain &= 0x0F;
    gain |= gain << 4;
    es8388_write_reg(0x09, gain);       /* R9,左右通道PGA增益设置 */
}

/**
 * @brief       ES8388 ALC设置
 * @param       sel
 *   @arg       0,关闭ALC
 *   @arg       1,右通道ALC
 *   @arg       2,左通道ALC
 *   @arg       3,立体声ALC
 * @param       maxgain : 0~7,对应-6.5~+35.5dB
 * @param       minigain: 0~7,对应-12~+30dB 6dB/STEP
 * @retval      无
 */
void es8388_alc_ctrl(uint8_t sel, uint8_t maxgain, uint8_t mingain)
{
    uint8_t tempreg = 0;
    tempreg = sel << 6;
    tempreg |= (maxgain & 0x07) << 3;
    tempreg |= mingain & 0x07;
    es8388_write_reg(0x12, tempreg);     /* R18,ALC设置 */
}

/**
 * @brief       ES8388 ADC输出通道配置
 * @param       in : 输入通道
 *    @arg      0, 通道1输入
 *    @arg      1, 通道2输入
 * @retval      无
 */
void es8388_input_cfg(uint8_t in)
{
    es8388_write_reg(0x0A, (5 * in) << 4);   /* ADC1 输入通道选择L/R	INPUT1 */
}
