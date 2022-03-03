#include "lis2dh12.h"

/* -------------------------------------------------------------------------- */

// TODO: HANDLE_ERROR
#define HANDLE_ERROR(x)         \
	if (x)                      \
	{                           \
		printk("\nERROR \n\n"); \
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
// static struct k_sem gpio_sem;
K_SEM_DEFINE(gpio_sem, 0, 1);

/* tmp */
double f_raw_output_data[32][3];

/* -------------------------------------------------------------------------- */
#include <sys/ring_buffer.h>

// RING_BUF_ITEM_DECLARE_POW2(accel_ring_buf, 8); // unusable, can't align mem 6 and 4
RING_BUF_ITEM_DECLARE_SIZE(accel_ring_buf, 4 * 32 * 2 * 3 * 4); // 3072 DEBUG
// RING_BUF_ITEM_DECLARE_SIZE(accel_ring_buf, 10 * 32 * 2 * 3 * 4); // TODO: ...

K_SEM_DEFINE(ring_buf_sem, 0, 100); // TODO: count_limit

/* -------------------------------------------------------------------------- */

// TODO: 2 consequitive interrupts? before taking semaphore => 1 reading will be lost
/** @brief gpio callback function, will give semaphore for reading the fifo, on each interupt */
static void lis_interrupt_callback(const struct device *dev,
								   struct gpio_callback *cb, gpio_port_pins_t pins)
{
	printk("\nlis_interrupt_callback\n");
	printk("\tgpio_sem: limit=%d, count=%d\n", gpio_sem.limit, gpio_sem.count);
	printk("\tgpio_sem give\n\n");
	k_sem_give(&gpio_sem);
}

/* -------------------------------------------------------------------------- */

int lis2dh12_init()
{
	printk("lis2dh12_init\n");

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
		printk("Failed to get pointer device!\n");
		return -EINVAL;
	}

	gpio_pin_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INPUT | GPIO_ACTIVE_HIGH);

	gpio_init_callback(&gpio_cb, lis_interrupt_callback, BIT(INT1_PIN_NUMBER));

	res = gpio_add_callback(gpio_dev, &gpio_cb);
	if (res < 0)
	{
		printk("Failed to set gpio callback!\n");
		return res;
	}

	printk("lis2dh12_init success\n\n");
	return 0;
}

void _test_lis2dh12_config();
int _check_flags();

void lis2dh12_config()
{
	printk("lis2dh12_config\n");

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR(i2c_reg_write_byte(i2c_dev,
										LIS_ADDRESS,
										reg_rows[i].addr,
										reg_rows[i].val));
	}

	_test_lis2dh12_config();
}

int lis2dh12_enable_interrupt()
{
	printk("lis2dh12_enable_interrupt\n");

	uint8_t temp;

	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp));
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // ?

	int res = gpio_pin_interrupt_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INT_EDGE_RISING); // ?
	if (res != 0)
	{
		printk("Failed irq configure!");
		return res;
	}

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // ?

	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp));
	HANDLE_ERROR(READ_REG(ADDR_FIFO_SRC_REG, &temp));

	printk("lis2dh12_enable_interrupt sucess\n\n");
	// _test_lis2dh12_config();
	return 0;
}

void lis2dh12_enable_fifo(void)
{
	printk("lis2dh12_enable_fifo\n");
	_check_flags();

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x0f); // clears fifo?

	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_FIFO_CTRL_REG, 0x8f); // LATCH CLEAR
	i2c_reg_write_byte(i2c_dev, LIS_ADDRESS, ADDR_CTRL_REG5, 0x48);		// ENABLE FIFO, LATCH ENABLE ??

	_check_flags();
	printk("lis2dh12_enable_fifo end\n\n");
	// _test_lis2dh12_config();
}

int write_ringbuffer(struct ring_buf *buf, int byte_count);

