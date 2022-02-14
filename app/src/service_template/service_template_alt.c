#include "service_template.h"

// TODO: FIX, doesn't work properly

/* binary semaphore */
K_SEM_DEFINE(service_sem, 0, 1); // semaphore
static bool is_enabled = false;  // condition

/* define all variables and structs */
// same

/* public interface ---------------------------------------------------------- */

void config_service()
{
    // ...
}

void enable_service()
{
    /* will be called from another thread (shell, main, ...) */
    k_sem_give(&service_sem);
    is_enabled = true;
}

void disable_service()
{
    /* will be called from another thread (shell, main, ...) */
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _service();

static void _service_loop()
{

    printk("Service Thread %p started\n", k_current_get());

    while (1)
    {
        printk("S: taking semaphore\n");
        k_sem_take(&service_sem, K_FOREVER);
        printk("S: taken semaphore\n");

        if (!is_enabled)
        {
            printk("S: disabled\n");
            k_yield();
            continue; // problem?
        }

        printk("S: enabled\n");

        /* Perform (critical) service functionality ... */
        _service();

        printk("S: giving semaphore\n");
        k_sem_give(&service_sem);
        printk("S: gave semaphore\n");

        k_yield();
    }
}

static void _service()
{
    /* Service functionality */
}
/* thread creation ----------------------------------------------------------- */

// same

/* shell commands ------------------------------------------------------------ */

// same
