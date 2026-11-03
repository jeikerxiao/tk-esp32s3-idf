/**
 ****************************************************************************************************
* @file        oled.c
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

#define __OLED_VERSION__  "1.0"

#include "oled.h"
#include "oledfont.h"


i2c_obj_t oled_master;

/*OLED的显存
    存放格式如下.
    [0]0 1 2 3 ... 127
    [1]0 1 2 3 ... 127
    [2]0 1 2 3 ... 127
    [3]0 1 2 3 ... 127
    [4]0 1 2 3 ... 127
    [5]0 1 2 3 ... 127
    [6]0 1 2 3 ... 127
    [7]0 1 2 3 ... 127
*/
uint8_t OLED_GRAM[128][8];

/**
 * @brief       oled IIC写数据
 * @param       data_wr：发送的数据或者命令
 * @param       size   ：发送数据的大小
 * @retval      0：发送成功；非0值：发送失败
 */
esp_err_t oled_write(uint8_t* data_wr, size_t size)
{
    i2c_buf_t bufs = {
        .len = size,
        .buf = data_wr,
    };

    i2c_transfer(&oled_master, OLED_ADDR, 1, &bufs, I2C_FLAG_STOP);
    return ESP_OK;
}

/**
 * @brief       oled 写命令
 * @param       tx_data：数据
 * @param       command：命令值
 * @retval      无
 */
void oled_write_Byte(unsigned char tx_data, unsigned char command)
{
    unsigned char data[2] = {command, tx_data};
    oled_write(data, sizeof(data));
}

/**
 * @brief       更新显存到LCD
 * @param       无
 * @retval      无
 */
void oled_refresh_gram(void)
{
    uint8_t i, n;
    
    for (i = 0; i < 8; i++)
    {  
        oled_write_Byte(0xb0 + i, OLED_CMD);     /* 设置页地址（0~7） */
        oled_write_Byte(0x00, OLED_CMD);         /* 设置显示位置—列低地址 */
        oled_write_Byte(0x10, OLED_CMD);         /* 设置显示位置—列高地址 */
        
        for (n = 0; n < 128; n++)
        {
            oled_write_Byte(OLED_GRAM[n][i], OLED_DATA);
        }
    }
}

/**
 * @brief       初始化OLED
 * @param       i2c_obj_t self: 传入的IIC初始化参数，用以判断是否已经完成IIC初始化
 * @retval      无
 */
