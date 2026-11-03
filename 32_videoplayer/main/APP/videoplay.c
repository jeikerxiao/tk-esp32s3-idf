/**
 ****************************************************************************************************
 * @file        videoplay.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       视频播放器 应用代码
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

#include "videoplay.h"


extern uint8_t frameup;

/**
 * @brief       获取指定路径下有效视频文件的数量
 * @param       path: 指定路径
 * @retval      有效视频文件的数量
 */
static uint16_t video_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    FF_DIR tdir;
    FILINFO *tfileinfo;
    
    tfileinfo = (FILINFO *)malloc(sizeof(FILINFO));             /* 申请内存 */
    res = (uint8_t)f_opendir(&tdir, (const TCHAR *)path);       /* 打开目录 */
    
    if ((res == 0) && tfileinfo)
    {
        while (1)                                               /* 查询总的有效文件数 */
        {
            res = (uint8_t)f_readdir(&tdir, tfileinfo);         /* 读取目录下的一个文件 */
            
            if ((res != 0) || (tfileinfo->fname[0] == 0))       /* 错误或到末尾，退出 */
            {
                break;
            }
            
            res = exfuns_file_type(tfileinfo->fname);
            
            if ((res & 0xF0) == 0x60)                           /* 是视频文件 */
            {
                rval++;
            }
        }
    }
    
    free(tfileinfo);                                            /* 释放内存 */
    
    return rval;
}

/**
 * @brief       显示视频基本信息
 * @param       name : 视频名字
 * @param       index: 当前索引
 * @param       total: 总文件数
 * @retval      无
 */
static void video_bmsg_show(uint8_t *name, uint16_t index, uint16_t total)
{
    uint8_t *buf;
    
    buf = malloc(100);                                                /* 申请100字节内存 */
    
    sprintf((char *)buf, "文件名:%s", name);
//    text_show_string(10, 10, lcd_self.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示文件名 */
    
    sprintf((char *)buf, "索引:%d/%d", index, total);
//    text_show_string(10, 30, lcd_self.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示索引 */
    
    free(buf);                                                        /* 释放内存 */
}

/**
 * @brief       显示当前视频文件的相关信息
 * @param       aviinfo: avi控制结构体
 * @retval      无
 */
