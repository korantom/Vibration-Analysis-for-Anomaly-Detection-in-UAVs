#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#include <device.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */

/** @brief init and config the lis2dh12 sensor, set up callback, preprare for reading data */
void config_accelerometer();

/**
 * @brief   Enables accelerometer
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_accelerometer(void);

/**
 * @brief   Disables accelerometer
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_accelerometer(void);

#endif //__ACCELEROMETER_H__
