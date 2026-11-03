/**
 ****************************************************************************************************
 * @file        recorder.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       wav解码 代码
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

#include "recorder.h"



uint32_t g_wav_size;    /* wav数据大小(字节数,不包括文件头!!) */

uint8_t g_rec_sta = 0;  /**
                         * 录音状态
                         * [7]:0,没有开启录音;1,已经开启录音;
                         * [6:1]:保留
                         * [0]:0,正在录音;1,暂停录音;
                         */

/**
 * @brief       进入PCM 录音模式
 * @param       无
 * @retval      无
 */
void recoder_enter_rec_mode(void)
{
    es8388_adda_cfg(0, 1);          /* 开启ADC */
    es8388_input_cfg(0);            /* 开启输入通道(通道1,MIC所在通道) */
    es8388_mic_gain(8);             /* MIC增益设置为最大 */
    es8388_alc_ctrl(3, 4, 4);       /* 开启立体声ALC控制,以提高录音音量 */
    es8388_output_cfg(0, 0);        /* 关闭通道1和2的输出 */
    es8388_spkvol_set(0);           /* 关闭喇叭. */
    es8388_sai_cfg(0, 3);           /* 飞利浦标准,16位数据长度 */
    i2s_set_samplerate_bits_sample(SAMPLE_RATE,I2S_BITS_PER_SAMPLE_16BIT);    /* 初始化I2S */
    i2s_trx_start();                /* 开启I2S */
    recoder_remindmsg_show(0);
}

/**
 * @brief       进入PCM 放音模式
 * @param       无
 * @retval      无
 */
void recoder_enter_play_mode(void)
{
    es8388_adda_cfg(1, 0);      /* 开启DAC */
    es8388_output_cfg(1, 1);    /* 开启通道1和2的输出 */
    es8388_spkvol_set(28);      /* 喇叭音量设置 */
    i2s_trx_stop();             /* 停止录音 */
    recoder_remindmsg_show(1);  /* 显示提示信息 */
}

/**
 * @brief       初始化WAV头
 * @param       wavhead : wav文件头指针
 * @retval      无
 */
void recoder_wav_init(__WaveHeader *wavhead)
{
    wavhead->riff.ChunkID = 0x46464952;                  /* RIFF" */
    wavhead->riff.ChunkSize = 0;                         /* 还未确定,最后需要计算 */
    wavhead->riff.Format = 0x45564157;                   /* "WAVE" */
    wavhead->fmt.ChunkID = 0x20746D66;                   /* "fmt " */
    wavhead->fmt.ChunkSize = 16;                         /* 大小为16个字节 */
    wavhead->fmt.AudioFormat = 0x01;                     /* 0x01,表示PCM; 0x00,表示IMA ADPCM */
    wavhead->fmt.NumOfChannels = 2;                      /* 双声道 */
    wavhead->fmt.SampleRate = SAMPLE_RATE;               /* 采样速率 */
    wavhead->fmt.ByteRate = wavhead->fmt.SampleRate * 4; /* 字节速率=采样率*通道数*(ADC位数/8) */
    wavhead->fmt.BlockAlign = 4;                         /* 块大小=通道数*(ADC位数/8) */
    wavhead->fmt.BitsPerSample = 16;                     /* 16位PCM */
    wavhead->data.ChunkID = 0x61746164;                  /* "data" */
    wavhead->data.ChunkSize = 0;                         /* 数据大小,还需要计算 */
}

/**
 * @brief       显示录音时间和码率
 * @param       tsec : 时间（单位 : 秒）
 * @param       kbps : 码率
 * @retval      无
 */
void recoder_msg_show(uint32_t tsec, uint32_t kbps)
{
    lcd_show_string(30, 210, 200, 16, 16, "TIME:", RED);    /* 显示录音时间 */
    lcd_show_num(30 + 40, 210, tsec / 60, 2, 16, RED);      /* 分钟 */
    lcd_show_char(30 + 56, 210, ':', 16, 0, RED);
    lcd_show_num(30 + 64, 210, tsec % 60, 2, 16, RED);      /* 秒钟 */

    lcd_show_string(140, 210, 200, 16, 16, "KPBS:", RED);   /* 显示码率 */
    lcd_show_num(140 + 40, 210, kbps / 1000, 4, 16, RED);   /* 码率显示 */
}

/**
 * @brief       提示信息
 * @param       mode : 工作模式
 *   @arg       0, 录音模式
 *   @arg       1, 放音模式
 * @retval      无
 */
