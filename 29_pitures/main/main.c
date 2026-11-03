/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       图片显示 实验
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

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "led.h"
#include "spi.h"
#include "iic.h"
#include "lcd.h"
#include "xl9555.h"
#include "spi_sdcard.h"
#include "math.h"
#include "exfuns.h"
#include "piclib.h"
#include "ff.h"
#include "fonts.h"
#include "text.h"


i2c_obj_t i2c0_master;

/**
 * @brief       得到path路径下,目标文件的总个数
 * @param       path : 路径
 * @retval      总有效文件数
 */
uint16_t pic_get_tnum(char *path)
{
    uint8_t res;
    uint16_t rval = 0;
    FF_DIR tdir;                                    /* 临时目录 */
    FILINFO *tfileinfo;                             /* 临时文件信息 */
    tfileinfo = (FILINFO *)malloc(sizeof(FILINFO)); /* 申请内存 */
    res = f_opendir(&tdir, (const TCHAR *)path);    /* 打开目录 */

    if (res == FR_OK && tfileinfo)
    {
        while (1)                                   /* 查询总的有效文件数 */
        {
            res = f_readdir(&tdir, tfileinfo);      /* 读取目录下的一个文件 */

            if (res != FR_OK || tfileinfo->fname[0] == 0)break; /* 错误了/到末尾了,退出 */
            res = exfuns_file_type(tfileinfo->fname);

            if ((res & 0X0F) != 0X00)               /* 取低四位,看看是不是图片文件 */
            {
                rval++;                             /* 有效文件数增加1 */
            }
        }
    }

    free(tfileinfo);                                /* 释放内存 */
    return rval;
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
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    esp_err_t ret = 0;
    uint8_t res = 0;
    FF_DIR picdir;                                      /* 图片目录 */
    FILINFO *picfileinfo;                               /* 文件信息 */
    char *pname;                                        /* 带路径的文件名 */
    uint16_t totpicnum;                                 /* 图片文件总数 */
    uint16_t curindex = 0;                              /* 图片当前索引 */
    uint8_t key = 0;                                    /* 键值 */
    uint8_t pause = 0;                                  /* 暂停标记 */
    uint8_t t;
    uint16_t temp;
    uint32_t *picoffsettbl;                             /* 图片文件offset索引表 */

    ret = nvs_flash_init();                             /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                                         /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);                  /* 初始化IIC0 */
    spi2_init();                                        /* 初始化SPI */
    xl9555_init(i2c0_master);                           /* 初始化IO扩展芯片 */  
    lcd_init();                                         /* 初始化LCD */

    while (sd_spi_init())                               /* 检测不到SD卡 */
    {
        lcd_show_string(30, 110, 200, 16, 16, "SD Card Error!", RED);
        vTaskDelay(500);
        lcd_show_string(30, 130, 200, 16, 16, "Please Check! ", RED);
        vTaskDelay(500);
    }

    res = exfuns_init();                                /* 为fatfs相关变量申请内存 */

    while (fonts_init())                                /* 检查字库 */
    {
        lcd_clear(WHITE);                               /* 清屏 */
        lcd_show_string(30, 30, 200, 16, 16, "ESP32-S3", RED);
        
        key = fonts_update_font(30, 50, 16, (uint8_t *)"0:", RED);  /* 更新字库 */

        while (key)                                     /* 更新失败 */
        {
            lcd_show_string(30, 50, 200, 16, 16, "Font Update Failed!", RED);
            vTaskDelay(200);
            lcd_fill(20, 50, 200 + 20, 90 + 16, WHITE);
            vTaskDelay(200);
        }

        lcd_show_string(30, 50, 200, 16, 16, "Font Update Success!   ", RED);
        vTaskDelay(1500);
        lcd_clear(WHITE);                               /* 清屏 */
    }

    text_show_string(30, 50, 200, 16, "正点原子STM32开发板",16,0, RED);
    text_show_string(30, 70, 200, 16, "图片显示 实验", 16, 0, RED);
    text_show_string(30, 90, 200, 16, "正点原子@ALIENTEK", 16, 0, RED);
    text_show_string(30, 110, 200, 16, "KEY0:NEXT KEY1:PREV", 16, 0, RED);
    text_show_string(30, 130, 200, 16, "KEY_UP:PAUSE:", 16, 0, RED);

    while (f_opendir(&picdir, "0:/PICTURE"))            /* 打开图片文件夹 */
    {
        text_show_string(30, 150, 240, 16, "PICTURE文件夹错误!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);             /* 清除显示 */
        vTaskDelay(200);
    }

    totpicnum = pic_get_tnum("0:/PICTURE");             /* 得到总有效文件数 */
 
    while (totpicnum == NULL)                           /* 图片文件为0 */
    {
        text_show_string(30, 150, 240, 16, "没有图片文件!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);             /* 清除显示 */
        vTaskDelay(200);
    }

    picfileinfo = (FILINFO *)malloc(sizeof(FILINFO));   /* 申请内存 */
    pname = malloc(255 * 2 + 1);                        /* 为带路径的文件名分配内存 */
    picoffsettbl = malloc(4 * totpicnum);               /* 申请4*totpicnum个字节的内存,用于存放图片索引 */

    while (!picfileinfo || !pname || !picoffsettbl)     /* 内存分配出错 */
    {
        text_show_string(30, 150, 240, 16, "内存分配失败!", 16, 0, RED);
        vTaskDelay(200);
        lcd_fill(30, 150, 240, 186, WHITE);             /* 清除显示 */
        vTaskDelay(200);
    }

    /* 记录索引 */
    res = f_opendir(&picdir, "0:/PICTURE");             /* 打开目录 */

    if (res == FR_OK)
    {
        curindex = 0;                                   /* 当前索引为0 */

        while (1)                                       /* 全部查询一遍 */
        {
            temp = picdir.dptr;                         /* 记录当前dptr偏移 */
            res = f_readdir(&picdir, picfileinfo);      /* 读取目录下的一个文件 */
            if (res != FR_OK || picfileinfo->fname[0] == 0)break;   /* 错误了/到末尾了,退出 */

            res = exfuns_file_type(picfileinfo->fname);

            if ((res & 0X0F) != 0X00)                   /* 取高四位,看看是不是图片文件 */
            {
                picoffsettbl[curindex] = temp;          /* 记录索引 */
                curindex++;
            }
        }
    }

    text_show_string(30, 150, 240, 16, "开始显示...", 16, 0, RED);
    vTaskDelay(1500);
    piclib_init();                                                              /* 初始化画图 */
    curindex = 0;                                                               /* 从0开始显示 */
    res = f_opendir(&picdir, (const TCHAR *)"0:/PICTURE");                      /* 打开目录 */

    while (res == FR_OK)                                                        /* 打开成功 */
    {
        atk_dir_sdi(&picdir, picoffsettbl[curindex]);                           /* 改变当前目录索引 */
        res = f_readdir(&picdir, picfileinfo);                                  /* 读取目录下的一个文件 */

        if (res != FR_OK || picfileinfo->fname[0] == 0)break;                   /* 错误了/到末尾了,退出 */

        strcpy((char *)pname, "0:/PICTURE/");                                   /* 复制路径(目录) */
        strcat((char *)pname, (const char *)picfileinfo->fname);                /* 将文件名接在后面 */
        lcd_clear(BLACK);
        piclib_ai_load_picfile(pname, 0, 0, lcd_self.width, lcd_self.height);   /* 显示图片 */
        text_show_string(2, 2, lcd_self.width, 16, (char *)pname, 16, 0, RED);  /* 显示图片名字 */
        t = 0;

        while (1)
        {
            key = xl9555_key_scan(0);   /* 扫描按键 */

            if (t > 250)key = 1;        /* 模拟一次按下KEY0 */

            if ((t % 20) == 0)
            {
                LED_TOGGLE();           /* LED闪烁,提示程序正在运行. */
            }

            if (key == KEY1_PRES)       /* 上一张 */
            {
                if (curindex)
                {
                    curindex--;
                }
                else
                {
                    curindex = totpicnum - 1;
                }
                
                break;
            }
            else if (key == KEY0_PRES)      /* 下一张 */
            {
                curindex++;

                if (curindex >= totpicnum)
                {
                    curindex = 0;           /* 到末尾的时候,自动从头开始 */
                }

                break;
            }
            else if (key == KEY3_PRES)
            {
                pause = !pause;
                LED(pause);                 /* 暂停的时候LED1亮. */
            }

            if (pause == 0)t++;

            vTaskDelay(10);
        }

        res = 0;
    }

    free(picfileinfo);    /* 释放内存 */
    free(pname);          /* 释放内存 */
    free(picoffsettbl);   /* 释放内存 */
}