void oled_init(i2c_obj_t self)
{
    if (self.init_flag == ESP_FAIL)
    {
        iic_init(I2C_NUM_1);                                /* 初始化IIC */
    }

    oled_master = self;
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT;                /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;       /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;  /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ULL << OLED_D2_PIN;    /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;         /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_OUTPUT;               /* 输入输出模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;       /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;  /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ULL << OLED_DC_PIN;    /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);
    
    OLED_DC(0);
    
    /* 配置复位引脚电平 */
    xl9555_pin_write(OV_RESET_IO, 1);

    /*复位OLED*/
    OLED_RST(0);
    vTaskDelay(100);
    OLED_RST(1);
    vTaskDelay(100);

    /* 初始化代码 */
    oled_write_Byte(0xAE, OLED_CMD);    /* 关闭显示 */
    oled_write_Byte(0xD5, OLED_CMD);    /* 设置时钟分频因子,震荡频率 */
    oled_write_Byte(80, OLED_CMD);      /* [3:0],分频因子;[7:4],震荡频率 */
    oled_write_Byte(0xA8, OLED_CMD);    /* 设置驱动路数 */
    oled_write_Byte(0X3F, OLED_CMD);    /* 默认0X3F(1/64) */
    oled_write_Byte(0xD3, OLED_CMD);    /* 设置显示偏移 */
    oled_write_Byte(0X00, OLED_CMD);    /* 默认为0 */

    oled_write_Byte(0x40, OLED_CMD);    /* 设置显示开始行 [5:0],行数 */

    oled_write_Byte(0x8D, OLED_CMD);    /* 电荷泵设置 */
    oled_write_Byte(0x14, OLED_CMD);    /* bit2，开启/关闭 */
    oled_write_Byte(0x20, OLED_CMD);    /* 设置内存地址模式 */
    oled_write_Byte(0x02, OLED_CMD);    /* [1:0],00，列地址模式;01，行地址模式;10,页地址模式;默认10; */
    oled_write_Byte(0xA1, OLED_CMD);    /* 段重定义设置,bit0:0,0->0;1,0->127; */
    oled_write_Byte(0xC0, OLED_CMD);    /* 设置COM扫描方向;bit3:0,普通模式;1,重定义模式 COM[N-1]->COM0;N:驱动路数 */
    oled_write_Byte(0xDA, OLED_CMD);    /* 设置COM硬件引脚配置 */
    oled_write_Byte(0x12, OLED_CMD);    /* [5:4]配置 */
            
    oled_write_Byte(0x81, OLED_CMD);    /* 对比度设置 */
    oled_write_Byte(0xEF, OLED_CMD);    /* 1~255;默认0X7F (亮度设置,越大越亮) */
    oled_write_Byte(0xD9, OLED_CMD);    /* 设置预充电周期 */
    oled_write_Byte(0xf1, OLED_CMD);    /* [3:0],PHASE 1;[7:4],PHASE 2; */
    oled_write_Byte(0xDB, OLED_CMD);    /* 设置VCOMH 电压倍率 */
    oled_write_Byte(0x30, OLED_CMD);    /* [6:4] 000,0.65*vcc;001,0.77*vcc;011,0.83*vcc; */

    oled_write_Byte(0xA4, OLED_CMD);    /* 全局显示开启;bit0:1,开启;0,关闭;(白屏/黑屏) */
    oled_write_Byte(0xA6, OLED_CMD);    /* 设置显示方式;bit0:1,反相显示;0,正常显示 */
    oled_write_Byte(0xAF, OLED_CMD);    /* 开启显示 */
    /* 打开oled */
    oled_on();
    oled_clear();
}

/**
 * @brief       打开OLED
 * @param       无
 * @retval      无
 */
void oled_on(void)
{
    oled_write_Byte(0X8D, OLED_CMD);
    oled_write_Byte(0X14, OLED_CMD);
    oled_write_Byte(0XAF, OLED_CMD);
}

/**
 * @brief       关闭OLED
 * @param       无
 * @retval      无
 */
void oled_off(void)
{
    oled_write_Byte(0X8D, OLED_CMD);
    oled_write_Byte(0X10, OLED_CMD);
    oled_write_Byte(0XAE, OLED_CMD);
}

/**
 * @brief       清屏
 * @param       无
 * @retval      无
 */
void oled_clear(void)
{
    uint8_t i,n;

    for (i = 0;i < 8;i++)
    {
        for (n = 0;n < 128;n++)
        {
            OLED_GRAM[n][i] = 0X00;
        }
    }

    oled_refresh_gram();    /* 更新显示 */
}

/**
 * @brief       OLED画点 
 * @param       x  : 0~127
 * @param       y  : 0~63
 * @param       dot: 1 填充 0,清空
 * @retval      无
 */ 
void oled_draw_point(uint8_t x, uint8_t y, uint8_t dot)
{
    uint8_t pos, bx, temp = 0;

    if (x > 127 || y > 63)
    {
        return;                     /* 超出范围了 */
    }

    pos = 7 - y / 8;                /* 计算GRAM里面的y坐标所在的字节, 每个字节可以存储8个行坐标 */
    bx = y % 8;                     /* 取余数,方便计算y在对应字节里面的位置,及行(y)位置 */
    temp = 1 << (7 - bx);           /* 高位表示高行号, 得到y对应的bit位置,将该bit先置1 */

    if(dot)                         /* 画实心点 */
    {
        OLED_GRAM[x][pos] |= temp;  
    }
    else                            /* 画空点,即不显示 */
    {
        OLED_GRAM[x][pos] &= ~ temp;
    }
}

/**
 * @brief       OLED填充区域填充 
 * @note:       注意:需要确保: x1<=x2; y1<=y2  0<=x1<=127  0<=y1<=63
 * @param       x1,y1: 起点坐标
 * @param       x2,y2: 终点坐标 
 * @param       dot: 1 填充 0,清空
 * @retval      无
 */ 