void recoder_remindmsg_show(uint8_t mode)
{
    lcd_fill(30, 120, lcd_self.width, 180, WHITE);            /* 清除原来的显示 */

    if (mode == 0)  /* 录音模式 */
    {
        lcd_show_string(30, 120, 200, 16, 16, "KEY0:REC/PAUSE", BLUE);
        lcd_show_string(30, 140, 200, 16, 16, "KEY2:STOP&SAVE", BLUE);
        lcd_show_string(30, 160, 200, 16, 16, "KEY3:PLAY", BLUE);
    }
    else            /* 放音模式 */
    {
        lcd_show_string(30, 120, 200, 16, 16, "KEY0:STOP Play", BLUE);
        lcd_show_string(30, 140, 200, 16, 16, "KEY3:PLAY/PAUSE", BLUE);
    }
}

/**
 * @brief       通过时间获取文件名
 * @note        仅限在SD卡保存,不支持FLASH DISK保存
 * @note        组合成:形如"0:RECORDER/REC00001.wav"的文件名
 * @param       pname : 文件路径
 * @retval      无
 */
void recoder_new_pathname(uint8_t *pname)
{
    uint8_t res;
    uint16_t index = 0;
    FIL *ftemp;
    ftemp = (FIL *)malloc(sizeof(FIL));           /* 开辟FIL字节的内存区域 */

    if (ftemp == NULL) 
    {
        return;  /* 内存申请失败 */
    }

    while (index < 0xFFFF)
    {
        sprintf((char *)pname, "0:RECORDER/REC%05d.wav", index);
        res = f_open(ftemp, (const TCHAR *)pname, FA_READ); /* 尝试打开这个文件 */

        if (res == FR_NO_FILE)
        {
            break;              /* 该文件名不存在=正是我们需要的. */
        }

        index++;
    }

    free(ftemp);
}

/**
 * @brief       WAV录音
 * @param       无
 * @retval      无
 */
