#include "lis2dh12.h"

/* -------------------------------------------------------------------------- */

// TODO: HANDLE_ERROR
#define HANDLE_ERROR(x)     \
	if (x)                  \
	{                       \
		printk("error \n"); \
	}

/* -------------------------------------------------------------------------- */

/* read register macro : input parameters are register address and data pointer */
#define READ_REG(addr, ptr) i2c_reg_read_byte(i2c_dev, LIS_ADDRESS, addr, ptr)
/* write regiter macro: input parameters are address and output data variable */
#define WRITE_REG(addr, val) i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, addr, val)

/* define GPIO PORT and pin number*/
#define GPIO_PORT "GPIO_1"
#define INT1_PIN_NUMBER 12

/* struct for reg address and values*/
typedef struct
{
	uint8_t addr;
	uint8_t val;
} reg_row_t;

/* -------------------------------------------------------------------------- */
// TODO: double check the order of writes, it matters
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
	{ADDR_CTRL_REG5, 0x80}, // RESET

	{ADDR_CTRL_REG1, 0x77}, // Sampling Rate 400 Hz
	{ADDR_CTRL_REG2, 0x08},
	{ADDR_CTRL_REG3, 0x06}, // INTERUPT ACTIVE 1, OVERRUN, WATERMARK
	{ADDR_CTRL_REG4, 0x90}, // BDU on, 4g, hr off

	{ADDR_CTRL_REG6, 0x00},

	{ADDR_INT1_THS, 0x00},
	{ADDR_INT1_DURATION, 0x03},
	{ADDR_INT1_CFG, 0x00},

	{ADDR_CTRL_REG5, 0x48}, // ENABLE FIFO, LATCH ENABLE
	// {ADDR_CTRL_REG5, 0x40}, // ENABLE FIFO, LATCH DISABLE
};

/* -------------------------------------------------------------------------- */

/* I2C device */
static const struct device *i2c_dev;

/* GPIO device and callback*/
static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;

/* Semaphore will be given at very interrupt callback */
// static struct k_sem gpio_sem;
K_SEM_DEFINE(gpio_sem, 0, 1);

/* tmp */
double f_raw_output_data[32][3];

/* -------------------------------------------------------------------------- */

// TODO: 2 consequitive interrupts? before taking semaphore => 1 reading will be lost
/** @brief gpio callback function, will give semaphore for reading the fifo, on each interupt */
static void lis_interrupt_callback(const struct device *dev,
								   struct gpio_callback *cb, gpio_port_pins_t pins)
{
	printk("lis_interrupt_callback\n");
	k_sem_give(&gpio_sem);
}

/* -------------------------------------------------------------------------- */

int lis2dh12_init()
{
	uint8_t chip_id;
	int res;

	i2c_dev = device_get_binding(I2C0_LABEL);
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

	gpio_pin_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INPUT | GPIO_ACTIVE_HIGH);

	gpio_init_callback(&gpio_cb, lis_interrupt_callback, BIT(INT1_PIN_NUMBER));

	res = gpio_add_callback(gpio_dev, &gpio_cb);
	if (res < 0)
	{
		printk("Failed to set gpio callback!");
		return res;
	}

	return 0;
}

void lis2dh12_config()
{
	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR(i2c_reg_write_byte(i2c_dev,
										LIS_ADDRESS,
										reg_rows[i].addr,
										reg_rows[i].val));
	}
}

int lis2dh12_enable_interrupt()
{
	uint8_t temp;

	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp));
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f);

	int res = gpio_pin_interrupt_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INT_EDGE_RISING);
	if (res != 0)
	{
		printk("Failed irq configure!");
		return res;
	}

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f);

	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp));
	HANDLE_ERROR(READ_REG(ADDR_FIFO_SRC_REG, &temp));

	return 0;
}

void lis2dh12_enable_fifo(void)
{
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x8f); // LATCH CLEAR
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_CTRL_REG5, 0x48);		// ENABLE FIFO, LATCH ENABLE ??
}

int lis2dh12_read_buffer(k_timeout_t timeout)
{
	return 0;
}

/*
SYS_INIT
- lis2dh12_init();
- lis2dh12_config();
- lis2dh12_enable_interrupt();
With every service enable:
- lis2dh12_enable_fifo();
*/

/* -------------------------------------------------------------------------- */
#include <assert.h>

void _test_lis2dh12_config()
{
	uint8_t val;
	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR(i2c_reg_read_byte(i2c_dev, LIS_ADDRESS,
									   reg_rows[i].addr, &val));
		printk("reg: %02x; %02x ?= %02x\n",
			   reg_rows[i].addr, (int)reg_rows[i].val, (int)val);
		assert(reg_rows[i].val == val);
	}
}
