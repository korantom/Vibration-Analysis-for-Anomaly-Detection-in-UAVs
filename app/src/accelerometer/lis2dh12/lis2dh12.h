/*Register Addresses*/

#ifndef __LIS2DH12_FIFO_H__
#define __LIS2DH12_FIFO_H__

#include <kernel.h>

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

#define REG1_LOW_PWR_MODE_DISABLE (0)
#define REG1_LOW_PWR_MODE_ENABLE (1 << 3)
#define REG1_ENABLE_AXIS_X 0x01
#define REG1_ENABLE_AXIS_Y 0x02
#define REG1_ENABLE_AXIS_Z 0x04
#define REG1_ENABLE_AXIS_XY 0x03
#define REG1_ENABLE_AXIS_YZ 0x06
#define REG1_ENABLE_AXIS_XZ 0x05
#define REG1_ENABLE_AXIS_XYZ 0x07
#define REG1_POWER_DOWN 0x00
// rest of the values in header
#define REG1_ODR_1_620_KHZ 0x80          // Low-power (1.620 kHz)
#define REG1_ODR_1_344_OR_5_376_KHZ 0x90 // Normal/HR (1.344 kHz), Low-power (5.376 kHz)

#define REG2_HPM_NORMAL 0 // can (but don't have to) clear DC by reading ADDR_REFERENCE reg
#define REG2_HPM_REFERENCE (1 << 6)
#define REG2_HPM_NORMAL2 (1 << 7)
#define REG2_HPM_AUTORESET ((1 << 7) | (1 << 6))
#define REG2_FDS_BYPASS 0
#define REG2_FDS_USE (1 << 3) // filtered data sent to data regs and FIFO
#define REG2_HP_IA1_ENABLED (1 << 0)
#define REG2_HP_IA2_ENABLED (1 << 1)

#define LIS_ADDRESS 0x19

int lis2dh12_init();

void lis2dh12_fifo_read_thread();

void lis2dh12_config(void);

float lis2dh12_convert_output(s32_t val);

#endif // __LIS2DH12_FIFO_H__
