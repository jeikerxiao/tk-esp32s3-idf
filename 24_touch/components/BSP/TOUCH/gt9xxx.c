/**
 ****************************************************************************************************
* @file        gt9xxx.c
* @author      正点原子团队(正点原子)
* @version     V1.1
* @date        2023-12-1
* @brief       4.3寸电容触摸屏-GT9xxx 驱动代码
*   @note      GT系列电容触摸屏IC通用驱动,本代码支持: GT9147/GT917S/GT968/GT1151/GT9271 等多种
*              驱动IC, 这些驱动IC仅ID不一样, 具体代码基本不需要做任何修改即可通过本代码直接驱动
*
* @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
****************************************************************************************************
* @attention
*
* 实验平台:正点原子 ESP32S3 开发板
* 在线视频:www.yuanzige.com
* 技术论坛:www.openedv.com
* 公司网址:www.alientek.com
* 购买地址:openedv.taobao.com
*
****************************************************************************************************
*/

#include "gt9xxx.h"


/* 注意: 除了GT9271支持10点触摸之外, 其他触摸芯片只支持 5点触摸 */
uint8_t g_gt_tnum = 5;      /* 默认支持的触摸屏点数(5点触摸) */

/**
 * @brief       向gt9xxx写入数据
 * @param       reg : 起始寄存器地址
 * @param       buf : 数据缓缓存区
 * @param       len : 写数据长度
 * @retval      esp_err_t ：0, 成功; 1, 失败;
 */
esp_err_t gt9xxx_wr_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, GT9XXX_CMD_WR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg & 0XFF, ACK_CHECK_EN);
    i2c_master_write(cmd, buf, len, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief       从gt9xxx读出数据
 * @param       reg : 起始寄存器地址
 * @param       buf : 数据缓缓存区
 * @param       len : 读数据长度
 * @retval      esp_err_t ：0, 成功; 1, 失败;
 */
esp_err_t gt9xxx_rd_reg(uint16_t reg, uint8_t *buf, uint8_t len)
{
    if (len == 0)
    {
        return ESP_OK;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, GT9XXX_CMD_WR, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg >> 8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg & 0XFF, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000); 
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, GT9XXX_CMD_RD, ACK_CHECK_EN);
    if (len > 1)
    {
        i2c_master_read(cmd, buf, len - 1, 0);
    }
    i2c_master_read_byte(cmd, buf + len - 1, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret |= i2c_master_cmd_begin(GT9XXX_IIC_PORT, cmd, 1000);
    i2c_cmd_link_delete(cmd);

    return ret;
}

/**
 * @brief       初始化gt9xxx触摸屏
 * @param       无
 * @retval      0, 初始化成功; 1, 初始化失败;
 */
uint8_t gt9xxx_init(void)
{
    uint8_t temp[5];
    gpio_config_t gpio_init_struct = {0};

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;                 /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT;                        /* 输入模式 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_DISABLE;              /* 失能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;          /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = (1ull << GT9XXX_INT_GPIO_PIN);  /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                                 /* 配置GPIO */

    i2c_config_t iic_config_struct = {0};

    iic_config_struct.mode = I2C_MODE_MASTER;                       /* 设置IIC模式-主机模式 */
    iic_config_struct.sda_io_num = GT9XXX_IIC_SDA;                  /* 设置IIC_SDA引脚 */
    iic_config_struct.scl_io_num = GT9XXX_IIC_CLK;                  /* 设置IIC_SCL引脚 */
    iic_config_struct.sda_pullup_en = GPIO_PULLUP_ENABLE;           /* 配置IIC_SDA引脚上拉使能 */
    iic_config_struct.scl_pullup_en = GPIO_PULLUP_ENABLE;           /* 配置IIC_SCL引脚上拉使能 */
    iic_config_struct.master.clk_speed = GT9XXX_IIC_FREQ;           /* 设置IIC通信频率 */
    i2c_param_config(GT9XXX_IIC_PORT, &iic_config_struct);          /* 设置IIC初始化参数 */

    /* 激活I2C控制器的驱动 */
    i2c_driver_install(GT9XXX_IIC_PORT,                             /* 端口号 */
                       iic_config_struct.mode,                      /* 主机模式 */
                       I2C_MASTER_RX_BUF_DISABLE,                   /* 从机模式下接收缓存大小(主机模式不使用) */
                       I2C_MASTER_TX_BUF_DISABLE,                   /* 从机模式下发送缓存大小(主机模式不使用) */ 
                       0);                                          /* 用于分配中断的标志(通常从机模式使用) */ 

    for (int i = 2;i > 0;i--)
    {
        CT_RST(0);
        vTaskDelay(200);
        CT_RST(1);
        vTaskDelay(200);
    }

    gt9xxx_rd_reg(GT9XXX_PID_REG, temp, 4);     /* 读取产品ID */
    temp[4] = 0;
    /* 判断一下是否是特定的触摸屏 */
    if (strcmp((char *)temp, "911") && strcmp((char *)temp, "9147") && strcmp((char *)temp, "1158") && strcmp((char *)temp, "9271"))
    {
        return 1;   /* 若不是触摸屏用到的GT911/9147/1158/9271，则初始化失败，需硬件查看触摸IC型号以及查看时序函数是否正确 */
    }
    printf("CTP ID:%s\r\n", temp);                              /* 打印ID */
    
    if (strcmp((char *)temp, "9271") == 0)                      /* ID==9271, 支持10点触摸 */
    {
        g_gt_tnum = 10;                                         /* 支持10点触摸屏 */
    }

    temp[0] = 0X02;
    gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);                    /* 软复位GT9XXX */
    
    vTaskDelay(10);
    
    temp[0] = 0X00;
    gt9xxx_wr_reg(GT9XXX_CTRL_REG, temp, 1);                    /* 结束复位, 进入读坐标状态 */

    return 0;
}

