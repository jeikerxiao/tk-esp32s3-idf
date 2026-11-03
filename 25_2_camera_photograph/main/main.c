/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-08-26
 * @brief       照相机实验
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
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "camera.h"
#include "led.h"
#include "lcd.h"
#include "spi_sdcard.h"
#include "jpegd2.h"
#include "sdmmc_cmd.h"
#include "exfuns.h"


/* TASK3 任务 配置
 * 包括: 任务句柄 任务优先级 堆栈大小 创建任务
 */
#define TASK3_PRIO      4                   /* 任务优先级 */
#define TASK3_STK_SIZE  5*1024              /* 任务堆栈大小 */
TaskHandle_t            Task3Task_Handler;  /* 任务句柄 */
void task3(void *pvParameters);             /* 任务函数 */
SemaphoreHandle_t BinarySemaphore;          /* 二值信号量句柄 */
i2c_obj_t i2c0_master;
uint8_t *rgb565 = NULL;
static const char *TAG = "main";
uint8_t sd_check_en = 0;                    /* sd卡检测标志 */
extern sdmmc_card_t *card;                  /* SD / MMC卡结构 */

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
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
{
    unsigned long i = 0;
    unsigned long j = 0;
    uint8_t key = 0;

    esp_err_t ret;
    
    ret = nvs_flash_init();             /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    led_init();                         /* 初始化LED */
    i2c0_master = iic_init(I2C_NUM_0);  /* 初始化IIC0 */
    spi2_init();                        /* 初始化SPI2 */
    xl9555_init(i2c0_master);           /* 初始化XL9555 */
    lcd_init();                         /* 初始化LCD */

    while (sd_spi_init())               /* 检测不到SD卡 */
    {
        lcd_show_string(30, 50, 200, 16, 16, "SD Card Failed!", RED);
        vTaskDelay(200);
        lcd_fill(30, 50, 200 + 30, 50 + 16, WHITE);
        vTaskDelay(200);
        sd_check_en = 0;
    }

    sd_check_en = 1;

    lcd_show_string(30, 50, 200, 16, 16, "ESP32", RED);
    lcd_show_string(30, 70, 200, 16, 16, "CAMERA TEST", RED);
    lcd_show_string(30, 90, 200, 16, 16, "ATOM@ALIENTEK", RED);

    /* 初始化摄像头 */
    while (camera_init())
    {
        lcd_show_string(30, 110, 200, 16, 16, "CAMERA Fail!", BLUE);
        vTaskDelay(500);
    }

    rgb565 = malloc(240 * 320 * 2);

    if (NULL == rgb565)
    {
        ESP_LOGE(TAG, "can't alloc memory for rgb565 buffer");
    }

    lcd_clear(BLACK);

    BinarySemaphore = xSemaphoreCreateBinary();
    /* 创建任务3 */
    xTaskCreatePinnedToCore((TaskFunction_t )task3,                 /* 任务函数 */
                            (const char*    )"task3",               /* 任务名称 */
                            (uint16_t       )TASK3_STK_SIZE,        /* 任务堆栈大小 */
                            (void*          )NULL,                  /* 传入给任务函数的参数 */
                            (UBaseType_t    )TASK3_PRIO,            /* 任务优先级 */
                            (TaskHandle_t*  )&Task3Task_Handler,    /* 任务句柄 */
                            (BaseType_t     ) 0);                   /* 该任务哪个内核运行 */

    while (1)
    {
        key = xl9555_key_scan(0);
        camera_fb_t *pic = esp_camera_fb_get();

        if (pic)
        {
            mjpegdraw(pic->buf, pic->len, (uint8_t *)rgb565, NULL);

            lcd_set_window(0, 0, 0 + pic->width - 1, 0 + pic->height - 1);

            if (key == KEY0_PRES)
            {
                /* lcd_buf存储摄像头整一帧RGB数据 */
                for (j = 0; j < pic->width * pic->height; j++)
                {
                    lcd_buf[2 * j] = (pic->buf[2 * i]) ;
                    lcd_buf[2 * j + 1] =  (pic->buf[2 * i + 1]);
                    i ++;
                }

                xSemaphoreGive(BinarySemaphore);                    /* 释放二值信号量 */
            }

            /* 处理SD卡释放挂载 */
            if (sd_check_en == 1)
            {
                if (sdmmc_get_status(card) != ESP_OK)
                {
                    sd_check_en = 0;
                }
            }
            else
            {
                if (sd_spi_init() == ESP_OK)
                {
                    if (sdmmc_get_status(card) == ESP_OK)
                    {
                        sd_check_en = 1;
                    }
                }
            }
            
            /* 例如：96*96*2/1536 = 12;分12次发送RGB数据 */
            for(j = 0; j < (pic->width * pic->height * 2 / LCD_BUF_SIZE); j++)
            {
                /* &lcd_buf[j * LCD_BUF_SIZE] 偏移地址发送数据 */
                lcd_write_data(&rgb565[j * LCD_BUF_SIZE] , LCD_BUF_SIZE);
            }

            esp_camera_fb_return(pic);
        }
        else
        {
            ESP_LOGE(TAG, "Get frame failed");
        }

        i = 0;
        pic = NULL;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    free(rgb565);
}

/**
 * @brief       task3
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void task3(void *pvParameters)
{
    pvParameters = pvParameters;
    char file_name[30];
    uint32_t pictureNumber = 0;
    uint8_t res = 0;
    size_t writelen = 0;
    FIL *fftemp;
    res = exfuns_init();                                            /* 为fatfs相关变量申请内存 */
    pictureNumber = pic_get_tnum("0:/PICTURE");                     /* 得到总有效文件数 */
    pictureNumber = pictureNumber + 1;

    while (1)
    {
        xSemaphoreTake(BinarySemaphore, portMAX_DELAY);             /* 获取二值信号量 */
        
        /* SD卡挂载了，才能拍照 */
        if (sd_check_en == 1)
        {
            sprintf(file_name, "0:/PICTURE/img%ld.jpg", pictureNumber);
            fftemp = (FIL *)malloc(sizeof(FIL));                    /* 分配内存 */
            res = f_open(fftemp, (const TCHAR *)file_name, FA_WRITE | FA_CREATE_NEW);   /* 尝试打开 */

            if (res != FR_OK)
            {
                ESP_LOGE(TAG, "img open err\r\n");
            }

            f_write(fftemp, (const void *)lcd_buf, sizeof(lcd_buf), &writelen); /* 写入头数据 */

            if (writelen != sizeof(lcd_buf))
            {
                ESP_LOGE(TAG, "img Write err");
            }
            else
            {
                ESP_LOGI(TAG, "write buff len %d byte", writelen);
                pictureNumber++;
            }

            f_close(fftemp);
            free(fftemp);
        }
    }
}