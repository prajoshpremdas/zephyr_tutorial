/*
 * Copyright (c) 2021 Prajosh Premdas <premdas.prajosh@gmail.com>
 *
 */

#include <zephyr.h>
#include <device.h>
#include <string.h>
#include <inttypes.h>
#include <devicetree.h>
#include <sys/printk.h>
#include <drivers/gpio.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
#define LOG_MODULE_NAME task
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE   DT_ALIAS(led0)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
#define LED0        DT_GPIO_LABEL(LED0_NODE, gpios)
#define LED0_PIN    DT_GPIO_PIN(LED0_NODE, gpios)
#define LED0_FLAGS  DT_GPIO_FLAGS(LED0_NODE, gpios)
#else
/* A build error here means your board isn't set up to blink an LED. */
#error "Unsupported board: led0 devicetree alias is not defined"
#define LED0    ""
#define PIN     0
#define FLAGS   0
#endif

/* The devicetree node identifier for the "sw0" alias. */
#define SW0_NODE    DT_ALIAS(sw0)

#if DT_NODE_HAS_STATUS(SW0_NODE, okay)
#define SW0_GPIO_LABEL    DT_GPIO_LABEL(SW0_NODE, gpios)
#define SW0_GPIO_PIN    DT_GPIO_PIN(SW0_NODE, gpios)
#define SW0_GPIO_FLAGS    (GPIO_INPUT | DT_GPIO_FLAGS(SW0_NODE, gpios))
#else
#error "Unsupported board: sw0 devicetree alias is not defined"
#define SW0_GPIO_LABEL    ""
#define SW0_GPIO_PIN    0
#define SW0_GPIO_FLAGS    0
#endif

/* size of stack area used by each thread */
#define STACKSIZE 1024

/* scheduling priority used by each thread */
#define PRIORITY 7

K_SEM_DEFINE(sync_sem, 0 ,1);
static struct gpio_callback button_cb_data;

static void led_toggle(void)
{
    static bool led_is_on = false;
    const struct device *dev;

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    led_is_on = !led_is_on;
    gpio_pin_set(dev, LED0_PIN, (int)led_is_on);
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
            uint32_t pins)
{
    LOG_DBG("Button pressed at %" PRIu32, k_cycle_get_32());
    k_sem_give(&sync_sem);
}

static int button_init(void)
{
    const struct device *button;
    int ret;

    button = device_get_binding(SW0_GPIO_LABEL);
    if (button == NULL) {
        LOG_ERR("Error: didn't find %s device", SW0_GPIO_LABEL);
        return -1;
    }

    ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure %s pin %d",
               ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
        return ret;
    }

    ret = gpio_pin_interrupt_configure(button,
                       SW0_GPIO_PIN,
                       GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
            ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
    gpio_add_callback(button, &button_cb_data);
    LOG_DBG("Set up button at %s pin %d", SW0_GPIO_LABEL, SW0_GPIO_PIN);

    return 0;
}

static int led_init(void)
{
    const struct device *dev;
    int ret;

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return -1;
    }

    ret = gpio_pin_configure(dev, LED0_PIN, GPIO_OUTPUT_ACTIVE | LED0_FLAGS);
    if (ret < 0) {
        return ret;
    }
    gpio_pin_set(dev, LED0_PIN, 0);
    return 0;
}

void led_task(void)
{
    LOG_DBG("led_started");
    led_init();

    while (1) {
        k_sleep(K_MSEC(100));
    }
}

void button_task(void)
{
    button_init();
    LOG_DBG("button_started");

    while (1) {
        if (k_sem_take(&sync_sem, K_FOREVER) == 0) {
            LOG_DBG("take sem task0");
            led_toggle();
        }
    }
}

K_THREAD_DEFINE(led_id, STACKSIZE, led_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(switch_id, STACKSIZE, button_task, NULL, NULL, NULL, PRIORITY, 0, 0);
