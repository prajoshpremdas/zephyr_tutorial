/*
 * Copyright (c) 2021 Prajosh Premdas <premdas.prajosh@gmail.com>
 *
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <sys/__assert.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
#define LOG_MODULE_NAME task
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

K_MUTEX_DEFINE(sync_mutex);

static uint32_t count = 0;

void task0(void)
{
    LOG_DBG("task0");
    while (1) {
        k_sleep(K_MSEC(10000));
        if (k_mutex_lock(&sync_mutex, K_FOREVER) == 0) {
            count++;
            LOG_DBG("got sem task0 %d", count);
            k_mutex_unlock(&sync_mutex);
        }
    }
}

void task1(void)
{
    LOG_DBG("task1");
    while (1) {
        k_sleep(K_MSEC(1000));
        if (k_mutex_lock(&sync_mutex, K_FOREVER) == 0) {
            LOG_DBG("got sem task1 %d", count);
            k_mutex_unlock(&sync_mutex);
        }
    }
}


K_THREAD_DEFINE(task0_id, STACKSIZE, task0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(task1_id, STACKSIZE, task1, NULL, NULL, NULL, PRIORITY, 0, 0);

