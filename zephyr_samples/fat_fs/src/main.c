/*
 * Copyright (c) 2019 Tavish Naruka <tavishnaruka@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Sample which uses the filesystem API and SDHC driver */

#include <zephyr.h>
#include <device.h>
#include <storage/disk_access.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <ff.h>

LOG_MODULE_REGISTER(main);

static int lsdir(const char *path);

static FATFS fat_fs;
/* mounting info */
static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
};

/*
*  Note the fatfs library is able to mount only strings inside _VOLUME_STRS
*  in ffconf.h
*/
static const char *disk_mount_pt = "/SD:";

/* -------------------------------------------------------------------------- */
void disk_io_test()
{
	do {
		static const char *disk_pdrv = "SD";
		uint64_t memory_size_mb;
		uint32_t block_count;
		uint32_t block_size;

		if (disk_access_init(disk_pdrv) != 0) {
			LOG_ERR("Storage init ERROR!");
			break;
		}

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_COUNT, &block_count)) {
			LOG_ERR("Unable to get sector count");
			break;
		}
		LOG_INF("Block count %u", block_count);

		if (disk_access_ioctl(disk_pdrv, DISK_IOCTL_GET_SECTOR_SIZE, &block_size)) {
			LOG_ERR("Unable to get sector size");
			break;
		}
		LOG_INF("Sector size %u", block_size);

		memory_size_mb = (uint64_t)block_count * block_size;
		LOG_INF("Memory Size(MB) %u", (uint32_t)(memory_size_mb >> 20));
	} while (0);
}
/* -------------------------------------------------------------------------- */
int disk_mount()
{
	LOG_INF("disk_mount()");
	mp.mnt_point = disk_mount_pt;
	int res = fs_mount(&mp);

	if (res == FR_OK) {
		LOG_INF("Disk mounted.");
	} else {
		LOG_ERR("Disk mounting error=%d", res);
	}
	return res;
}
/* -------------------------------------------------------------------------- */
int disk_unmount()
{
	LOG_INF("disk_unmount()");
	mp.mnt_point = disk_mount_pt;
	int res = fs_unmount(&mp);

	if (res == 0) {
		LOG_INF("Disk unmounted.");
	} else {
		LOG_ERR("Disk unmounting error=%d", res);
	}
	return res;
}
/* -------------------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>

static struct fs_file_t file;

int disk_open_file(const char *path)
{
	LOG_INF("disk_open_file()");

	fs_mode_t flags = FS_O_WRITE | FS_O_CREATE;
	fs_file_t_init(&file);
	int res = 0;

	res = fs_open(&file, path, flags);
	if (res != 0) {
		LOG_ERR("file %s not created/opened. err=%d", log_strdup(path), res);
		return res;
	}
	LOG_INF("file %s created.", log_strdup(path));

	return 0;
}

int disk_close_file(void)
{
	LOG_INF("disk_close_file()");

	int res = fs_close(&file);
	if (res != 0) {
		LOG_ERR("file not closed. err=%d", res);
		return res;
	}
	LOG_INF("file closed");

	return 0;
}

/* -------------------------------------------------------------------------- */
static char out_str[128];

int write()
{
	int res = 0;
	for (int j = 0; j < 30; j++) {
		int written = snprintf(out_str, sizeof(out_str) - 1, "%.6lf, %.6lf, %.6lf\r\n",
				       j * 1.0, j * -1.0, j * 1.0);
		res = fs_write(&file, out_str, written);
		if (res < 0)
			break;
	}
	return res;
}
/* -------------------------------------------------------------------------- */
int sequential_write_test_0()
{
	int res = 0;
	res = disk_open_file("/SD:/test0.csv");
	res = write();
	res = disk_close_file();
	return res;
}
/* -------------------------------------------------------------------------- */
int sequential_write_test_1()
{
	int res = 0;
	res = disk_open_file("/SD:/test1.csv");

	for (int i = 0; i < 1000; i++) {
		LOG_INF("%d\n", i);
		res = write();
		if (res < 0)
			break;
	}

	res = disk_close_file();
	return res;
}
/* -------------------------------------------------------------------------- */
int sequential_write_test_2()
{
	int res = 0;
	res = disk_open_file("/SD:/test2.csv");

	for (int i = 0; i < 1000000; i++) {
		if (i % 100 == 0) {
			LOG_INF("%d\n", i);
		}
		res = write();
		if (res < 0) {
			LOG_INF("%d\n", i);
			break;
		}
	}

	res = disk_close_file();
	return res;
}
/* -------------------------------------------------------------------------- */
int sequential_write_test_3()
{
	int res = 0;

	for (int i = 0; i < 1000000; i++) {
		res = disk_open_file("/SD:/test3.csv");
		if (i % 100 == 0) {
			LOG_INF("%d\n", i);
		}
		res = write();
		if (res < 0) {
			LOG_INF("%d\n", i);
			break;
		}
		res = disk_close_file();
	}

	return res;
}
/* -------------------------------------------------------------------------- */
int sequential_write_test_4()
{
	int res = 0;

	for (int i = 0; i < 1000000; i++) {
		res = disk_mount();
		res = disk_open_file("/SD:/test4.csv");
		if (i % 100 == 0) {
			LOG_INF("%d\n", i);
		}
		res = write();
		if (res < 0) {
			LOG_INF("%d\n", i);
			break;
		}
		res = disk_close_file();
		res = disk_unmount();
	}

	return res;
}
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
void main(void)
{
	/* raw disk i/o */
	disk_io_test();

	disk_mount();

	lsdir(disk_mount_pt);

	sequential_write_test_4();

	lsdir(disk_mount_pt);

	disk_unmount();
}

static int lsdir(const char *path)
{
	int res;
	struct fs_dir_t dirp;
	static struct fs_dirent entry;

	fs_dir_t_init(&dirp);

	/* Verify fs_opendir() */
	res = fs_opendir(&dirp, path);
	if (res) {
		printk("Error opening dir %s [%d]\n", path, res);
		return res;
	}

	printk("\nListing dir %s ...\n", path);
	for (;;) {
		/* Verify fs_readdir() */
		res = fs_readdir(&dirp, &entry);

		/* entry.name[0] == 0 means end-of-dir */
		if (res || entry.name[0] == 0) {
			break;
		}

		if (entry.type == FS_DIR_ENTRY_DIR) {
			printk("[DIR ] %s\n", entry.name);
		} else {
			printk("[FILE] %s (size = %zu)\n", entry.name, entry.size);
		}
	}
	printk("[\n\n");

	/* Verify fs_closedir() */
	fs_closedir(&dirp);

	return res;
}
