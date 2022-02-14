#include "lis2dh12.h"
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/i2c.h>
#include <drivers/gpio.h>
#include <kernel.h>
#include <nrfx.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../common.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(VIBRATION);

#define HANDLE_ERROR(x)     \
	if (x)                  \
	{                       \
		printk("error \n"); \
	}

/* read register macro : input parameters are register address and data pointer */
#define READ_REG(addr, ptr) i2c_reg_read_byte(i2c_dev, LIS_ADDRESS, addr, ptr)
/* write regiter macro: input parameters are address and output data variable */
#define WRITE_REG(addr, val) i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, addr, val)

/* define GPIO PORT and pin number*/
#define GPIO_PORT "GPIO_1"
#define INT1_PIN_NUMBER 12

/* Define thread for reading the 32 values from the fifo at every interrupt
callback */
K_THREAD_DEFINE(fifo_read_id, 2048, lis2dh12_fifo_read_thread, NULL, NULL, NULL,
				6, 0, K_FOREVER);

/* struct for reg address and values*/
typedef struct
{
	uint8_t addr;
	uint8_t val;
} reg_row_t;

/* register configuration values to put the device in FIFO stream mode with
   interrupt at FIFO full
   The register configuration is likewise
   CTRL_REG1 = 97, the ODR, sampling rate set to 1.344 Khz and enable all axis
   CTRL_REG2 = 08, enabling high pass filter
   CTRL_REG3 = 42, enable interrupt signal on on int pin 1
   CTRL_REG4 = 90, BDU on, 4g, hr off
   CTRL_REG4 = 9f, enabling fifo-stream mode
*/
const reg_row_t reg_rows[] = {
	{ADDR_CTRL_REG1, 0x97},
	{ADDR_CTRL_REG2, 0x08},
	{ADDR_CTRL_REG3, 0x42}, // OVERRUN
	{ADDR_CTRL_REG4, 0x90}, // BDU on, 4g, hr off
	{ADDR_CTRL_REG5, 0x40},
	{ADDR_INT1_THS, 0x7F},
	{ADDR_INT1_DURATION, 0x03},
	{ADDR_CTRL_REG5, 0x48},
	{ADDR_FIFO_CTRL_REG, 0x9f},
};

/* I2C device */
static struct device *i2c_dev;

/* GPIO device and callback*/
static struct device *gpio_dev;
static struct gpio_callback gpio_cb;

/* Semaphore will be given at very interrupt callback */
static struct k_sem gpio_sem;

/* shared output: contains raw accel values*/
// s32_t raw_output_data[32][3];
double f_raw_output_data[32][3];

/* LIS configure function*/
void lis2dh12_config(void);

/*
 * @brief gpio callback function will give semaphore for reading the fifo
 */
static void lis_interrupt_callback(struct device *dev,
								   struct gpio_callback *cb, u32_t pins)
{
	LOG_DBG("int cb\r\n");
	k_sem_give(&gpio_sem);
}

/*
 * @brief start the LIS sensor
 */

int lis2dh12_init(void)
{
	uint8_t chip_id;

	i2c_dev = device_get_binding(DT_NORDIC_NRF_I2C_I2C_0_LABEL);
	if (!i2c_dev)
	{
		printk("I2C: Device driver not found.\n");
		return 0;
	}

	HANDLE_ERROR(READ_REG(ADDR_WHO_AM_I, &chip_id));

	gpio_dev = device_get_binding(GPIO_PORT);

	if (gpio_dev == NULL)
	{
		printk("Failed to get pointer device!");
		return -EINVAL;
	}

	gpio_pin_configure(gpio_dev, INT1_PIN_NUMBER,
					   GPIO_DIR_IN | GPIO_INT | GPIO_INT_ACTIVE_HIGH | GPIO_INT_EDGE);

	gpio_init_callback(&gpio_cb,
					   lis_interrupt_callback,
					   BIT(INT1_PIN_NUMBER));

	if (gpio_add_callback(gpio_dev, &gpio_cb) < 0)
	{
		LOG_DBG("Failed to set gpio callback!");
		return -EIO;
	}

	gpio_pin_enable_callback(gpio_dev, INT1_PIN_NUMBER);

	k_sem_init(&gpio_sem, 1, 1);

	return 0;
}

/*
 * @brief configure the sensor by wiriting the config struct
 */

void lis2dh12_config(void)
{
	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR(i2c_reg_write_byte(i2c_dev, LIS_ADDRESS,
										reg_rows[i].addr, reg_rows[i].val));
	}
}

/*
 * @brief read 32 values from the FIFO and fill the raw data buffer
 */

void lis2dh12_fifo_read_thread()
{
	u8_t temp;
	static u8_t buf[6];

	/* The explananttion of the calculaion below:
	Full scale range = +-4G,  range = 8G
	output data accuracy = 10-bit which is 1024 values (-512 to 511)

	Raw data is signed, and left aligned, so to
	preserve sign-bit, we interpret it as s16_t
	and divide by 64, to right align it(discard
	6 bits, '16-10', 2^6=64). Can't bit shift,
	since it is a signed integer.

	So value converted to m/s^2 will be
	raw/64*(8G/1024) or raw*(8*9.80665/1024/64) m/s^2 = 0.001197100830078125f.
	*/

	const double multiplier = 0.001197100830078125f;

	while (1)
	{
		k_sem_take(&gpio_sem, K_FOREVER);
		HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp));
		HANDLE_ERROR(READ_REG(ADDR_FIFO_SRC_REG, &temp));

		if (temp & 0xC0)
		{
			for (int i = 0; i < 32; i++)
			{
				i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, buf, sizeof(buf));
				// raw * (8*9.80665/1024/64)
				f_raw_output_data[i][0] = *(s16_t *)&buf[0] * multiplier;
				f_raw_output_data[i][1] = *(s16_t *)&buf[2] * multiplier;
				f_raw_output_data[i][2] = *(s16_t *)&buf[4] * multiplier;
			}
			data_ready = EVT_DATA_READY;
		}

		if (data_ready == EVT_DATA_READY)
		{
			while (k_msgq_put(&logging_q, &data_ready, K_NO_WAIT) != 0)
			{
				/* message queue is full: purge old data & try again */
				k_msgq_purge(&logging_q);
			}
		}
		else
		{
			data_ready = 0;
		}
	}
}
