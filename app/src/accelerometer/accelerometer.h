#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#include <device.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */

/** TODO: Move to common.h */
#define SERVICE_STACK_SIZE 1024
#define SERVICE_THREAD_PRIORITY -1
#define SERVICE_SYS_INIT_PRIORITY 90

/* -------------------------------------------------------------------------- */

/** @brief set up all necessary ... for the service to run */
void config_service();

/**
 * @brief   Enables service
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_service(void);

/**
 * @brief   Disables service
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_service(void);

#endif //__ACCELEROMETER_H__