static void video_info_show(AVI_INFO *aviinfo)
{
    uint8_t *buf;
    
    buf = malloc(100);                                                /* 申请100字节内存 */
    
    sprintf((char *)buf, "声道数:%d,采样率:%ld", aviinfo->Channels, aviinfo->SampleRate * 10);
    text_show_string(10, 10, lcd_self.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示歌曲名字 */
    
    sprintf((char *)buf, "帧率:%ld帧", 1000 / (aviinfo->SecPerFrame / 1000));
    text_show_string(10, 30, lcd_self.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示歌曲名字 */
    
    free(buf);                                                        /* 释放内存 */
}

/**
 * @brief       显示当前播放时间
 * @param       favi   : 当前播放的视频文件
 * @param       aviinfo: avi控制结构体
 * @retval      无
 */
void video_time_show(FIL *favi, AVI_INFO *aviinfo)
{
    static uint32_t oldsec;                                         /* 上一次的播放时间 */
    
    uint8_t *buf;
    
    uint32_t totsec = 0;                                            /* video文件总时间 */
    uint32_t cursec;                                                /* 当前播放时间 */
    
    totsec = (aviinfo->SecPerFrame / 1000) * aviinfo->TotalFrame;   /* 歌曲总长度(单位:ms) */
    totsec /= 1000;                                                 /* 秒钟数. */
    cursec = ((double)favi->fptr / favi->obj.objsize) * totsec;     /* 获取当前播放到多少秒 */
    
    if (oldsec != cursec)                                           /* 需要更新显示时间 */
    {
        buf = malloc(100);                                          /* 申请100字节内存 */
        oldsec = cursec;
        
        sprintf((char *)buf, "Play time:%02ld:%02ld:%02ld/%02ld:%02ld:%02ld", cursec / 3600, (cursec % 3600) / 60, cursec % 60, totsec / 3600, (totsec % 3600) / 60, totsec % 60);
        text_show_string(10, 50, lcd_self.width - 10, 16, (char *)buf, 16, 0, RED);   /* 显示歌曲名字 */
        
        free(buf);
    }
}

/**
 * @brief       播放MJPEG视频
 * @param       pname: 视频文件名
 * @retval      按键键值
 *              KEY0_PRES: 上一个视频
 *              WKUP_PRES: 下一个视频
 *              其他值   : 错误代码
 */
static uint8_t video_play_mjpeg(uint8_t *pname)
{
    uint8_t *framebuf;
    uint8_t *pbuf;
    uint8_t res = 0;
    uint16_t offset;
    uint32_t nr;
    uint8_t key;
    FIL *favi;

    framebuf = (uint8_t *)malloc(AVI_VIDEO_BUF_SIZE);   /* 申请内存 */
    favi = (FIL *)malloc(sizeof(FIL));

    if ((framebuf == NULL) || (favi == NULL))
    {
        printf("memory error!\r\n");
        res = 0xFF;
    }

    memset(framebuf, 0, AVI_VIDEO_BUF_SIZE);
    
    while (res == 0)
    {
        res = (uint8_t)f_open(favi, (const TCHAR *)pname, FA_READ);                                                     /* 打开文件 */
        
        if (res == 0)
        {
            pbuf = framebuf;
            res = (uint8_t)f_read(favi, pbuf, AVI_VIDEO_BUF_SIZE, (UINT*)&nr);                                                 /* 开始读取 */
            
            if (res != 0)
            {
                printf("fread error:%d\r\n", res);
                break;
            }
            
            res = avi_init(pbuf, AVI_VIDEO_BUF_SIZE);                                                                   /* AVI解析 */
            
            if (res != 0)
            {
                printf("avi error:%d\r\n", res);
                break;
            }
            
            video_info_show(&g_avix);
            esptim_int_init(g_avix.SecPerFrame / 1000, 1000);
            offset = avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");                                                   /* 寻找movi ID */
            avi_get_streaminfo(pbuf + offset + 4);                                                                      /* 获取流信息 */
            f_lseek(favi, offset + 12);                                                                                 /* 跳过标志ID，读地址偏移到流数据开始处 */
            res = mjpegdec_init((lcd_self.width - g_avix.Width) / 2, 110 + (lcd_self.height - 110 - g_avix.Height) / 2);/* 初始化JPG解码 */
            /* 定义图像的宽高 */
            Windows_Width = g_avix.Width;
            Windows_Height = g_avix.Height;

            if (g_avix.SampleRate)                                          /* 有音频信息,才初始化 */
            {
                printf("g_avix.SampleRate:%ld\r\n",g_avix.SampleRate);
                es8388_sai_cfg(0, 3);                                       /* 飞利浦标准,16位数据长度 */
                i2s_set_samplerate_bits_sample(g_avix.SampleRate,I2S_BITS_PER_SAMPLE_16BIT);    /* 设置采样率 */
                i2s_start(I2S_NUM);
            }

            while (1)
            {
                if (g_avix.StreamID == AVI_VIDS_FLAG)                       /* 视频流 */
                {
                    pbuf = framebuf;
                    f_read(favi, pbuf, g_avix.StreamSize + 8, (UINT*)&nr);  /* 读取整帧+下一帧数据流ID信息 */
                    res = mjpegdec_decode(pbuf, g_avix.StreamSize);
                    
                    if (res != 0)
                    {
                        printf("decode error!\r\n");
                    }

                    while (frameup == 0);                                   /* 等待播放时间到达 */
                    frameup = 0;
                }
                else
                {
                    video_time_show(favi, &g_avix);                         /* 显示当前播放时间 */
                    f_read(favi, framebuf, g_avix.StreamSize + 8, &nr);     /* 填充psaibuf */
                    pbuf = framebuf;
                    i2s_tx_write(framebuf, g_avix.StreamSize);          /* 数据转换+发送给DAC */
                }
                
                key = xl9555_key_scan(0);

                if (key == KEY0_PRES || key == KEY2_PRES)                   /* KEY0/KEY2按下,播放下一个/上一个视频 */
                {
                    res = key;
                    break;
                }
                else if (key == KEY1_PRES || key == KEY3_PRES)
                {
                    i2s_stop(I2S_NUM);                                      /* 关闭音频 */
                    video_seek(favi, &g_avix, framebuf);
                    pbuf = framebuf;
                    i2s_start(I2S_NUM);                                     /* 开启DMA播放 */
                }

                if (avi_get_streaminfo(pbuf + g_avix.StreamSize) != 0)      /* 读取下一帧流标志 */
                {
                    printf("g_frame error\r\n");
                    res = KEY0_PRES;
                    break;
                }
            }

            i2s_stop(I2S_NUM);
            esp_timer_stop(esp_tim_handle);
            lcd_set_window(0, 0, lcd_self.width, lcd_self.height);          /* 恢复窗口 */
            mjpegdec_free();                                                /* 释放内存 */
            f_close(favi);                                                  /* 关闭文件 */
        }
    }

    i2s_zero_dma_buffer(I2S_NUM);
    free(framebuf);
    free(favi);
    
    return res;
}

/**
 * @brief       转换
 * @param       fs:文件系统对象
 * @param       clst:转换
 * @retval      =0:扇区号，0:失败
 */
static LBA_t atk_clst2sect(FATFS *fs, DWORD clst)
{
    clst -= 2;  /* Cluster number is origin from 2 */

    if (clst >= fs->n_fatent - 2)
    {
        return 0;   /* Is it invalid cluster number? */
    }

    return fs->database + (LBA_t)fs->csize * clst;  /* Start sector number of the cluster */
}

/**
 * @brief       偏移
 * @param       dp:指向目录对象
 * @param       Offset:目录表的偏移量
 * @retval      FR_OK(0):成功，!=0:错误
 */
FRESULT atk_dir_sdi(FF_DIR *dp, DWORD ofs)
{
    DWORD clst;
    FATFS *fs = dp->obj.fs;

    if (ofs >= (DWORD)((FF_FS_EXFAT && fs->fs_type == FS_EXFAT) ? 0x10000000 : 0x200000) || ofs % 32)
    {
        /* Check range of offset and alignment */
        return FR_INT_ERR;
    }

    dp->dptr = ofs;         /* Set current offset */
    clst = dp->obj.sclust;  /* Table start cluster (0:root) */

    if (clst == 0 && fs->fs_type >= FS_FAT32)
    {	/* Replace cluster# 0 with root cluster# */
        clst = (DWORD)fs->dirbase;

        if (FF_FS_EXFAT)
        {
            dp->obj.stat = 0;
        }
        /* exFAT: Root dir has an FAT chain */
    }

    if (clst == 0)
    {	/* Static table (root-directory on the FAT volume) */
        if (ofs / 32 >= fs->n_rootdir)
        {
            return FR_INT_ERR;  /* Is index out of range? */
        }

        dp->sect = fs->dirbase;

    }
    else
    {   /* Dynamic table (sub-directory or root-directory on the FAT32/exFAT volume) */
        dp->sect = atk_clst2sect(fs, clst);
    }

    dp->clust = clst;   /* Current cluster# */

    if (dp->sect == 0)
    {
        return FR_INT_ERR;
    }

    dp->sect += ofs / fs->ssize;             /* Sector# of the directory entry */
    dp->dir = fs->win + (ofs % fs->ssize);   /* Pointer to the entry in the win[] */

    return FR_OK;
}

/**
 * @brief       播放视频
 * @param       无
 * @retval      无
 */
void video_play(void)
{
    uint8_t res;
    FF_DIR vdir;
    FILINFO *vfileinfo;
    uint8_t *pname;
    uint16_t totavinum;
    uint16_t curindex;
    uint32_t *voffsettbl;
    uint8_t key;
    uint32_t temp;
    
    while (f_opendir(&vdir, "0:/VIDEO") != FR_OK)                               /* 检查VIDEO文件夹是否存在 */
    {
        text_show_string(60, 190, 240, 16, "VIDEO文件夹错误!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(60, 190, 240, 206, WHITE);
        vTaskDelay(200);
    }
    
    totavinum = video_get_tnum("0:/VIDEO");                                     /* 检查是否有视频文件 */
    
    while (totavinum == 0)
    {
        text_show_string(60, 190, 240, 16, "没有视频文件!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(60, 190, 240, 146, WHITE);
        vTaskDelay(200);
    }
    
    vfileinfo = (FILINFO *)malloc(sizeof(FILINFO));                             /* 申请内存 */
    pname = (uint8_t *)malloc(2 * 255 + 1);
    voffsettbl = (uint32_t *)malloc(totavinum  * 4);
    
    while ((vfileinfo == NULL) || (pname == NULL) || (voffsettbl == NULL))
    {
        text_show_string(60, 190, 240, 16, "内存分配失败!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(60, 190, 240, 146, WHITE);
        vTaskDelay(200);
    }
    
    res = (uint8_t)f_opendir(&vdir, "0:/VIDEO");                                /* 打开目录 */
    
    if (res == 0)
    {
        curindex = 0;
        
        while (1)
        {
            temp = vdir.dptr;                                                   /* 记录当前dptr偏移 */
            res = (uint8_t)f_readdir(&vdir, vfileinfo);                         /* 读取下一个文件 */
            
            if ((res != 0) || (vfileinfo->fname[0] == 0))                       /* 错误或到末尾，退出 */
            {
                break;
            }
            
            res = exfuns_file_type(vfileinfo->fname);
            
            if ((res & 0xF0) == 0x60)                                           /* 是视频文件 */
            {
                voffsettbl[curindex] = temp;                                    /* 记录索引 */
                curindex++;
            }
        }
    }
    
    curindex = 0;
    res = (uint8_t)f_opendir(&vdir, "0:/VIDEO");                                /* 打开目录 */

    while (res == 0)
    {
        atk_dir_sdi(&vdir, voffsettbl[curindex]);                                   /* 改变当前目录索引 */
        res = (uint8_t)f_readdir(&vdir, vfileinfo);                             /* 读取目录的下一个文件 */
        
        if ((res != 0) || (vfileinfo->fname[0] == 0))                           /* 错误或到末尾，退出 */
        {
            break;
        }
        
        strcpy((char *)pname, "0:/VIDEO/");                                     /* 复制路径（目录） */
        strcat((char *)pname, (const char *)vfileinfo->fname);                  /* 将文件名接在后面 */
        lcd_clear(WHITE);
        video_bmsg_show((uint8_t *)vfileinfo->fname, curindex + 1, totavinum);  /* 显示视频基本信息 */
        
        key = video_play_mjpeg(pname);

        if (key == KEY2_PRES)   /* 上一个视频 */
        {
            if (curindex != 0)
            {
                curindex--;
            }
            else
            {
                curindex = totavinum - 1;
            }
        }
        else if (key == KEY0_PRES)  /* 下一个视频 */
        {
            curindex++;
            if (curindex >= totavinum)
            {
                curindex = 0;
            }
        }
        else
        {
            break;
        }
    }
    
    free(vfileinfo);      /* 释放内存 */
    free(pname);
    free(voffsettbl);
}

/**
 * @brief       AVI文件查找
 * @param       favi    : AVI文件
 * @param       aviinfo : AVI信息结构体
 * @param       mbuf    : 数据缓冲区
 * @retval      执行结果
 *   @arg       0    , 成功
 *   @arg       其他 , 错误
 */
uint8_t video_seek(FIL *favi, AVI_INFO *aviinfo, uint8_t *mbuf)
{
    uint32_t fpos = favi->fptr;
    uint8_t *pbuf;
    uint16_t offset;
    uint32_t br;
    uint32_t delta;
    uint32_t totsec;
    uint8_t key;

    totsec = (aviinfo->SecPerFrame / 1000) * aviinfo->TotalFrame;
    totsec /= 1000;                             /* 秒钟数 */
    delta = (favi->obj.objsize / totsec) * 5;   /* 每次前进5秒钟的数据量 */

    while (1)
    {
        key = xl9555_key_scan(1);

        if (key == KEY3_PRES)                   /* 快进 */
        {
            if (fpos < favi->obj.objsize)
            {
                fpos += delta;
            }

            if (fpos > (favi->obj.objsize - AVI_VIDEO_BUF_SIZE))
            {
                fpos = favi->obj.objsize - AVI_VIDEO_BUF_SIZE;
            }
        }
        else if (key == KEY1_PRES)              /* 快退 */
        {
            if (fpos > delta)
            {
                fpos -= delta;
            }
            else
            {
                fpos = 0;
            }
        }
        else 
        {
            break;
        }

        f_lseek(favi, fpos);
        f_read(favi, mbuf, AVI_VIDEO_BUF_SIZE, &br);       /* 读入整帧+下一数据流ID信息 */
        pbuf = mbuf;
        
        if (fpos == 0)                                     /* 从0开始,得先寻找movi ID */
        {
            offset = avi_srarch_id(pbuf, AVI_VIDEO_BUF_SIZE, "movi");
        }
        else
        {
            offset = 0;
        }

        offset += avi_srarch_id(pbuf + offset, AVI_VIDEO_BUF_SIZE, g_avix.VideoFLAG); /* 寻找视频帧 */
        avi_get_streaminfo(pbuf + offset);                  /* 获取流信息 */
        f_lseek(favi, fpos + offset + 8);                   /* 跳过标志ID,读地址偏移到流数据开始处 */
        
        if (g_avix.StreamID == AVI_VIDS_FLAG)
        {
            f_read(favi, mbuf, g_avix.StreamSize + 8, &br); /* 读入整帧 */
            mjpegdec_decode(mbuf, g_avix.StreamSize);       /* 显示视频帧 */
        }
        else
        {
            printf("error flag");
        }

        video_time_show(favi, &g_avix);
    }

    return 0;
}
