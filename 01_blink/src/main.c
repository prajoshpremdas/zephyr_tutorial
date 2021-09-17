/*
 * Copyright (c) 2021 Prajosh Premdas <premdas.prajosh@gmail.com>
 *
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <sys/printk.h>
#include <drivers/gpio.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0        DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED0_PIN    DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS    DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0    ""
#define PIN    0
#define FLAGS    0
#endif

void main(void)
{
    const struct device *dev;
    bool led_is_on = true;
    int ret;

    printk("Hello World! %s\n", CONFIG_BOARD);

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    ret = gpio_pin_configure(dev, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    if (ret < 0) {
        return;
    }

    while (1) {
        gpio_pin_set(dev, LED0_PIN, (int)led_is_on);
        led_is_on = !led_is_on;
        k_msleep(SLEEP_TIME_MS);
    }
}
