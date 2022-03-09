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

K_SEM_DEFINE(sync_sem, 0 ,1)

void task0(void)
{
    LOG_DBG("task0");
    while (1) {
        k_sleep(K_MSEC(100));
        if (k_sem_take(&sync_sem, K_FOREVER) == 0) {
            LOG_DBG("take sem task0");
        }
    }
}

void task1(void)
{
    LOG_DBG("task1");
    while (1) {
        k_sleep(K_MSEC(10000));
        LOG_DBG("give sem task1");
        k_sem_give(&sync_sem);
    }
}


K_THREAD_DEFINE(task0_id, STACKSIZE, task0, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(task1_id, STACKSIZE, task1, NULL, NULL, NULL, PRIORITY, 0, 0);

