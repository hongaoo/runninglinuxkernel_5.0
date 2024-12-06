#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "libpgm.h"

/**
 * save_pgm_to_file - 保存PGM格式的图像数据到文件
 * @filename: 保存的文件名
 * @data: 图像数据数组
 * @width: 图像宽度
 * @height: 图像高度
 *
 * 返回值: 成功返回0，失败返回负的错误码
 */
int save_pgm_to_file(const char *filename, uint8_t *data, int width, int height) {
    struct file *file;
    loff_t pos = 0;
    mm_segment_t old_fs;
    char *header;
    int header_len, ret;

    // 分配内存用于文件头
    header_len = 100; // 足够大的缓冲区
    header = kmalloc(header_len, GFP_KERNEL);
    if (!header) {
        printk(KERN_ALERT "Failed to allocate memory for PGM header\n");
        return -ENOMEM;
    }

    // 构建PGM文件头
    snprintf(header, header_len, "P5\n%d %d\n255\n", width, height);

    // 打开文件
    file = filp_open(filename, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    if (IS_ERR(file)) {
        printk(KERN_ALERT "Failed to open file: %s\n", filename);
        kfree(header);
        return PTR_ERR(file);
    }

    // 设置文件指针
    old_fs = get_fs();
    set_fs(KERNEL_DS);

    // 写入文件头
    ret = vfs_write(file, (const char __user *)header, strlen(header), &pos);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to write PGM header\n");
        goto cleanup;
    }

    // 写入图像数据
    ret = vfs_write(file, (const char __user *)data, width * height, &pos);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to write PGM data\n");
        goto cleanup;
    }

    // 关闭文件
    filp_close(file, NULL);

    // 恢复文件指针
    set_fs(old_fs);

    kfree(header);
    return 0;

cleanup:
    filp_close(file, NULL);
    set_fs(old_fs);
    kfree(header);
    return ret;
}
EXPORT_SYMBOL(save_pgm_to_file);
