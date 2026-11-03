/**
 ****************************************************************************************************
 * @file        emission.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       RMT红外解码驱动代码
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

#include "emission.h"


/* 保存NEC解码的地址和命令字节 */
uint16_t s_nec_code_address;
uint16_t s_nec_code_command;

QueueHandle_t receive_queue;
uint8_t tbuf[40];
extern uint8_t rmt_flag;

/**
 * @brief       初始化RMT
 * @param       无
 * @retval      无
 */
void emission_init(void)
{
    uint8_t t = 0;
    uint8_t xl_key = 0;

    /* 配置接收通道 */
    rmt_rx_channel_config_t rx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,                                                                                 /* RMT接收通道时钟源 */
        .resolution_hz = RMT_RESOLUTION_HZ,                                                                             /* RMT接收通道时钟分辨率 */
        .mem_block_symbols = 64,                                                                                        /* 通道一次可以存储的RMT符号数量 */
        .gpio_num = RMT_RX_PIN,                                                                                         /* RMT接收通道引脚 */
    };
    rmt_channel_handle_t rx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_channel_cfg, &rx_channel));                                                  /* 创建一个RMT接收通道 */

    /* 配置消息队列 */
    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));                                    /* 定义一个消息队列，用以处理RMT接收的回调函数 */
    assert(receive_queue);
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = RMT_Rx_Done_Callback,                                                                           /* 事件回调，当一个RMT通道接收事务完成时调用 */
    };
    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_channel, &cbs, receive_queue));                                  /* 为RMT RX信道设置回调 */

    /* 以下时间要求基于NEC协议 */
    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 1250,                                                                                    /* NEC信号的最短持续时间为560us，1250ns＜560us，有效信号不会被视为噪声 */
        .signal_range_max_ns = 12000000,                                                                                /* NEC信号的最长持续时间为9000us，12000000ns>9000us，接收不会提前停止 */
    };

    /* 配置发送通道 */
    rmt_tx_channel_config_t tx_channel_cfg = {
        .clk_src = RMT_CLK_SRC_DEFAULT,                                                                                 /* RMT发送通道时钟源 */
        .resolution_hz = RMT_RESOLUTION_HZ,                                                                             /* RMT发送通道时钟分辨率 */
        .mem_block_symbols = 64,                                                                                        /* 通道一次可以存储的RMT符号数量 */
        .trans_queue_depth = 4,                                                                                         /* 允许在后台挂起的事务数，本例不会对多个事务进行排队，因此队列深度>1就足够了 */
        .gpio_num = RMT_TX_PIN,                                                                                         /* RMT发送通道引脚 */
    };
    rmt_channel_handle_t tx_channel = NULL;
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_channel_cfg, &tx_channel));                                                  /* 创建一个RMT发送通道 */

    /* 配置载波与占空比s */
    rmt_carrier_config_t carrier_cfg = {
        .frequency_hz = 38000,                                                                                          /* 载波频率，0表示禁用载波 */
        .duty_cycle = 0.33,                                                                                             /* 载波占空比 */
    };
    ESP_ERROR_CHECK(rmt_apply_carrier(tx_channel, &carrier_cfg));                                                       /* 对发送信道应用调制功能 */

    /* 不会在循环中发送NEC帧 */
    rmt_transmit_config_t transmit_config = {
        .loop_count = 0,                                                                                                /* 0为不循环，-1为无限循环 */
    };

    /* 配置编码器 */
    ir_nec_encoder_config_t nec_encoder_cfg = {
        .resolution = RMT_RESOLUTION_HZ,                                                                                /* 编码器分辨率 */
    };
    rmt_encoder_handle_t nec_encoder = NULL;
    ESP_ERROR_CHECK(rmt_new_ir_nec_encoder(&nec_encoder_cfg, &nec_encoder));                                            /* 配置编码器 */

    /* 使能发送、接收通道 */
    ESP_ERROR_CHECK(rmt_enable(tx_channel));                                                                            /* 使能发送通道 */
    ESP_ERROR_CHECK(rmt_enable(rx_channel));                                                                            /* 使能接收通道 */

    /* 保存接收到的RMT符号 */
    rmt_symbol_word_t raw_symbols[64];                                                                                  /* 64个符号对于标准NEC框架应该足够 */
    rmt_rx_done_event_data_t rx_data;

    ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));                        /* 准备接收 */

    while (1)
    {
        if (xQueueReceive(receive_queue, &rx_data, pdMS_TO_TICKS(1000)) == pdPASS)                                      /* 等待RX完成信号 */
        {
            example_parse_nec_frame(rx_data.received_symbols, rx_data.num_symbols);                                     /* 解析接收符号并打印结果 */
            ESP_ERROR_CHECK(rmt_receive(rx_channel, raw_symbols, sizeof(raw_symbols), &receive_config));                /* 重新开始接收 */
        }
        else                                                                                                            /* 超时，传输预定义的IR NEC数据包 */
        {
            t++;

            if (t == 0)
            {
                t = 1;
            }

            const ir_nec_scan_code_t scan_code = {
                .command = t,
            };

            xl_key = xl9555_key_scan(0);

            if (rmt_flag == 1)
            {
                printf("******OK!******\r\n");
                break;
            }

            if (xl_key == KEY0_PRES)
            {
                rmt_flag = 0;
                printf("******FAIL!******\r\n");
                break;
            }
            
            printf("TX CMD = %d\n", scan_code.command);
            ESP_ERROR_CHECK(rmt_transmit(tx_channel, nec_encoder, &scan_code, sizeof(scan_code), &transmit_config));    /* 通过RMT发送信道传输数据 */
        }
    }

    rmt_disable(tx_channel);
    rmt_disable(rx_channel);
    vQueueDelete(receive_queue);
}

