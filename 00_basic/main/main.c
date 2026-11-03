/**
 ******************************************************************************
 * @file     main.c
 * @author   正点原子团队(ALIENTEK)
 * @version  V1.0
 * @date     2023-08-26
 * @brief    ESP32-IDF基础工程
 * @license  Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ******************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 ESP32-S3 开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 ******************************************************************************
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include "esp_flash.h"


/**
 * @brief       程序入口
 * @param       无
 * @retval      无
 */
void app_main(void)
    {
    esp_err_t ret;
    uint32_t flash_size;
    esp_chip_info_t chip_info;                                      /* 定义芯片信息结构体变量 */

    ret = nvs_flash_init();                                         /* 初始化NVS */

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    esp_flash_get_size(NULL, &flash_size);                          /* 获取FLASH大小 */

    esp_chip_info(&chip_info);
    printf("内核：cup数量%d\n",chip_info.cores);                     /* 获取CPU内核数并显示 */
    printf("FLASH size:%ld MB flash\n",flash_size / (1024 * 1024)); /* 获取FLASH大小并显示 */
    printf("PSRAM size: %d bytes\n", esp_psram_get_size());         /* 获取PARAM大小并显示 */

    while(1)
    {
        printf("Hello-ESP32\r\n");
        vTaskDelay(1000);
    }
}
