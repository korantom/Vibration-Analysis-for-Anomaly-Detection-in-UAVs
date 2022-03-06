#ifndef __DISK_H__
#define __DISK_H__

#include <device.h>
#include <fs/fs.h>
#include <ff.h>
#include <logging/log.h>
#include <stdio.h>
#include <storage/disk_access.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../../common.h"

/* -------------------------------------------------------------------------- */

#define DISK_MOUNT_PT "/SD:"
#define MAKE_DISK_PATH(P) DISK_MOUNT_PT "/" P

/* -------------------------------------------------------------------------- */

/**
 * @brief  mount disk wrapper
 * @retval   0 on success
 * @retval < 0 on error
 */
int disk_mount();

/**
 * @brief  unmount disk wrapper
 * @retval   0 on success
 * @retval < 0 on error
 */
int disk_unmount();

/* -------------------------------------------------------------------------- */

/**
 * @brief  read disk config
 * @retval   0 on success
 * @retval < 0 on error
 */
int disk_test_config();

/* -------------------------------------------------------------------------- */

/**
 * @brief List contents (dirs, files) of a directory
 * @param path Path to the file or directory
 * @retval   0 on success
 * @retval < 0 on error
 */
int disk_list_dir(const char *path);

/* -------------------------------------------------------------------------- */

/** @brief Write dummy data to test.csv */
void disk_test_write();

/* -------------------------------------------------------------------------- */
/* Assumes writing to 1 file at max simultaneously */

/**
 * @brief open a file for writing, TODO: flags: append, ....
 * @param path Path to the file or directory
 * @retval   0 on success
 * @retval < 0 on error (cant open file, another file already opened)
 */
int disk_open_file(const char *path);

/**
 * @brief close a file
 * @retval   0 on success
 * @retval < 0 on error
 */
int disk_close_file(void);

/**
 * @brief  write to a file (assumes file opened already, disk_open_file called)
 * @param buf buffer with data
 * @param size number of bytes to write (buffer size)
 * @retval > 0 number of bytes written
 * @retval < 0 on error
 */
int disk_write_file(const void *buf, size_t size);

/* -------------------------------------------------------------------------- */

#endif // __DISK_H__
