#include "lis2dh12.h"

/* -------------------------------------------------------------------------- */

LOG_MODULE_REGISTER(lis2dh12);

#define HANDLE_ERROR(msg, e)       \
	if (e)                         \
	{                              \
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
	const uint8_t addr;
	const uint8_t val;
	const char *name;
} reg_row_t;

#define REG_ROW_T_ENTRY(ADDR, VAL) \
	{                              \
		ADDR, VAL, #ADDR           \
	}

/* register configuration values */
const reg_row_t reg_rows[] = {
	REG_ROW_T_ENTRY(ADDR_CTRL_REG1, CTRL_REG1_LOW_POWER_MODE_DISABLE | CTRL_REG1_XYZ_AXES_ENABLE | CTRL_REG1_OUTPUT_DATA_RATE_400_HZ),
	REG_ROW_T_ENTRY(ADDR_CTRL_REG2, CTRL_REG2_FILTERED_DATA_SELECTION_BYPASS),
	REG_ROW_T_ENTRY(ADDR_CTRL_REG3, CTRL_REG3_FIFO_WATERMARK_INTERRUPT_ENABLE),
	REG_ROW_T_ENTRY(ADDR_CTRL_REG4, CTRL_REG4_BLOCK_DATA_UPDATE_ENABLE | CTRL_REG4_SCALE_4G),
	REG_ROW_T_ENTRY(ADDR_CTRL_REG5, CTRL_REG5_FIFO_ENABLE | CTRL_REG5_LATCH_INTERRUPT_1_DISABLE),
	REG_ROW_T_ENTRY(ADDR_CTRL_REG6, 0x00),

	REG_ROW_T_ENTRY(ADDR_FIFO_CTRL_REG, FIFO_CTRL_REG_SET_FIFO_MODE_BYPASS | FIFO_CTRL_REG_SET_WATERMARK_THRESHOLD_16),

	REG_ROW_T_ENTRY(ADDR_INT1_CFG, 0x00),
	REG_ROW_T_ENTRY(ADDR_INT1_THS, 0x00),
	REG_ROW_T_ENTRY(ADDR_INT1_DURATION, 0x00),
};

/* tmp string for logs */
static char bin[32] = {0};

/* -------------------------------------------------------------------------- */

/* I2C device */
static const struct device *i2c_dev;

/* GPIO device and callback*/
static const struct device *gpio_dev;
static struct gpio_callback gpio_cb;

/* Semaphore will be given at very interrupt callback */
K_SEM_DEFINE(gpio_sem, 0, 5);

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

// RING_BUF_ITEM_DECLARE_POW2(lis2dh12_ring_buf, 8);			   // unusable, can't align mem 6 and 4
RING_BUF_ITEM_DECLARE_SIZE(lis2dh12_ring_buf, 4 * 32 * 2 * 3 * 4); // TODO: size
K_SEM_DEFINE(ring_buf_sem, 0, 100);								   // TODO: count_limit

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

	lis2dh12_write_default_config();

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		HANDLE_ERROR("WRITE_REG err", WRITE_REG(reg_rows[i].addr, reg_rows[i].val));
	}

	lis2dh12_read_config();
}

int lis2dh12_enable_interrupt()
{
	LOG_INF("lis2dh12_enable_interrupt()");

	uint8_t tmp;

	// TODO: Latch interrupt request on INT1_SRC clear (latch clear by reading from INT1_SRC)?
	HANDLE_ERROR("READ_REG(ADDR_INT1_SRC) err", READ_REG(ADDR_INT1_SRC, &tmp));

	int res = gpio_pin_interrupt_configure(gpio_dev, INT1_PIN_NUMBER, GPIO_INT_EDGE_RISING); // ?
	if (res != 0)
	{
		LOG_ERR("Failed irq configure!");
		return res;
	}

	// TODO: Latch interrupt request on INT1_SRC clear (latch clear by reading from INT1_SRC)?
	HANDLE_ERROR("READ_REG(ADDR_INT1_SRC) err", READ_REG(ADDR_INT1_SRC, &tmp));

	return 0;
}

void lis2dh12_enable_fifo(void)
{
	LOG_INF("lis2dh12_enable_fifo()");

	// Reset any leftovers from previous runs of accelerometer service
	k_sem_reset(&gpio_sem);

	// Stop and Clear FIFO
	WRITE_REG(ADDR_FIFO_CTRL_REG, FIFO_CTRL_REG_SET_FIFO_MODE_BYPASS | FIFO_CTRL_REG_SET_WATERMARK_THRESHOLD_16);

	// TODO: Latch Clear?

	// Start FIFO again (~ switch from Bypass mode to FIFO mode)
	WRITE_REG(ADDR_FIFO_CTRL_REG, FIFO_CTRL_REG_SET_FIFO_MODE_FIFO | FIFO_CTRL_REG_SET_WATERMARK_THRESHOLD_16);
}

