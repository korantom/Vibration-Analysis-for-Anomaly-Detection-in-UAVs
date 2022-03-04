#ifndef __DISK_H__
#define __DISK_H__

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>

#include <storage/disk_access.h>
#include <fs/fs.h>
#include <ff.h>

#include <stdio.h>

/* -------------------------------------------------------------------------- */

#define DISK_MOUNT_PT "/SD:"
#define MAKE_DISK_PATH(P) DISK_MOUNT_PT "/" P
// extern const char *disk_mount_pt;

/* -------------------------------------------------------------------------- */

/**
 * @brief  mount disk wrapper
 * @retval 0 on success;
 * @retval <0 on error.
 */
int mount_disk();

/**
 * @brief  unmount disk wrapper
 * @retval 0 on success;
 * @retval <0 on error.
 */
int unmount_disk();

/* -------------------------------------------------------------------------- */

/**
 * @brief  read disk config
 * @retval 0 on success;
 * @retval <0 on error.
 */
int test_disk_config();

/* -------------------------------------------------------------------------- */
/**
 * @brief List contents (dirs, files) of a directory including sizes
 * @param path Path to the file or directory
 * @retval 0 on success;
 * @retval <0 on error.
 */
int list_dir(const char *path);
/* -------------------------------------------------------------------------- */
/**
 * @brief List contents (dirs, files) of a directory including sizes
 * @param path Path to the file or directory
 * @retval 0 on success;
 * @retval <0 on error.
 */
int list_dir(const char *path);
/* -------------------------------------------------------------------------- */
/**
 * @brief  write to a file (create if non-existant)
 * @param path Path to the file
 * @param buf  Buffer with data
 * @param size number of bytes to write
 * @retval >0 number of bytes written
 * @retval <0 on error.
 */
int write(const char *path, const void *buf, size_t size);

/* -------------------------------------------------------------------------- */
/**
 * @brief List contents (dirs, files) of a directory including sizes
 */
void test_write();
/* -------------------------------------------------------------------------- */

#endif // __DISK_H__
