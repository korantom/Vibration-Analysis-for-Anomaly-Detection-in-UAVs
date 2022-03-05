#include "lis2dh12.h"

/* -------------------------------------------------------------------------- */

LOG_MODULE_REGISTER(lis2dh12);

#define HANDLE_ERROR(msg, e)       \
	if (e)                         \
	{                              \
		printk("\nERROR \n\n");    \
		LOG_ERR("%s: %d", msg, e); \
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

/** register configuration values
 * TODO: double check the order of writes, it matters
 */
const reg_row_t reg_rows[] = {
	// TODO: double check the order of reg writes (!the order is important!)
	{ADDR_CTRL_REG5, 0x80}, // RESET

	{ADDR_CTRL_REG1, 0x77}, // Sampling Rate 400 Hz
	{ADDR_CTRL_REG2, 0x08}, // Enabling high pass filter?
	{ADDR_CTRL_REG3, 0x06}, // INTERUPT ACTIVE 1, OVERRUN, WATERMARK
	{ADDR_CTRL_REG4, 0x90}, // BDU on, 4g, hr off

	{ADDR_CTRL_REG6, 0x00}, // ?

	{ADDR_INT1_THS, 0x00},
	{ADDR_INT1_DURATION, 0x03},
	{ADDR_INT1_CFG, 0x00},

	{ADDR_CTRL_REG5, 0x48},		  // ENABLE FIFO, LATCH ENABLE
	/* {ADDR_CTRL_REG5, 0x40}, */ // ENABLE FIFO, LATCH DISABLE
};

/* -------------------------------------------------------------------------- */

/* I2C device */
static const struct device *i2c_dev;

/* GPIO device and callback*/
static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;

/* Semaphore will be given at very interrupt callback */
K_SEM_DEFINE(gpio_sem, 0, 1);

/* -------------------------------------------------------------------------- */

// TODO: 2 consequitive interrupts? before taking semaphore => 1 reading will be lost; increase semaphore limit?
/** @brief gpio callback function, will give semaphore for reading the fifo, on each interupt */
static void lis_interrupt_callback(const struct device *dev,
								   struct gpio_callback *cb, gpio_port_pins_t pins)
{
	LOG_INF("lis_interrupt_callback()");
	LOG_INF("gpio_sem give: limit=%d, count=%d\n", gpio_sem.limit, gpio_sem.count);
	k_sem_give(&gpio_sem);
}

/* -------------------------------------------------------------------------- */

/** @brief compare values in registers with config values that were written to the registers */
void _test_lis2dh12_config();

/**
 * @brief check the FIFO flags for: Overrun, Watermark and Samples read count
 * @retval > 0 on success, samples in FIFO
 * @retval < 0 on error, FIFO overun (full, values were overwrited)
 */
int _check_flags();

/* -------------------------------------------------------------------------- */

int lis2dh12_init()
{
	LOG_INF("lis2dh12_init()");

	uint8_t chip_id;
	int res;

	i2c_dev = device_get_binding(I2C0_LABEL);
	if (i2c_dev == NULL)
	{
		LOG_ERR("I2C: Device not found");
		return -ENODEV;
	}

	HANDLE_ERROR("READ_REG(ADDR_WHO_AM_I) err", READ_REG(ADDR_WHO_AM_I, &chip_id)); // TODO: ?

	gpio_dev = device_get_binding(GPIO_PORT);
	if (gpio_dev == NULL)
	{
		LOG_ERR("GPIO: Device not found");
		return -ENODEV;
	}

	gpio_pin_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INPUT | GPIO_ACTIVE_HIGH);

	gpio_init_callback(&gpio_cb, lis_interrupt_callback, BIT(INT1_PIN_NUMBER));

	res = gpio_add_callback(gpio_dev, &gpio_cb);
	if (res < 0)
	{
		LOG_ERR("Failed to set gpio callback!");
		return res;
	}

	return 0;
}

void lis2dh12_config()
{
	LOG_INF("lis2dh12_config()");

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR("i2c_reg_write_byte() err", i2c_reg_write_byte(i2c_dev,
																	LIS_ADDRESS,
																	reg_rows[i].addr,
																	reg_rows[i].val));
	}

	_test_lis2dh12_config();
}

int lis2dh12_enable_interrupt()
{
	LOG_INF("lis2dh12_enable_interrupt()");

	uint8_t temp;

	HANDLE_ERROR("READ_REG(ADDR_INT1_SRC) err", READ_REG(ADDR_INT1_SRC, &temp));
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // ?

	int res = gpio_pin_interrupt_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INT_EDGE_RISING); // ?
	if (res != 0)
	{
		LOG_ERR("Failed irq configure!");
		return res;
	}

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // ?

	HANDLE_ERROR("READ_REG(ADDR_INT1_SRC) err", READ_REG(ADDR_INT1_SRC, &temp));
	HANDLE_ERROR("READ_REG(ADDR_FIFO_SRC_REG) err", READ_REG(ADDR_FIFO_SRC_REG, &temp));

	// _test_lis2dh12_config();
	return 0;
}

void lis2dh12_enable_fifo(void)
{
	LOG_INF("lis2dh12_enable_fifo()");

	_check_flags();

	LOG_INF("FIFO clear?, LATCH clear?, ENABLE FIFO&LATCH?");

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // CLEAR FIFO?

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x8f); // LATCH CLEAR?
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_CTRL_REG5, 0x48);		// ENABLE FIFO, LATCH ENABLE ??

	_check_flags();
}

/* -------------------------------------------------------------------------- */

// TODO: needed? when to call?
// void latch_clear()
// {
// 	uint8_t temp;
// 	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp)); // latch clear ??
// 	HANDLE_ERROR(READ_REG(ADDR_INT2_SRC, &temp)); // latch clear ??
// }

/* -------------------------------------------------------------------------- */

void _test_lis2dh12_config()
{
	LOG_INF("_test_lis2dh12_config()");

	uint8_t val;
	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR("i2c_reg_read_byte() err", i2c_reg_read_byte(i2c_dev,
																  LIS_ADDRESS,
																  reg_rows[i].addr,
																  &val));

		// assert(reg_rows[i].val == val);

		if (reg_rows[i].val == val)
			LOG_INF("sensor reg[%02x]: %02x == %02x reg_rows_config", reg_rows[i].addr, (int)val, (int)reg_rows[i].val);
		else
			LOG_WRN("sensor reg[%02x]: %02x != %02x reg_rows_config", reg_rows[i].addr, (int)val, (int)reg_rows[i].val);
	}
}

int _check_flags()
{
	LOG_INF("_check_flags()");

	uint8_t temp;

	/* read FIFO flags, (read state of fifo (depending on setting, watermark/overrun/samplecount)) */
	HANDLE_ERROR("READ_REG(ADDR_FIFO_SRC_REG) err", READ_REG(ADDR_FIFO_SRC_REG, &temp));

	/* mask flags */
	// int ? = temp & 0xC0 // TODO: ?
	bool watermark = temp & 0x80;
	bool overrun = temp & 0x40;
	bool empty = temp & 0x20;
	int sample_count = temp & 0x1f;

	LOG_INF("flags: sample_count=%d, empty: %d, watermark: %d, overrun: %d",
			sample_count, (int)empty, (int)watermark, (int)overrun);

	/*  --------------------------------------------------------------------- */
	if (overrun)
	{
		LOG_ERR("FIFO OVERRUN");
		return -1;
	}

	if (sample_count == 0)
	{
		LOG_WRN("FIFO EMPTY (0 samples)");
	}

	return sample_count;
}
