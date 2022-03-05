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
