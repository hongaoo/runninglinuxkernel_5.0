/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __LIBBMP_H__
#define __LIBBMP_H__

// BMP文件头结构
struct bmp_file_header {
    uint16_t bfType;       // 文件类型，必须为"BM"，即0x4D42
    uint32_t bfSize;       // 文件大小
    uint16_t bfReserved1;  // 保留，必须为0
    uint16_t bfReserved2;  // 保留，必须为0
    uint32_t bfOffBits;    // 图像数据偏移量
};

// DIB头结构
struct bmp_info_header {
    uint32_t biSize;           // 本结构的大小
    int32_t  biWidth;          // 图像宽度（像素）
    int32_t  biHeight;         // 图像高度（像素）
    uint16_t biPlanes;         // 目标设备平面数，必须为1
    uint16_t biBitCount;       // 每个像素的位数
    uint32_t biCompression;    // 压缩类型
    uint32_t biSizeImage;      // 图像数据大小
    int32_t  biXPelsPerMeter;  // 水平分辨率
    int32_t  biYPelsPerMeter;  // 垂直分辨率
    uint32_t biClrUsed;        // 使用的颜色数
    uint32_t biClrImportant;   // 重要颜色数
};

int save_bmp(const char *filename, uint8_t *data, int width, int height);
#endif