void oled_fill(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t dot)
{
    uint8_t x, y;
    
    for (x = x1; x <= x2; x++)
    {
        for (y = y1; y <= y2; y++)
        {
            oled_draw_point(x, y, dot);
        }
    }

    oled_refresh_gram();    /* 更新显示 */
}

/**
 * @brief       在指定位置显示一个字符,包括部分字符 
 * @param       x   : 0~127
 * @param       y   : 0~63
 * @param       size: 选择字体 12/16/24
 * @param       mode: 0,反白显示;1,正常显示
 * @retval      无
 */ 
void oled_show_char(uint8_t x, uint8_t y, uint8_t chr, uint8_t size, uint8_t mode)
{
    uint8_t temp, t, t1;
    uint8_t y0 = y;
    uint8_t csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';                                                /* 得到偏移后的值 */
    
    for (t = 0; t < csize; t ++)
    {   
        if (size == 12)
        {
            temp = atk_asc2_1206[chr][t];                           /* 调用1206字体 */
        }
        else if (size == 16)
        {
            temp = atk_asc2_1608[chr][t];                           /* 调用1608字体 */
        }
        else if (size == 24)
        {
            temp = atk_asc2_2412[chr][t];                           /* 调用2412字体 */
        }
        else
        {
            return;                                                 /* 没有的字库 */
        }

        for (t1 = 0; t1 < 8; t1++)
        {
            if (temp & 0x80)
            {
                oled_draw_point(x, y, mode);
            }
            else
            {
                oled_draw_point(x, y, !mode);
            }

            temp <<= 1;
            y++;

            if ((y - y0) == size)
            {
                y = y0;
                x++;
                break;
            }
        }
    }
}

/**
 * @brief       平方函数, m^n
 * @param       m: 底数
 * @param       n: 指数 
 * @retval      无
 */
uint32_t oled_pow(uint8_t m, uint8_t n)
{
    uint32_t result = 1;

    while (n--)
    {
        result *= m;
    }

    return result;
}

/**
 * @brief       显示len个数字 
 * @param       x,y : 起始坐标
 * @param       num : 数值(0 ~ 2^32)
 * @param       len : 显示数字的位数
 * @param       size: 选择字体 12/16/24
 * @retval      无
 */
void oled_show_num(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size)
{
    uint8_t t, temp;
    uint8_t enshow = 0;
    
    for (t = 0; t < len; t++)                                       /* 按总显示位数循环 */ 
    {
        temp = (num / oled_pow(10, len - t - 1)) % 10;              /* 获取对应位的数字 */

        if (enshow == 0 && t < (len - 1))                           /* 没有使能显示,且还有位要显示 */
        {
            if (temp == 0)
            {
                oled_show_char(x + (size / 2)*t, y, ' ', size, 1);  /* 显示空格,站位 */
                continue;                                           /* 继续下个一位 */
            }
            else
            {
                enshow = 1;                                         /* 使能显示 */
            }
        }

        oled_show_char(x + (size / 2)*t, y, temp + '0', size, 1);   /* 显示字符 */
    }
} 

/**
 * @brief       显示字符串
 * @param       x,y : 起始坐标
 * @param       size: 选择字体 12/16/24
 * @param       *p  : 字符串指针,指向字符串首地址 
 * @retval      无
 */ 
void oled_show_string(uint8_t x, uint8_t y, const char *p, uint8_t size)
{
    while ((*p <= '~') && (*p >= ' '))      /* 判断是不是非法字符!*/
    {       
        if (x > (128 - (size / 2)))         /* 宽度越界 */
        {
            x = 0;
            y += size;                      /* 换行 */
        }
        if (y > (64 - size))                /* 高度越界 */
        {
            y = x = 0;
            oled_clear();
        }

        oled_show_char(x, y, *p, size, 1);  /* 显示一个字符 */
        x += size / 2;                      /* ASCII字符宽度为汉字宽度的一半 */
        p++;
    }
}
