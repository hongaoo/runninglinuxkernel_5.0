#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "libbmp.h"

// 保存BMP文件
int save_bmp(const char *filename, uint8_t *data, int width, int height) {
    struct bmp_file_header file_header;
    struct bmp_info_header info_header;
    struct file *file;
    mm_segment_t old_fs;
    ssize_t bytes_written;
    int y, x;  // 将变量声明移到循环之外

    // 初始化BMP文件头
    file_header.bfType = 0x4D42;  // "BM"
    file_header.bfSize = sizeof(struct bmp_file_header) + sizeof(struct bmp_info_header) + width * height * 3;
    file_header.bfReserved1 = 0;
    file_header.bfReserved2 = 0;
    file_header.bfOffBits = sizeof(struct bmp_file_header) + sizeof(struct bmp_info_header);

    // 初始化DIB头
    info_header.biSize = sizeof(struct bmp_info_header);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;  // 24位真彩色
    info_header.biCompression = 0; // 不压缩
    info_header.biSizeImage = width * height * 3;
    info_header.biXPelsPerMeter = 2835; // 72 DPI
    info_header.biYPelsPerMeter = 2835; // 72 DPI
    info_header.biClrUsed = 0;
    info_header.biClrImportant = 0;

    // 打开文件
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    file = filp_open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ERR "Could not open file for writing: %s\n", filename);
        set_fs(old_fs);
        return -1;
    }

    // 写入文件头和信息头
    bytes_written = vfs_write(file, (char *)&file_header, sizeof(file_header), &file->f_pos);
    if (bytes_written != sizeof(file_header)) {
        printk(KERN_ERR "Failed to write file header\n");
        filp_close(file, NULL);
        set_fs(old_fs);
        return -1;
    }

    bytes_written = vfs_write(file, (char *)&info_header, sizeof(info_header), &file->f_pos);
    if (bytes_written != sizeof(info_header)) {
        printk(KERN_ERR "Failed to write info header\n");
        filp_close(file, NULL);
        set_fs(old_fs);
        return -1;
    }

    // 写入图像数据
    for (y = height - 1; y >= 0; y--) {
        for (x = 0; x < width; x++) {
            int index = (y * width + x) * 3;
            bytes_written = vfs_write(file, (char *)&data[index], 3, &file->f_pos);
            if (bytes_written != 3) {
                printk(KERN_ERR "Failed to write pixel data\n");
                filp_close(file, NULL);
                set_fs(old_fs);
                return -1;
            }
        }
    }

    filp_close(file, NULL);
    set_fs(old_fs);
    return 0;
}
EXPORT_SYMBOL(save_bmp);