void wav_recorder(void)
{
    uint8_t res;
    uint8_t key;
    uint8_t rval = 0;
    uint32_t bw;
    
    __WaveHeader *wavhead = 0;
    FF_DIR recdir;          /* 目录 */
    FIL *f_rec;             /* 录音文件 */
    
    uint8_t *pdatabuf;      /* 数据缓存指针 */
    uint8_t *pname = 0;     /* 文件名称 */
    uint32_t recsec = 0;    /* 录音时间 */
    uint8_t timecnt = 0;    /* 计时器 */
    uint16_t bytes_read = 0;

    while (f_opendir(&recdir, "0:/RECORDER"))   /* 打开录音文件夹 */
    {
        lcd_show_string(30, 230, 240, 16, 16, "RECORDER folder error!", RED);
        vTaskDelay(200);
        lcd_fill(30, 230, 240, 246, WHITE);     /* 清除显示 */
        vTaskDelay(200);
        f_mkdir("0:/RECORDER");                 /* 创建该目录 */
    }

    pdatabuf = malloc(1024 * 10);                           /* 录音存储区 */
    f_rec = (FIL*)malloc(sizeof(FIL));                      /* 开辟FIL字节的内存区域 */
    wavhead = (__WaveHeader *)malloc(sizeof(__WaveHeader)); /* 开辟__WaveHeader字节的内存区域 */
    pname = malloc(30);   /* 申请30个字节内存,文件名类似"0:RECORDER/REC00001.wav" */

    if (!f_rec || !wavhead || !pname || !pdatabuf)
    {
        rval = 1;           /* 任意一项失败, 则失败 */
    }

    if (rval == 0)
    {
        recoder_enter_rec_mode();   /* 进入录音模式,此时耳机可以听到咪头采集到的音频 */
        pname[0] = 0;               /* pname没有任何文件名 */

        while (rval == 0)
        {
            key = xl9555_key_scan(0);

            switch (key)
            {
                case KEY2_PRES:                                     /* STOP&SAVE */
                    if (g_rec_sta & 0x80)                           /* 有录音 */
                    {
                        g_rec_sta = 0;                              /* 关闭录音 */
                        wavhead->riff.ChunkSize = g_wav_size + 36;  /* 整个文件的大小-8; */
                        wavhead->data.ChunkSize = g_wav_size;       /* 数据大小 */
                        f_lseek(f_rec, 0);                          /* 偏移到文件头. */
                        f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), &bw); /* 写入头数据 */
                        f_close(f_rec);
                        g_wav_size = 0;
                    }

                    g_rec_sta = 0;
                    recsec = 0;
                    LED(1);        /* 关闭DS0 */
                    lcd_fill(30, 190, lcd_self.width, lcd_self.height, WHITE); /* 清除显示,清除之前显示的录音文件名 */
                    break;
 
                case KEY0_PRES:     /* REC/PAUSE */
                    if (g_rec_sta & 0x01)                           /* 如果是暂停,继续录音 */
                    {
                        g_rec_sta &= 0xFE;                          /* 取消暂停 */
                    }
                    else if (g_rec_sta & 0x80)                      /* 已经在录音了,暂停 */
                    {
                        g_rec_sta |= 0x01;                          /* 暂停 */
                    }
                    else                                            /* 还没开始录音 */
                    {
                        recsec = 0;
                        recoder_new_pathname(pname);                /* 得到新的名字 */
                        text_show_string(30, 190, lcd_self.width, 16, "录制:", 16, 0, RED);
                        text_show_string(30 + 40, 190, lcd_self.width, 16, (char *)pname + 11, 16, 0, RED);   /* 显示当前录音文件名字 */
                        recoder_wav_init(wavhead);                  /* 初始化wav数据 */
                        res = f_open(f_rec, (const TCHAR*)pname, FA_CREATE_ALWAYS | FA_WRITE);    /* 打开文件 */

                        if (res)                        /* 文件创建失败 */
                        {
                            g_rec_sta = 0;              /* 创建文件失败,不能录音 */
                            rval = 0xFE;                /* 提示是否存在SD卡 */
                        }
                        else
                        {
                            res = f_write(f_rec, (const void *)wavhead, sizeof(__WaveHeader), (UINT*)&bw); /* 写入头数据 */
                            recoder_msg_show(0, 0);
                            g_rec_sta |= 0x80;          /* 开始录音 */
                        }
                    }

                    if (g_rec_sta & 0x01)
                    {
                        LED(0);                         /* 提示正在暂停 */
                    }
                    else 
                    {
                        LED(1);
                    }
                    break;

                case KEY3_PRES:                         /* 播放最近一段录音 */
                    if (g_rec_sta != 0x80)              /* 没有在录音 */
                    {
                        if (pname[0])                   /* 如果按键被按下,且pname不为空 */
                        {
                            text_show_string(30, 190, lcd_self.width, 16, "播放:", 16, 0, RED);
                            text_show_string(30 + 40, 190, lcd_self.width, 16, (char *)pname + 11, 16, 0, RED); /* 显示当播放的文件名字 */
                            recoder_enter_play_mode();  /* 进入播放模式 */
                            audio_play_song(pname);     /* 播放pname */
                            lcd_fill(30, 190, lcd_self.width, lcd_self.height, WHITE); /* 清除显示,清除之前显示的录音文件名 */
                            recoder_enter_rec_mode();   /* 重新进入录音模式 */
                        }
                    }
                    break;
            }

            if ((g_rec_sta & 0x80) == 0x80)
            {
                if ((g_rec_sta & 0x01) == 0x00)
                {
                    bytes_read = i2s_rx_read((uint8_t *)pdatabuf, 1024 * 10);
                    res = f_write(f_rec, pdatabuf, bytes_read, (UINT*)&bw);  /* 写入文件 */

                    if (res)
                    {
                        printf("write error:%d\r\n", res);
                    }

                    g_wav_size += bytes_read;                         /* WAV数据大小增加 */
                }
            }
            else 
            {
                vTaskDelay(1);
            }

            timecnt++;

            if ((timecnt % 20) == 0)
            {
                LED_TOGGLE();      /* LED闪烁 */
            }


            if (recsec != (g_wav_size / wavhead->fmt.ByteRate))    /* 录音时间显示 */
            {
                recsec = g_wav_size / wavhead->fmt.ByteRate;       /* 录音时间 */
                recoder_msg_show(recsec, wavhead->fmt.SampleRate * wavhead->fmt.NumOfChannels * wavhead->fmt.BitsPerSample); /* 显示码率 */
            }
        }
    }

    free(pdatabuf);       /* 释放内存 */
    free(f_rec);          /* 释放内存 */
    free(wavhead);        /* 释放内存 */
    free(pname);          /* 释放内存 */
}
