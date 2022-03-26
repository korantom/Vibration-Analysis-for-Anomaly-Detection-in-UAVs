#ifndef __LIS2DH12_H__
#define __LIS2DH12_H__

#include <device.h>
#include <drivers/gpio.h>
#include <drivers/i2c.h>
#include <kernel.h>
#include <logging/log.h>
#include <nrfx.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/printk.h>
#include <sys/ring_buffer.h>
#include <zephyr.h>

#include "../../common.h"

/* -------------------------------------------------------------------------- */

#define LIS_ADDRESS 0x19

/* Register Addresses */
#define ADDR_WHO_AM_I 0x0F

#define ADDR_CTRL_REG0 0x1E
#define ADDR_TEMP_CFG_REG 0X1F
#define ADDR_CTRL_REG1 0x20
#define ADDR_CTRL_REG2 0x21
#define ADDR_CTRL_REG3 0x22
#define ADDR_CTRL_REG4 0x23
#define ADDR_CTRL_REG5 0x24
#define ADDR_CTRL_REG6 0x25

#define ADDR_REFERENCE 0x26

#define ADDR_STATUS_REG 0x27

#define ADDR_OUT_X_L 0x28
#define ADDR_OUT_X_H 0x29
#define ADDR_OUT_Y_L 0x2A
#define ADDR_OUT_Y_H 0x2B
#define ADDR_OUT_Z_L 0x2C
#define ADDR_OUT_Z_H 0x2D

#define ADDR_FIFO_CTRL_REG 0x2E
#define ADDR_FIFO_SRC_REG 0x2F

#define ADDR_INT1_CFG 0x30
#define ADDR_INT1_SRC 0x31
#define ADDR_INT1_THS 0x32
#define ADDR_INT1_DURATION 0x33

#define ADDR_INT2_CFG 0x34
#define ADDR_INT2_SRC 0x35
#define ADDR_INT2_THS 0x36
#define ADDR_INT2_DURATION 0x37

/* -------------------------------------------------------------------------- */
/* Register config masks */

/* Dummy read only register for testing purposes */
#define WHO_AM_I_DEFAULT_VALUE 00110011

/* Helper for setting up confgig options */
#define BIT_0 (1 << 0) // 0x01
#define BIT_1 (1 << 1) // 0x02
#define BIT_2 (1 << 2) // 0x04
#define BIT_3 (1 << 3) // 0x08

#define BIT_4 (1 << 4) // 0x10
#define BIT_5 (1 << 5) // 0x20
#define BIT_6 (1 << 6) // 0x40
#define BIT_7 (1 << 7) // 0x80

#define BIT_Z (0 << 0) // 0x00

/* CTRL_REG1 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Axis selection, power modes, sampling rate (OUTPUT DATA RATE)

/* Axes */
#define CTRL_REG1_XYZ_AXES_ENABLE (BIT_2 | BIT_1 | BIT_0)

/* Power modes */
#define CTRL_REG1_LOW_POWER_MODE_ENABLE (BIT_3)  // => low resolution
#define CTRL_REG1_LOW_POWER_MODE_DISABLE (BIT_Z) // => normal/high resolution
/* Note: resolution is determined by combined config of CTRL_REG1 LPen (Low Power enabled) bit and CTRL_REG4 HR (High Resolution) bit */

/* Output Data Rates (ODR) (Sampling rate), => ... bandwidth */
#define CTRL_REG1_OUTPUT_DATA_RATE_1_HZ (BIT_4)                   // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_10_HZ (BIT_5)                  // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_25_HZ (BIT_5 | BIT_4)          // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_50_HZ (BIT_6)                  // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_100_HZ (BIT_6 | BIT_4)         // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_200_HZ (BIT_6 | BIT_5)         // low/normal/high resolution
#define CTRL_REG1_OUTPUT_DATA_RATE_400_HZ (BIT_6 | BIT_5 | BIT_4) // low/normal/high resolution

#define CTRL_REG1_OUTPUT_DATA_RATE_1620_HZ (BIT_7)         // only in LOW_POWER_MODE & no HIGH RESOLUTION
#define CTRL_REG1_OUTPUT_DATA_RATE_1344_HZ (BIT_7 | BIT_4) // only in NORMAL_POWER_MODE
#define CTRL_REG1_OUTPUT_DATA_RATE_5376_HZ (BIT_7 | BIT_4) // only in LOW_POWER_MODE & no HIGH RESOLUTION