/* -------------------------------------------------------------------------- */

/**
 * @brief write data from i2c_burst_read directly into the ring buffer (without copying)
 *
 * @details claim memory from ring buffer
 * - returned size == claim size => all ok, write to ring buffer
 * - returned size <  claim size => either not enough mem, or mem wrap
 *
 * @note: REQUIERES RING BUFFER TO BE 4 and 6 Bytes ALIGNED
 * 	- 4 <= 32-bit processor, ...
 *	- 6 <= 3-axis, per axis value = 2 bytes
 *
 * @param p_ring_buf - pointer to ring buffer
 * @param byte_count - total number of bytes to read from FIFO to ring buffer (sample_count * bytes_per_sample)
 *
 * @retval > 0 - number of bytes read and written succesfuly from from FIFO into ring buffer
 * @retval = 0 - requested 0 bytes to write
 * @retval < 0 - error (insufficient ring buffer memory, i2c_burst_read error, ...)
 */
int i2c_ringbuffer_burst_read(struct ring_buf *p_ring_buf, int byte_count)
{
	LOG_INF("i2c_ringbuffer_burst_read()");

	// TODO: shouldn't occur
	if (byte_count == 0)
	{
		LOG_ERR("RING i2c_ringbuffer_burst_read(0 Bytes) to write requested");
		return 0;
	}

	uint8_t *data;
	int ringbuf_aloc_size;
	// int ringbuf_finish_ret;

	ringbuf_aloc_size = ring_buf_put_claim(p_ring_buf, &data, byte_count);
	LOG_INF("ring_buf_put_claim: claimed=%d, alocated=%d, space=%d", byte_count, ringbuf_aloc_size, ring_buf_space_get(p_ring_buf));

	if (ringbuf_aloc_size == 0)
	{
		LOG_ERR("RING BUFFER FULL");
		ring_buf_put_finish(p_ring_buf, 0);
		return -ENOBUFS;
	}
	else if (ringbuf_aloc_size < byte_count)
	{
		LOG_INF("RING BUFFER INSUFFICIENT (full or mem wrap)");

		HANDLE_ERROR("i2c_burst_read error",
					 i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, data, ringbuf_aloc_size));
		ring_buf_put_finish(p_ring_buf, ringbuf_aloc_size);

		LOG_INF("RING BUFFER calim/finish %d bytes", ringbuf_aloc_size);

		return ringbuf_aloc_size + i2c_ringbuffer_burst_read(p_ring_buf, byte_count - ringbuf_aloc_size);
	}
	else // ringbuf_aloc_size == byte_count
	{
		HANDLE_ERROR("i2c_burst_read error",
					 i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, data, ringbuf_aloc_size));
		ring_buf_put_finish(p_ring_buf, ringbuf_aloc_size);

		LOG_INF("RING BUFFER calim/finish %d bytes", ringbuf_aloc_size);

		return ringbuf_aloc_size;
	}
}

int lis2dh12_read_fifo_to_ringbuffer(k_timeout_t timeout)
{
	LOG_INF("lis2dh12_read_fifo_to_ringbuffer()");

	// Take semaphore (interrupt occured)
	LOG_INF("gpio_sem take: limit=%d, count=%d\n", gpio_sem.limit, gpio_sem.count);
	int timeout_res = k_sem_take(&gpio_sem, timeout);
	if (timeout_res)
	{
		LOG_WRN("gpio_sem timeout %d", timeout_res);
		return timeout_res;
	}
	LOG_INF("gpio_sem taken");

	int sample_count = lis2dh12_read_fifo_flags();

	if (sample_count < 0)
	{
		LOG_ERR("FIFO OVERRUN");
		return sample_count;
	}

	if (sample_count == 0)
	{
		return sample_count;
	}

	////////////////////////////////////////////////////////////////////////////

	int bytes_per_sample = 3 * 2;
	int ret = i2c_ringbuffer_burst_read(&lis2dh12_ring_buf, sample_count * bytes_per_sample);

	if (ret < sample_count * bytes_per_sample)
	{
		LOG_ERR("RING BUFEER FULL");
		return -ENOBUFS;
	}

	LOG_INF("ring_buf_sem give: limit=%d, count=%d\n", ring_buf_sem.limit, ring_buf_sem.count);
	k_sem_give(&ring_buf_sem);

	////////////////////////////////////////////////////////////////////////////

	LOG_INF("read %d samples from FIFO into ringbuffer", sample_count);

	return sample_count;
}

