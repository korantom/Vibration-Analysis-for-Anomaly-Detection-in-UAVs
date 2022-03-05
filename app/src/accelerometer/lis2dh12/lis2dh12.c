#include "lis2dh12.h"

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