// 处理XRGB数组并保存为BMP文件
void save_xrgb_as_bmp(const char *filename, unsigned char *xrgb_data, int width, int height) {
    unsigned int padsize = (4 - (width * 3) % 4) % 4; // 计算行填充
    unsigned int imagesize = width * height * 3;
    unsigned int filesize = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + imagesize + height * padsize;
    int i ,j, ret;
    mm_segment_t old_fs;

    BMPFileHeader file_header = {
        .header = {'B', 'M'},
        //.filesize = filesize,
        .filesize = imagesize,
        .reserved = 0,
        .offset = 54
    };

    BMPInfoHeader info_header = {
        .size = 40,
        .width = width,
        .height = height,
        .planes = 1,
        .bitcount = 24,
        .compression = 0,
        .imagesize = imagesize,
        .xresolution = 0, // 2835 dpi
        .yresolution = 0,
        .colors = 0,
        .importantcolors = 0
    };

    struct file *file;
    loff_t pos = 0;
    char *data = (char *)vmalloc(filesize);
    if (!data) {
        printk(KERN_ALERT "xrgb_to_bmp: Unable to allocate memory for BMP data\n");
        return;
    }

    // 拷贝文件头
    memcpy(data, &file_header, sizeof(file_header));
    pos += sizeof(file_header);

    // 拷贝信息头
    memcpy(data + pos, &info_header, sizeof(info_header));
    pos += sizeof(info_header);

    // 拷贝图像数据并转换为BGR
    for (i = height - 1; i >= 0; i--) { // BMP从底部开始存储
        for (j = 0; j < width; j++) {
            unsigned char alpha = xrgb_data[(i * width + j) * 4 + 0];
            unsigned char red = xrgb_data[(i * width + j) * 4 + 1];
            unsigned char green = xrgb_data[(i * width + j) * 4 + 2];
            unsigned char blue = xrgb_data[(i * width + j) * 4 + 3];

            data[pos++] = blue; // B
            data[pos++] = green; // G
            data[pos++] = red;   // R
        }
        // 写入填充字节
        for (j = 0; j < padsize; j++) {
            data[pos++] = 0;
        }
    }

    // 创建并写入文件
    file = filp_open(filename, O_CREAT | O_WRONLY, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ALERT "xrgb_to_bmp: Unable to open file %s\n", filename);
        vfree(data);
        return;
    }

    // 设置文件指针
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    // vfs_write(file, data, filesize, &pos);
    pos = 0;
    // 写入图像数据
    ret = vfs_write(file, (const char __user *)data, filesize, &pos);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to write PGM data\n");
        goto cleanup;
    }
    
    filp_close(file, NULL);

    // 恢复文件指针
    set_fs(old_fs);

    vfree(data);
    printk(KERN_INFO "xrgb_to_bmp: Image saved as %s\n", filename);
    return 0;

cleanup:
    filp_close(file, NULL);
    set_fs(old_fs);
    vfree(data);
    return ret;
}
EXPORT_SYMBOL(save_xrgb_as_bmp);

// static int __init bmp_save_init(void) {
//     const int width = 8;
//     const int height = 8;
//     uint8_t data[width * height * 3] = {
//         255, 0, 0,   0, 255, 0,   0, 0, 255,   255, 255, 0,   0, 255, 255,   255, 0, 255,   255, 255, 255,   0, 0, 0,
//         0, 255, 0,   255, 0, 0,   255, 255, 0,   0, 0, 255,   255, 0, 255,   0, 255, 255,   0, 0, 0,      255, 255, 255,
//         0, 0, 255,   255, 255, 0,   0, 255, 0,   255, 0, 255,   0, 255, 255,   0, 0, 0,      255, 255, 255,   0, 0, 0,
//         255, 255, 0,   0, 0, 255,   255, 0, 0,   255, 255, 0,   0, 0, 255,   255, 255, 255,   0, 0, 0,      255, 255, 255,
//         0, 255, 255,   255, 0, 255,   0, 255, 255,   0, 0, 255,   255, 255, 0,   0, 0, 0,      255, 255, 255,   0, 0, 0,
//         255, 0, 255,   0, 255, 255,   255, 0, 0,   255, 255, 0,   0, 0, 255,   255, 255, 255,   0, 0, 0,      255, 255, 255,
//         0, 255, 255,   255, 0, 0,   255, 255, 0,   0, 0, 255,   255, 255, 0,   0, 0, 0,      255, 255, 255,   0, 0, 0,
//         255, 255, 0,   0, 255, 255,   255, 0, 0,   255, 255, 0,   0, 0, 255,   255, 255, 255,   0, 0, 0,      255, 255, 255
//     };

//     if (save_bmp("/tmp/output.bmp", data, width, height) == 0) {
//         printk(KERN_INFO "BMP file saved as /tmp/output.bmp\n");
//     } else {
//         printk(KERN_ERR "Failed to save BMP file\n");
//     }

//     return 0;
// }

// static void __exit bmp_save_exit(void) {
//     printk(KERN_INFO "BMP save module unloaded\n");
// }

// module_init(bmp_save_init);
// module_exit(bmp_save_exit);

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Your Name");
// MODULE_DESCRIPTION("A module to save an array as a BMP file");