/* GT9XXX 10个触摸点(最多) 对应的寄存器表 */
const uint16_t GT9XXX_TPX_TBL[10] =
{
    GT9XXX_TP1_REG, GT9XXX_TP2_REG, GT9XXX_TP3_REG, GT9XXX_TP4_REG, GT9XXX_TP5_REG,
    GT9XXX_TP6_REG, GT9XXX_TP7_REG, GT9XXX_TP8_REG, GT9XXX_TP9_REG, GT9XXX_TP10_REG,
};

/**
 * @brief       扫描触摸屏(采用查询方式)
 * @param       mode : 电容屏未用到次参数, 为了兼容电阻屏
 * @retval      当前触屏状态
 *   @arg       0, 触屏无触摸; 
 *   @arg       1, 触屏有触摸;
 */
uint8_t gt9xxx_scan(uint8_t mode)
{
    uint8_t buf[4];
    uint8_t i = 0;
    uint8_t res = 0;
    uint16_t temp;
    uint16_t tempsta;
    static uint8_t t = 0;                                                               /* 控制查询间隔,从而降低CPU占用率 */
    t++;

    if ((t % 10) == 0 || t < 10)                                                        /* 空闲时,每进入10次CTP_Scan函数才检测1次,从而节省CPU使用率 */
    {
        gt9xxx_rd_reg(GT9XXX_GSTID_REG, &mode, 1);                                      /* 读取触摸点的状态 */

        if ((mode & 0X80) && ((mode & 0XF) <= g_gt_tnum))
        {
            i = 0;
            gt9xxx_wr_reg(GT9XXX_GSTID_REG, &i, 1);                                     /* 清标志 */
        }

        if ((mode & 0XF) && ((mode & 0XF) <= g_gt_tnum))
        {
            temp = 0XFFFF << (mode & 0XF);                                              /* 将点的个数转换为1的位数,匹配tp_dev.sta定义 */
            tempsta = tp_dev.sta;                                                       /* 保存当前的tp_dev.sta值 */
            tp_dev.sta = (~temp) | TP_PRES_DOWN | TP_CATH_PRES;
            tp_dev.x[g_gt_tnum - 1] = tp_dev.x[0];                                      /* 保存触点0的数据,保存在最后一个上 */
            tp_dev.y[g_gt_tnum - 1] = tp_dev.y[0];

            for (i = 0; i < g_gt_tnum; i++)
            {
                if (tp_dev.sta & (1 << i))                                              /* 触摸有效? */
                {
                    gt9xxx_rd_reg(GT9XXX_TPX_TBL[i], buf, 4);                           /* 读取XY坐标值 */

                    if (tp_dev.touchtype & 0X01)                                        /* 横屏 */
                    {
                        tp_dev.x[i] = ((uint16_t)buf[1] << 8) + buf[0];
                        tp_dev.y[i] = ((uint16_t)buf[3] << 8) + buf[2];
                    }
                    else                                                                /* 竖屏 */
                    {
                        tp_dev.x[i] = ltdcdev.width - (((uint16_t)buf[3] << 8) + buf[2]);
                        tp_dev.y[i] = ((uint16_t)buf[1] << 8) + buf[0];
                    }
                }
            }

            res = 1;

            if (tp_dev.x[0] > ltdcdev.width || tp_dev.y[0] > ltdcdev.height)            /* 非法数据(坐标超出了) */
            {
                if ((mode & 0XF) > 1)                                                   /* 有其他点有数据,则复第二个触点的数据到第一个触点. */
                {
                    tp_dev.x[0] = tp_dev.x[1];
                    tp_dev.y[0] = tp_dev.y[1];
                    t = 0;                                                              /* 触发一次,则会最少连续监测10次,从而提高命中率 */
                }
                else                                                                    /* 非法数据,则忽略此次数据(还原原来的) */
                {
                    tp_dev.x[0] = tp_dev.x[g_gt_tnum - 1];
                    tp_dev.y[0] = tp_dev.y[g_gt_tnum - 1];
                    mode = 0X80;
                    tp_dev.sta = tempsta;                                               /* 恢复tp_dev.sta */
                }
            }
            else 
            {
                t = 0;                                                                  /* 触发一次,则会最少连续监测10次,从而提高命中率 */
            }
        }
    }

    if ((mode & 0X8F) == 0X80)                                                          /* 无触摸点按下 */
    {
        if (tp_dev.sta & TP_PRES_DOWN)                                                  /* 之前是被按下的 */
        {
            tp_dev.sta &= ~TP_PRES_DOWN;                                                /* 标记按键松开 */
        }
        else                                                                            /* 之前就没有被按下 */
        {
            tp_dev.x[0] = 0xffff;
            tp_dev.y[0] = 0xffff;
            tp_dev.sta &= 0XE000;                                                       /* 清除点有效标记 */
        }
    }

    if (t > 240)
    {
        t = 10;                                                                         /* 重新从10开始计数 */
    }

    return res;
}
