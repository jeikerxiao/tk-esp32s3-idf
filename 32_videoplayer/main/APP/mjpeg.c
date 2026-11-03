/**
 ****************************************************************************************************
 * @file        mjpeg.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2023-12-01
 * @brief       MJPEG视频处理 代码
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

#include "mjpeg.h"


struct jpeg_decompress_struct *cinfo;
struct my_error_mgr *jerr;
int Windows_Width = 0;
int Windows_Height = 0;
uint16_t imgoffx, imgoffy;                  /* 图像在x,y方向的偏移量 */
typedef struct my_error_mgr* my_error_ptr;

/**
 * @brief       错误退出
 * @param       cinfo   : JPEG编码解码控制结构体
 * @retval      无
 */
METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message) (cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/**
 * @brief       发出消息
 * @param       cinfo       : JPEG编码解码控制结构体
 * @param       msg_level   : 消息等级
 * @retval      无
 */
METHODDEF(void) my_emit_message(j_common_ptr cinfo, int msg_level)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    if (msg_level < 0)
    {
        printf("emit msg:%d\r\n", msg_level);
        longjmp(myerr->setjmp_buffer, 1);
    }
}

static portMUX_TYPE my_spinlock = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief       解码一副JPEG图片
 * @param       buf: jpeg数据流数组
 * @param       bsize: 数组大小
 * @retval      0,成功; 1,失败
 */
uint8_t mjpegdec_decode(uint8_t* buf, uint32_t bsize)
{

    JSAMPARRAY buffer;
    if (bsize == 0) return 1;
    int row_stride = 0;
    int j = 0;                      /* 记录当前解码的行数 */
    int lineR = 0;                  /* 每一行R分量的起始位置 */
    cinfo->err = jpeg_std_error(&jerr->pub);
    jerr->pub.error_exit = my_error_exit;
    jerr->pub.emit_message = my_emit_message;
    cinfo->out_color_space = JCS_RGB;

    if (setjmp(jerr->setjmp_buffer)) /* 错误处理 */
    {
        jpeg_abort_decompress(cinfo);
        jpeg_destroy_decompress(cinfo);
        return 2;
    }

    jpeg_create_decompress(cinfo);

    jpeg_mem_src(cinfo, buf, bsize);    /* 测试正常 */
    jpeg_read_header(cinfo, TRUE);

    jpeg_start_decompress(cinfo); 

    row_stride = cinfo->output_width * cinfo->output_components;

    /* 计算buffer大小并申请相应空间 */
    buffer = (*cinfo->mem->alloc_sarray)
        ((j_common_ptr)cinfo, JPOOL_IMAGE, row_stride, 1);
    
    while (cinfo->output_scanline < cinfo->output_height)
    {
        int i = 0;

        jpeg_read_scanlines(cinfo, buffer, 1);
        unsigned short tmp_color565;

        /* 为上述图像数据赋值 */
        for (int k = 0; k < Windows_Width * 2; k += 2)
        {
            tmp_color565 = rgb565(buffer[0][i],buffer[0][i + 1],buffer[0][i + 2]);
            lcd_buf[lineR + k] = (tmp_color565 & 0xFF00) >> 8;
            lcd_buf[lineR + k + 1] =  tmp_color565 & 0x00FF;

            i += 3;
        }
        
        j++;
        lineR = j * Windows_Width * 2;
    }

    lcd_set_window(imgoffx, imgoffy - 30, imgoffx + cinfo->output_width - 1, imgoffy - 30 + cinfo->output_height - 1);
    taskENTER_CRITICAL(&my_spinlock);
    /* 例如：96*96*2/1536 = 12;分12次发送RGB数据 */
    for(int x = 0; x < (cinfo->output_width * cinfo->output_height * 2 / LCD_BUF_SIZE); x++)
    {
        /* &lcd_buf[j * LCD_BUF_SIZE] 偏移地址发送数据 */
        lcd_write_data(&lcd_buf[x * LCD_BUF_SIZE] , LCD_BUF_SIZE);
    }
    
    taskEXIT_CRITICAL(&my_spinlock);
    lcd_set_window(0, 0, lcd_self.width, lcd_self.height);  /* 恢复窗口 */
    jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);
    return 0;
}

/**
 * @brief       mjpeg 解码初始化
 * @param       offx,offy:x,y方向的偏移
 * @retval      0,成功; 1,失败
 */
char mjpegdec_init(uint16_t offx, uint16_t offy)
{
    cinfo = (struct jpeg_decompress_struct *)malloc(sizeof(struct jpeg_decompress_struct));
    jerr = (struct my_error_mgr *)malloc(sizeof(struct my_error_mgr));

    if (cinfo == NULL || jerr == NULL)
    {
        printf("[E][mjpeg.cpp] mjpegdec_init(): malloc failed to apply for memory\r\n");
        mjpegdec_free();
        return -1;
    }

    /* 保存图像在x,y方向的偏移量 */
    imgoffx = offx;
    imgoffy = offy;

    return 0;
}

/**
 * @brief       mjpeg结束,释放内存
 * @param       无
 * @retval      无
 */
void mjpegdec_free(void)
{
    free(cinfo);
    free(jerr);
}
