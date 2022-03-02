#include "disk.h"

/* -------------------------------------------------------------------------- */

static struct fs_file_t file;
static const char *disk_mount_pt = "/SD:";

/* mounting info */
static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

/* -------------------------------------------------------------------------- */

int mount_disk()
{
    mp.mnt_point = disk_mount_pt;
    int res = fs_mount(&mp);

    if (res == FR_OK)
    {
        printk("Disk mounted. res=%d\n", res);
    }
    else
    {
        printk("Error mounting disk. res=%d\n", res);
    }
    return -res;
}

int unmount_disk()
{
    mp.mnt_point = disk_mount_pt;
    int res = fs_unmount(&mp);
    if (res == 0)
    {
        printk("Disk unmounted. res=%d\n", res);
    }
    else
    {
        printk("Error unmounting disk. res=%d\n", res);
    }
    return -res;
}

/* -------------------------------------------------------------------------- */

int test_disk_config()
{
    /* raw disk i/o */
    static const char *disk_pdrv = "SD";
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    int err = 0;

    err = disk_access_init(disk_pdrv);
    if (err)
    {
        printk("Storage init ERROR!\n");
        return err;
    }

    err = disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
    if (err)
    {
        printk("Unable to get sector count\n");
        return err;
    }

    printk("Block count %u", block_count);

    err = disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
    if (err)
    {
        printk("Unable to get sector size\n");
        return err;
    }

    printk("Sector size %u\n", block_size);

    memory_size_mb = (uint64_t)block_count * block_size;
    printk("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

    return 0;
}

/* -------------------------------------------------------------------------- */

int list_dir(const char *path)
{
    // TODO: pass by ref list of all files/dirs
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, path);
    if (res)
    {
        printk("Error opening dir %s [%d]\n", path, res);
        return res;
    }

    printk("\nListing dir %s ...\n", path);
    for (;;)
    {
        /* Verify fs_readdir() */
        res = fs_readdir(&dirp, &entry);
        if (res)
        {
            printk("Error reading dir %s [%d]\n", path, res);
            break;
        }

        /* entry.name[0] == 0 means end-of-dir */
        if (entry.name[0] == 0)
        {
            break;
        }

        if (entry.type == FS_DIR_ENTRY_DIR)
        {
            printk("[DIR ] %s\n", entry.name);
        }
        else
        {
            printk("[FILE] %s (size = %zu)\n",
                   entry.name, entry.size);
        }
    }

    /* Verify fs_closedir() */
    res = fs_closedir(&dirp);
    if (res)
    {
        printk("Error closing dir %s [%d]\n", path, res);
    }

    return res;
}

/* -------------------------------------------------------------------------- */

int write(const char *path, const void *buf, size_t size);

void test_write()
{
    const char *path = DISK_MOUNT_PT "/" "test.csv";
    char out_str[100];
    int written;

    for (int i = 0; i < 32; i++)
    {
        written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n",
                           i * 1.0, i * 2.0, i * 3.0);

        // TODO: ineffective, (avoid opening and closing file)
        write(path, out_str, written);
    }
}
/* -------------------------------------------------------------------------- */

int write(const char *path, const void *buf, size_t size)
{
    // TODO: diff function for multiple sequential writes (avoid opening and closing file)
    // TODO: select append or not
    fs_mode_t flags = FS_O_WRITE | FS_O_CREATE | FS_O_APPEND;
    int res = 0;

    // TODO: once is enough, during mount?
    fs_file_t_init(&file);

    res = fs_open(&file, path, flags);
    if (res != 0)
    {
        printk("file not created. err=%d\n", res);
        return res;
    }

    int w_size = fs_write(&file, buf, size);
    if (w_size > 0)
    {
        printk("%d bytes written\r\n", w_size);
    }
    else
    {
        printk("err in writing. err=%d\n", w_size);
    }

    res = fs_close(&file);
    if (res != 0)
    {
        printk("file not closed. err=%d\n", res);
        return res;
    }
    return w_size;
}

/* -------------------------------------------------------------------------- */

void test()
{

    test_disk_config();

    mount_disk();

    list_dir(DISK_MOUNT_PT);

    test_write();

    list_dir(DISK_MOUNT_PT);

    test_write();

    list_dir(DISK_MOUNT_PT);

    unmount_disk();
}