/**
 * @brief       判断数据时序长度是否在NEC时序时长容差范围内 正负 RMT_NEC_DECODE_MARGIN 的值以内
 * @param       无
 * @retval      无
 */
inline bool nec_check_in_range(uint32_t signal_duration, uint32_t spec_duration)
{
    return (signal_duration < (spec_duration + RMT_NEC_DECODE_MARGIN)) &&
           (signal_duration > (spec_duration - RMT_NEC_DECODE_MARGIN));
}

/**
 * @brief       对比数据时序长度判断是否为逻辑0
 * @param       无
 * @retval      无
 */
bool nec_parse_logic0(rmt_symbol_word_t *rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ZERO_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ZERO_DURATION_1);
}

/**
 * @brief       对比数据时序长度判断是否为逻辑1
 * @param       无
 * @retval      无
 */
bool nec_parse_logic1(rmt_symbol_word_t *rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_PAYLOAD_ONE_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_PAYLOAD_ONE_DURATION_1);
}

/**
 * @brief       将RMT接收结果解码出NEC地址和命令
 * @param       无
 * @retval      无
 */
bool nec_parse_frame(rmt_symbol_word_t *rmt_nec_symbols)
{
    rmt_symbol_word_t *cur = rmt_nec_symbols;
    uint16_t address = 0;
    uint16_t command = 0;

    bool valid_leading_code = nec_check_in_range(cur->duration0, NEC_LEADING_CODE_DURATION_0) &&
                              nec_check_in_range(cur->duration1, NEC_LEADING_CODE_DURATION_1);

    if (!valid_leading_code) 
    {
        return false;
    }

    cur++;

    for (int i = 0; i < 16; i++)
    {
        if (nec_parse_logic1(cur)) 
        {
            address |= 1 << i;
        } 
        else if (nec_parse_logic0(cur))
        {
            address &= ~(1 << i);
        } 
        else 
        {
            return false;
        }
        cur++;
    }

    for (int i = 0; i < 16; i++)
    {
        if (nec_parse_logic1(cur))
        {
            command |= 1 << i;
        }
        else if (nec_parse_logic0(cur))
        {
            command &= ~(1 << i);
        }
        else
        {
            return false;
        }
        cur++;
    }

    /* 保存数据地址和命令，用于判断重复按键 */
    s_nec_code_address = address;
    s_nec_code_command = command;

    return true;
}

/**
 * @brief       检查数据帧是否为重复按键：一直按住同一个键
 * @param       无
 * @retval      无
 */
bool nec_parse_frame_repeat(rmt_symbol_word_t *rmt_nec_symbols)
{
    return nec_check_in_range(rmt_nec_symbols->duration0, NEC_REPEAT_CODE_DURATION_0) &&
           nec_check_in_range(rmt_nec_symbols->duration1, NEC_REPEAT_CODE_DURATION_1);
}

/**
 * @brief       根据NEC编码解析红外协议并打印指令结果
 * @param       无
 * @retval      无
 */
void example_parse_nec_frame(rmt_symbol_word_t *rmt_nec_symbols, size_t symbol_num)
{
    switch (symbol_num) /* 解码RMT接收数据 */
    {
        case 34:        /* 正常NEC数据帧 */
        {
            if (nec_parse_frame(rmt_nec_symbols) )
            {
                rmt_flag = 1;
                printf("RX CMD = %d\n", s_nec_code_command);
            }
            break;
        }
        
        case 2:         /* 重复NEC数据帧 */
        {
            if (nec_parse_frame_repeat(rmt_nec_symbols))
            {
            }
            break;
        }

        default:        /* 未知NEC数据帧 */
        {
            break;
        }
    }
}

/**
 * @brief       RMT数据接收完成回调函数
 * @param       无
 * @retval      无
 */
bool RMT_Rx_Done_Callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *user_data)
{
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;

    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup); /* 将收到的RMT数据通过消息队列发送到解析任务 */
    return high_task_wakeup == pdTRUE;
}