/* CTRL_REG2 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Filters

// TODO: Filter config, default is a highpass filter

#define CTRL_REG2_FILTERED_DATA_SELECTION_ENABLE (BIT_3) //  data from internal filter sent to output register and FIFO
#define CTRL_REG2_FILTERED_DATA_SELECTION_BYPASS (BIT_Z) //  internal filter bypassed;

// #define CTRL_REG2_HIGH_PASS_FILTER_MODE_ (BIT_Z)
// #define CTRL_REG2_HIGH_PASS_FILTER_CUT_OFF_ (BIT_Z)

// TODO: ...

/* CTRL_REG3 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Interrupts (what/which will generate them)

/* Note: both on same  INT1 pin => only one at time will work? */
#define CTRL_REG3_FIFO_OVERRUN_INTERRUPT_ENABLE (BIT_1)
#define CTRL_REG3_FIFO_WATERMARK_INTERRUPT_ENABLE (BIT_2)

/* CTRL_REG4 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// Block data update, Endianity, Scale, Operating mode (High Resolution enabled/disabled)

#define CTRL_REG4_BLOCK_DATA_UPDATE_ENABLE (BIT_7)
#define CTRL_REG4_SCALE_2G (BIT_Z)
#define CTRL_REG4_SCALE_4G (BIT_4)
#define CTRL_REG4_SCALE_8G (BIT_5)

/* CTRL_REG5 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* TODO: ... */
#define CTRL_REG5_REBOOT_REGISTERS (BIT_7) // TODO: doesnt do anything?

#define CTRL_REG5_FIFO_ENABLE (BIT_6)

/* Enale interrupt latching (...) */
#define CTRL_REG5_LATCH_INTERRUPT_1_ENABLE (BIT_3)
#define CTRL_REG5_LATCH_INTERRUPT_1_DISABLE (BIT_Z)

/* CTRL_REG6 - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
// TODO: ....

/* FIFO_CTRL_REG - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#define FIFO_CTRL_REG_SET_WATERMARK_THRESHOLD_16 (BIT_3 | BIT_2 | BIT_1 | BIT_0)
#define FIFO_CTRL_REG_SET_WATERMARK_THRESHOLD_12 (BIT_3 | BIT_1 | BIT_0)

#define FIFO_CTRL_REG_SET_FIFO_MODE_BYPASS (BIT_Z)
#define FIFO_CTRL_REG_SET_FIFO_MODE_FIFO (BIT_6)
#define FIFO_CTRL_REG_SET_FIFO_MODE_STREAM (BIT_7)
#define FIFO_CTRL_REG_SET_FIFO_MODE_FIFO_TO_STREAM (BIT_7 | BIT_6)

/* Note: reset/clear fifo = set to bypass mode */
#define FIFO_CTRL_REG_RESET_FIFO FIFO_CTRL_REG_SET_FIFO_MODE_BYPASS

/* INT1_CFG - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* INT1_THS - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/* INT1_DURATION - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/* -------------------------------------------------------------------------- */

#define I2C0_LABEL DT_LABEL(DT_NODELABEL(i2c0))

/* -------------------------------------------------------------------------- */

/* ringbuffer containing sensor data */
extern struct ring_buf lis2dh12_ring_buf; // TODO: race conditions? simultaneousl read write?
/* ringbuffer semaphore, signaled when new data added to lis2dh12_ring_buf */
extern struct k_sem ring_buf_sem;

extern double dummy_f_raw_output_data[32][3];
/* -------------------------------------------------------------------------- */

/*
Call once:
- lis2dh12_init();
- lis2dh12_config();
- lis2dh12_enable_interrupt();

Call each time ready to start reading data from the sensor:
- lis2dh12_enable_fifo();
*/

/* -------------------------------------------------------------------------- */

/**
 * @brief   start the LIS sensor
 * @details device_get_binding() for i2c and gpio, gpio_pin_configure(), gpio_init_callback(), gpio_add_callback()
 * @retval   0 on success
 * @retval < 0 on error
 */
int lis2dh12_init();