int lis2dh12_read_buffer(k_timeout_t timeout)
{
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

	// TODO: move conversion to write service
	// const double multiplier = 0.001197100830078125f;

	// Take semaphore (interrupt occured)
	printk("\tlis2dh12_read_buffer k_sem_take\n");
	printk("\tgpio_sem: limit=%d, count=%d\n", gpio_sem.limit, gpio_sem.count);

	int timeout_res = k_sem_take(&gpio_sem, timeout);

	printk("\tlis2dh12_read_buffer sem taken\n");

	if (timeout_res)
	{
		printk("\tlis2dh12_read_buffer timeout\n");
		return timeout_res;
	}
	printk("\tlis2dh12_read_buffer check_flags\n");

	// TODO: check flags (watermark, overrun, sample_count, ...)
	int sample_count = _check_flags();
	if (sample_count <= 0)
	{
		return 0;
	}
	int bytes_per_sample = 3 * 2;
	int ret = write_ringbuffer(&accel_ring_buf, sample_count * bytes_per_sample);

	if (ret != sample_count * bytes_per_sample)
	{
		// TODO: ring buffer full
		return -1;
	}

	// TODO: signal ring buffer entry (semaphore/cond_var?)
	// ...

	printk("\tlis2dh12_read_buffer finished reading %d samples\n\n", sample_count);

	return sample_count;
}

// TODO: needed? when to call?
// void latch_clear()
// {
// 	uint8_t temp;
// 	HANDLE_ERROR(READ_REG(ADDR_INT1_SRC, &temp)); // latch clear ??
// 	HANDLE_ERROR(READ_REG(ADDR_INT2_SRC, &temp)); // latch clear ??
// }

int _check_flags()
{

	uint8_t temp;

	/* read flags ----------------------------------------------------------- */

	// latch_clear(); //?

	// read state of fifo (depending on setting, watermark/overrun/samplecount)
	HANDLE_ERROR(READ_REG(ADDR_FIFO_SRC_REG, &temp));

	/* mask flags ----------------------------------------------------------- */

	// TODO: what is temp & 0xC0

	bool watermark = temp & 0x80;
	bool overrun = temp & 0x40;
	bool empty = temp & 0x20;
	int sample_count = temp & 0x1f;

	printk("\tsample_count=%d, empty: %d, watermark: %d, overrun: %d\n",
		   sample_count, (int)empty, (int)watermark, (int)overrun);

	/* check overrun -------------------------------------------------------- */
	if (overrun)
	{
		printk("\t\tOVERRUN\n");
		return -1;
	}
	/* ---------------------------------------------------------------------- */

	if (sample_count == 0)
	{
		printk("\t\tEMPTY (0 samples)\n");
		return 0;
	}

	/* ---------------------------------------------------------------------- */

	return sample_count;
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
		// assert(reg_rows[i].val == val);
	}
}

/* -------------------------------------------------------------------------- */

/**
 * @brief write to the ring buffer by calling i2c_burst_read(),  deal with no mem, mem overlap situations
 *
 * - claim memory from ring buffer
 * 	- returned size == claim size all ok, write to ring buffer
 * 	- returned size <  claim size, either not enough mem, or mem wrap
 *
 * - REQUIERES RING BUFFER TO BE ALIGNED 4 and 6 Bytes.
 * 	- 4 <= 32-bit processor, ...
 *	- 6 <= 3-axis, per axis value = 2 bytes
 *
 * @param byte_count - sample_count * bytes_per_sample (bytes_per_sample=3*2)
 *
 * @retval n - number of bytes read from the i2c ... (writen to ring buffer)
 * @retval 0 - if no ring buffer memory space
 */
int write_ringbuffer(struct ring_buf *p_buf, int byte_count)
{
	uint8_t *data;

	int ret = ring_buf_put_claim(p_buf, &data, byte_count);
	printk("\t\tRING BUFFER claim, ret = %d, %d\n", byte_count, ret);

	if (ret == byte_count) // normal
	{
		i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, data, byte_count);
		ring_buf_put_finish(p_buf, byte_count); // TODO: check return value of == 0

		k_sem_give(&ring_buf_sem);
		return byte_count;
	}
	else if (ret == 0) // 0 space
	{
		// ERROR?
		// ring_buf_space_get(p_buf) // should be zero
		printk("\t\tRING BUFFER FULL? ring_buf_space_get = %d\n", ring_buf_space_get(p_buf));
		return 0;
	}
	else // insufficient space, or not enough continuous memory (mem wrap)
	{
		printk("\t\tRING BUFFER FULL or MEM WRAP? ring_buf_space_get = %d\n", ring_buf_space_get(p_buf));
		i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, data, ret);
		ring_buf_put_finish(p_buf, ret); // TODO: check return value of == 0
		return ret + write_ringbuffer(p_buf, byte_count - ret);
	}
	return 0;
}
