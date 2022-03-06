#ifndef __COMMON_H__
#define __COMMON_H__

// SYS_INIT priorities: integer in range [0-99], lower values indicate earlier initialization
#define MAIN_SYS_INIT_PRIORITY 90
#define ACCELEROMETER_SYS_INIT_PRIORITY 91

// STACK SIZES
#define ACCELEROMETER_STACK_SIZE 1024

// THREAD PRIORITIES (MAIN_THREAD_PRIORITY = -1)
#define ACCELEROMETER_THREAD_PRIORITY -1

#endif // __COMMON_H__