/* -------------------------------------------------------------------------- */

double dummy_f_raw_output_data[32][3];
int lis2dh12_read_fifo_dummy(k_timeout_t timeout)
{
	LOG_INF("lis2dh12_read_fifo_dummy()");

	static uint8_t buf[6];

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

	// Take semaphore (interrupt occured)
	LOG_INF("gpio_sem take");
	int timeout_res = k_sem_take(&gpio_sem, timeout);
	if (timeout_res)
	{
		LOG_INF("gpio_sem timeout %d", timeout_res);
		return -1;
	}
	LOG_INF("gpio_sem taken");

	int sample_count = lis2dh12_read_fifo_flags();

	for (int i = 0; i < sample_count; i++)
	{
		i2c_burst_read(i2c_dev, LIS_ADDRESS, 0x80 | ADDR_OUT_X_L, buf, sizeof(buf));
		// raw * (8*9.80665/1024/64)
		dummy_f_raw_output_data[i][0] = *(int16_t *)&buf[0] * multiplier;
		dummy_f_raw_output_data[i][1] = *(int16_t *)&buf[2] * multiplier;
		dummy_f_raw_output_data[i][2] = *(int16_t *)&buf[4] * multiplier;
	}

	LOG_INF("read %d samples", sample_count);

	return sample_count;
}

/* -------------------------------------------------------------------------- */

int lis2dh12_read_fifo_flags()
{
	LOG_INF("lis2dh12_read_fifo_flags()");

	uint8_t tmp;

	HANDLE_ERROR("READ_REG(ADDR_FIFO_SRC_REG) err", READ_REG(ADDR_FIFO_SRC_REG, &tmp));

	/* mask flags */
	bool watermark = tmp & (1 << 7);
	bool overrun = tmp & (1 << 6);
	bool empty = tmp & (1 << 5);
	uint8_t sample_count = tmp & 0x1f;

	LOG_INF("ADDR_FIFO_SRC_REG: watermark: %d, overrun: %d, empty: %d, sample_count=%d\n",
			watermark, overrun, empty, sample_count);

	if (overrun)
	{
		LOG_ERR("FIFO OVERRUN");
		return -1;
	}

	return sample_count;
}

void lis2dh12_write_default_config()
{
	LOG_INF("lis2dh12_write_default_config()");

	WRITE_REG(ADDR_CTRL_REG5, CTRL_REG5_REBOOT_REGISTERS); // TODO: ?

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		WRITE_REG(reg_rows[i].addr, 0x00);
	}
}

void itoa(uint8_t val, char *buf, uint8_t base)
{

	int i = sizeof(val) * 8;
	buf[i--] = '\0';

	for (; i + 1; --i, val /= base)
		buf[i] = "0123456789abcdef"[val % base];
}

void lis2dh12_read_config()
{
	LOG_INF("lis2dh12_read_config()");

	uint8_t val;

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		READ_REG(reg_rows[i].addr, &val);
		itoa(val, bin, 2);

		LOG_INF("%-24s[%02x] = 0x%02X = 0b%s; (my_config_val = 0x%02X)l\n", reg_rows[i].name, reg_rows[i].addr, (int)val, log_strdup(bin), reg_rows[i].val);
		// printk("%-24s[%02x] = 0x%02X = 0b%s; (my_config_val = 0x%02X)l\n", reg_rows[i].name, reg_rows[i].addr, (int)val, bin, reg_rows[i].val);
	}
}

/** @brief compare values in registers with config values that were written to the registers */
void lis2dh12_check_config()
{
	LOG_INF("lis2dh12_check_config()");

	uint8_t val;

	for (int i = 0; i < ARRAY_SIZE(reg_rows); i++)
	{
		READ_REG(reg_rows[i].addr, &val);
		itoa(val, bin, 2);

		if (reg_rows[i].val == val)
			LOG_INF("%-24s[%02x] = 0x%02X = 0b%s == 0x%02X (config)l\n", reg_rows[i].name, reg_rows[i].addr, (int)val, log_strdup(bin), reg_rows[i].val);
		else
			LOG_WRN("%-24s[%02x] = 0x%02X = 0b%s != 0x%02X (config)l\n", reg_rows[i].name, reg_rows[i].addr, (int)val, log_strdup(bin), reg_rows[i].val);
	}
}
