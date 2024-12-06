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