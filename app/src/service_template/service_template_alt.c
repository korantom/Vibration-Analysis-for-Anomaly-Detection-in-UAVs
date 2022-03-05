#include "service_template.h"

/* binary semaphore */
K_SEM_DEFINE(service_sem, 0, 1); // semaphore
static bool is_enabled = false;  // condition

/* define all variables and structs */
// ...

LOG_MODULE_REGISTER(service_template);

/* public interface ---------------------------------------------------------- */

void config_service()
{
    LOG_INF("config_service()");
}

void enable_service()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("enable_service()");
    LOG_INF("semaphore give");
    is_enabled = true;
    k_sem_give(&service_sem);
}

void disable_service()
{
    /* will be called from another thread (shell, main, ...) */
    LOG_INF("disable_service()");
    is_enabled = false;
}

/* private functions --------------------------------------------------------- */

static void _service();

static void _service_loop()
{

    LOG_INF("thr: %p, Service thread started", k_current_get());

    while (1)
    {
        if (!is_enabled)
        {
            LOG_INF("semaphore taking");
            k_sem_take(&service_sem, K_FOREVER);
            LOG_INF("semaphore taken");
        }

        /* Perform (critical) service functionality ... */
        _service();

        LOG_INF("yielding");
        k_yield();
    }
}
