#ifndef __SERVICE_TEMPLATE_H__
#define __SERVICE_TEMPLATE_H__

#include <device.h>
#include <logging/log.h>
#include <shell/shell.h>
#include <sys/printk.h>
#include <zephyr.h>

#include "../common.h"

/* -------------------------------------------------------------------------- */

/**
 * All services will follow a similar structure and interface,
 * and will probably be called in the same order as well.
 * ...
 *
 * Each service should:
 * 1. call SYS_INIT
 *   - will probably call config_service(...)
 *   - called only once during system boot
 *
 * 2. call config_service():
 *   - set up all ... needed before spawing a thread
 *   - should be called only once?
 *
 * 3. spawn a thread:
 *   - thread should remain for the entire time of the system runnig (only 1 thread per service is created)
 *   - thread will run the service core function (in an infinite loop)
 *
 *   - service will be controlled (paused and resumed) using kernel synchronization objects
 *     - pausing service  ~ putting thread to sleep (suspending it / forcing it to wait)
 *     - resuming service ~ waking up thread (signaling it)
 *       - will be done by another thread => need condvar/sem for signaling
 *   - we can use either a mutex + conditional_variable or a binary semaphore?
 *
 *   - service thread will be suspended/waiting when:
 *     - service is disabled
 *     - service depends on another threds output/resource
 *
 * 4. provide a shell interface (allow control from the shell)
 *
 * TODO: All thread should be cooperative (prio < 0)?
 * TODO: check binary semaphore solution equivalent to mutex + conditinal variable solution?
 * TODO: what should be timeoutable
 */

/* -------------------------------------------------------------------------- */

/** TODO: Move to common.h */
#define SERVICE_STACK_SIZE 1024
#define SERVICE_THREAD_PRIORITY -1
#define SERVICE_SYS_INIT_PRIORITY 90

/* -------------------------------------------------------------------------- */

/** @brief set up all necessary ... for the service to run */
void config_service();

/**
 * @brief   Enables service
 * @details Sets the loop condition to true and signals the service thread (sem or condvar)
 */
void enable_service(void);

/**
 * @brief   Disables service
 * @details Sets the loop condition to false, forcing service to yield/wait until signaled to be awaken
 */
void disable_service(void);

#endif //__SERVICE_TEMPLATE_H__