/**
 * @brief   configure the sensor by writing values to the sensors registers...
 * @details
 *  - 1. Reset all values? write default values?
 * Config registers (R/W)
 *  - CTRL_REG1
 *    - Output Data Rate Set
 *    - Selecet Axes
 *  - CTRL_REG2
 *    - Filters: enable/disable, highpass mode and cut off freq
 *  - CTRL_REG3
 *    - Interrupt generation (what will generate interrupt on INT1 pin) (TODO: only 1 source will work at once?)
 *      - Overrun interrupt (FIFO overwritten data)
 *      - Watermark interrupt (FIFO has at least n samples) (threshold n set in FIFO_CTRL_REG)
 *      - DataReady interrupt (1 new sample written to FIFO?)
 *      - IA1 interrupt (Interrupt Action 1?) (e.g. motion detection) (set/configured in INT1_CFG, INT1_THS, INT1_DURATION)
 *  - CTRL_REG4
 *    - Block Data Update: enable/disable
 *    - Endianity,
 *    - Scale/Sensitivity (milliVolts/g?),
 *    - Operating/Resolution mode (High Resolution enabled/disabled)
 *  - CTRL_REG5
 *    - Reboot memory TODO: No effect?
 *    - FIFO enable/disable (will values from sensor get written queued FIFO or not)
 *    - Latch interrupt request on INT1_SRC TODO: No effect? (with INT1_SRC register cleared by reading INT1_SRC itself)
 *  - CTRL_REG6
 *    - TODO: ?
 *
 *
 *  - FIFO_CTRL_REG
 *    - FIFO mode selection:
 *      - Bypass mode: FIFO is not operating
 *      - FIFO mode: stops collecting data when the FIFO is full (overrun)
 *      - Stream mode: starts overwriting data when full, doesn't stop
 *      - Stream-to-FIFO mode: starts in Stream mode and on ...? switches to FIFO mode
 *
 *  - INT1_CFG, INT1_THS, INT1_DURATION
 *    - ...
 * @note
 *  Read Only Registers
 *  - REFERENCE     TODO:resets ... by reading thir register
 *  - STATUS_REG    TODO: ...
 *  - FIFO_SRC_REG  contains flags (overun, watermark, empy) about FIFO and number of samples currently in FIFO
 *  - INT1_SRC      TODO: contains the source of interupt (Reading at this address clears the INT1_SRC, allows the refresh of data in the INT1_SRC if the latched enabled)
 *
 */
void lis2dh12_config();

/**
 * @brief   ... enable/set gpio interrupt ...
 * @details gpio_pin_interrupt_configure(), TODO: config sensor / read from registers ?
 * @note    has to be done after lis2dh12_config() (only after device bindings ...)
 * @retval   0 on success
 * @retval < 0 on error
 */
int lis2dh12_enable_interrupt();

/**
 * @brief   clears the sensors FIFO, and enables it (sesor starts wrtitng values to the FIFO)
 * @details
 * - 1. FIFO clear: set FIFO to Bypass mode (FIFO is not operating, it is emptied and remains empty)
 * - 2. Latch clear? TODO: how to clear latch? by latch enable CTRL_REG5 or by reading from INT1_SRC?
 * - 3. FIFO enable: set FIFO to FIFO/(Sream mode)
 */
void lis2dh12_enable_fifo(void);

/**
 * @brief   reads n sample from the sesors FIFO
 * @details wait for interrupt to happen (sem_take), i.e. data ready in FIFO,
 *          check FIFO flags (watermark, overflow),
 *          read data from FIFO and write into a ringbufer (claim/finish => avoid unnecessary tmp copy)
 * TODO: should be in accelerometer, in lis2dh12 should contain only driver/wrapper functions
 * @note 1 sample = 3 axis values = 3*2 bytes
 * @param timeout maximum time to wait for interrupt (sem_take)
 * @retval > 0 sample written into the ringbuffer
 * @retval = 0 sample written into the ringbuffer (shouldnt occur, 0 samples written = interrupt occured but empty FIFO)
 * @retval = -ENOBUFS Ring buffer is full
 * @retval = -EOVERFLOW FIFO overrun
 * @retval = -EBUSY | -EAGAIN sem_take timeout occured (no interrupt occured to give sem and notify enough samples ready)
 */
int lis2dh12_read_fifo_to_ringbuffer(k_timeout_t timeout);

/** @brief same as lis2dh12_read_fifo_to_ringbuffer(), but values are just read into a static array instead of ringbuffer */
int lis2dh12_read_fifo_dummy(k_timeout_t timeout);

/* -------------------------------------------------------------------------- */

/**
 * @brief check the FIFO flags for: Overrun, Watermark, Empty and unread Sample count
 * @retval > 0 on success, samples in FIFO
 * @retval < 0 on error, FIFO overun (full, values were overwrited)
 */
int lis2dh12_read_fifo_flags();

void lis2dh12_write_default_config();

void lis2dh12_read_config();

void lis2dh12_check_config();

#endif // __LIS2DH12_H__
