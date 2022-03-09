/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
#define LOG_MODULE_NAME main
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

static void tmr1_expiry_func(struct k_timer *timer); 

K_TIMER_DEFINE(tmr1, tmr1_expiry_func, NULL);

static void tmr1_expiry_func(struct k_timer *timer)
{
	printk("tmr1 exp\n\r");
	LOG_DBG("ltmr1 expiry");
}

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);
	LOG_DBG("l-Hello world");
	LOG_WRN("start");
	k_timer_start(&tmr1, K_SECONDS(10), K_SECONDS(10));
}
