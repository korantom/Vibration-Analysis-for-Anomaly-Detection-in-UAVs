#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#include <zephyr.h>
#include <sys/printk.h>
#include <shell/shell.h>
#include <device.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */

/** @brief set up all necessary ... for the service to run */
void config_accelerometer();

/**
 * @brief   Enables service
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_accelerometer(void);

/**
 * @brief   Disables service
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_accelerometer(void);

#endif //__ACCELEROMETER_H__
