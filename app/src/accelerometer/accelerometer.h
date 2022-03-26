#ifndef __ACCELEROMETER_H__
#define __ACCELEROMETER_H__

#include <device.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */
// TODO: move ringbuffer logic out from lis2dh12 to accelerometer?
/* -------------------------------------------------------------------------- */
/* TODO: will be set to true if occured during ... */
extern bool fifo_overrun;
extern bool ring_buffer_insufficient_memory;
/* -------------------------------------------------------------------------- */

/** @brief init and config the lis2dh12 sensor, set up callback, preprare for reading data */
void config_accelerometer();

/**
 * @brief   Enables accelerometer, resets fifo_overrun and ring_buffer_insufficient_memory (sets to false)
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_accelerometer(void);

/**
 * @brief   Disables accelerometer
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_accelerometer(void);

#endif //__ACCELEROMETER_H__
