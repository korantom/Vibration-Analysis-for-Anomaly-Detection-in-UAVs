#include "disk.h"

/* -------------------------------------------------------------------------- */

LOG_MODULE_REGISTER(disk_);

static bool is_file_opened = false;
static char opened_file[50];

static struct fs_file_t file;
static const char *disk_mount_pt = DISK_MOUNT_PT;

/* mounting info */
static FATFS fat_fs;
static struct fs_mount_t mp = {
    .type = FS_FATFS,
    .fs_data = &fat_fs,
};

/* -------------------------------------------------------------------------- */

int disk_mount()
{
    LOG_INF("disk_mount()");
    mp.mnt_point = disk_mount_pt;
    int res = fs_mount(&mp);

    if (res == FR_OK)
    {
        LOG_INF("Disk mounted. res=%d", res);
    }
    else
    {
        LOG_ERR("Disk mounting error=%d", res);
    }
    return -res;
}

int disk_unmount()
{
    LOG_INF("disk_unmount()");
    mp.mnt_point = disk_mount_pt;
    int res = fs_unmount(&mp);
    if (res == 0)
    {
        LOG_INF("Disk unmounted. res=%d", res);
    }
    else
    {
        LOG_ERR("Disk unmounting error=%d", res);
    }
    return -res;
}

/* -------------------------------------------------------------------------- */

int disk_test_config()
{
    LOG_INF("disk_test_config()");
    /* raw disk i/o */
    static const char *disk_pdrv = "SD";
    uint64_t memory_size_mb;
    uint32_t block_count;
    uint32_t block_size;

    int err = 0;

    err = disk_access_init(disk_pdrv);
    if (err)
    {
        LOG_ERR("Storage init ERROR!");
        return err;
    }

    err = disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_COUNT, &block_count);
    if (err)
    {
        LOG_ERR("Unable to get sector count");
        return err;
    }

    LOG_INF("Block count %u", block_count);

    err = disk_access_ioctl(disk_pdrv,
                            DISK_IOCTL_GET_SECTOR_SIZE, &block_size);
    if (err)
    {
        LOG_ERR("Unable to get sector size");
        return err;
    }

    LOG_INF("Sector size %u\n", block_size);

    memory_size_mb = (uint64_t)block_count * block_size;
    LOG_INF("Memory Size(MB) %u\n", (uint32_t)(memory_size_mb >> 20));

    return 0;
}

/* -------------------------------------------------------------------------- */

int disk_list_dir(const char *path)
{
    LOG_INF("disk_list_dir()");
    int res;
    struct fs_dir_t dirp;
    static struct fs_dirent entry;

    fs_dir_t_init(&dirp);

    /* Verify fs_opendir() */
    res = fs_opendir(&dirp, path);
    if (res)
    {
        LOG_ERR("Error opening dir %s [%d]", path, res);
        return res;
    }

    printk("Listing dir %s ...\n", path);
    for (;;)
    {
        /* Verify fs_readdir() */
        res = fs_readdir(&dirp, &entry);
        if (res)
        {
            LOG_ERR("Error reading dir %s [%d]", path, res);
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
        LOG_ERR("Error closing dir %s [%d]\n", path, res);
    }

    return res;
}

/* -------------------------------------------------------------------------- */

/** @brief example of hwo to write to a file */
static int disk_test_write_buf(const char *path, const void *buf, size_t size)
{
    LOG_INF("disk_test_write_buf()");

    struct fs_file_t file;
    fs_mode_t flags = FS_O_WRITE | FS_O_CREATE | FS_O_APPEND;
    int res = 0;

    fs_file_t_init(&file);

    res = fs_open(&file, path, flags);
    if (res != 0)
    {
        LOG_ERR("file not created/opened. err=%d", res);
        return res;
    }

    int w_size = fs_write(&file, buf, size);
    if (w_size > 0)
    {
        LOG_INF("%d bytes written\r\n", w_size);
    }
    else
    {
        LOG_ERR("err in writing. err=%d\n", w_size);
    }

    res = fs_close(&file);
    if (res != 0)
    {
        LOG_ERR("file not closed. err=%d\n", res);
        return res;
    }
    return w_size;
}

void disk_test_write()
{
    const char *path = DISK_MOUNT_PT "/"
                                     "test.csv";
    char out_str[100];
    int written;

    for (int i = 0; i < 3; i++)
    {
        written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n",
                           i * 1.0, i * 2.0, i * 3.0);

        // TODO: ineffective, (avoid opening and closing file)
        disk_test_write_buf(path, out_str, written);
    }
}

/* -------------------------------------------------------------------------- */

int disk_open_file(const char *path)
{
    LOG_INF("disk_open_file()");

    if (is_file_opened)
    {
        LOG_WRN("another file (%s) already opened", log_strdup(opened_file));
        return -1;
    }

    fs_mode_t flags = FS_O_WRITE | FS_O_CREATE;
    fs_file_t_init(&file);
    int res = 0;

    res = fs_open(&file, path, flags);
    if (res != 0)
    {
        LOG_ERR("file %s not created/opened. err=%d", log_strdup(path), res);
        return res;
    }

    LOG_INF("file %s created/opened", log_strdup(path));
    is_file_opened = true;
    strcpy(opened_file, path);

    return 0;
}

int disk_close_file(void)
{
    LOG_INF("disk_close_file()");

    int res = fs_close(&file);
    if (res != 0)
    {
        LOG_ERR("file %s not closed. err=%d", log_strdup(opened_file), res);
        return res;
    }

    LOG_INF("file %s closed", log_strdup(opened_file));
    is_file_opened = false;

    return 0;
}

int disk_write_file(const void *buf, size_t size)
{
    LOG_INF("disk_write_file()");

    int w_size = fs_write(&file, buf, size);
    if (w_size > 0)
    {
        LOG_INF("%d bytes written\r\n", w_size);
    }
    else
    {
        LOG_ERR("err in writing. err=%d\n", w_size);
    }

    return w_size;
}

int disk_flush()
{
    return fs_sync(&file);
}
/* -------------------------------------------------------------------------- */

int disk_create_folder(const char *dir_name)
{

    char path[100];

    snprintf(path, sizeof(path), "%s/%s", DISK_MOUNT_PT, dir_name);
    int res = fs_mkdir(path);

    return res;
}

/* -------------------------------------------------------------------------- */

// void test()
// {
//     disk_test_config();
//     disk_mount();
//     disk_list_dir(DISK_MOUNT_PT);
//     disk_test_write();
//     disk_test_write();
//     disk_list_dir(DISK_MOUNT_PT);
//     disk_unmount();
// }
