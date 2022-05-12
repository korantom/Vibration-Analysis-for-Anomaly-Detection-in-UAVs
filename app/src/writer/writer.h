#ifndef __WRITER_H__
#define __WRITER_H__

#include <device.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */

/** @brief ... */
void config_writer();

/**
 * @brief   Enables writer
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_writer(const char *file_name);

/**
 * @brief   Disables writer
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_writer(void);

/** @brief Dump all of ring buffer content into a file */
void dump_all(const char *file_name);

#endif //__WRITER_H__
