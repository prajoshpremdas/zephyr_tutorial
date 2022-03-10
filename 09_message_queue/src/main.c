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
#include <random/rand32.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
#define LOG_MODULE_NAME task
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

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

typedef struct {
    uint32_t seq;
    uint32_t timestamp;
    uint32_t data;
    uint32_t randnum;
}__attribute__((aligned(4))) msg;

K_MSGQ_DEFINE(queue, sizeof(msg), 10, 4);

static struct gpio_callback button_cb_data;
static uint32_t count;

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
            uint32_t pins)
{
    msg rx_data;

    count++;
    rx_data.seq = count;
    rx_data.timestamp = k_cycle_get_32();
    rx_data.data = 1234567890;
    rx_data.randnum = sys_rand32_get();

    LOG_DBG("Button pressed at %" PRIu32, rx_data.timestamp);

    while (k_msgq_put(&queue, &rx_data, K_NO_WAIT) != 0) {
        k_msgq_purge(&queue);
    }
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

void rx_task(void)
{
    msg rx_data;

    LOG_DBG("rx started");

    while (1) {
        k_sleep(K_MSEC(100));
        k_msgq_get(&queue, &rx_data, K_FOREVER);
        LOG_DBG("%d %d %d %d", rx_data.seq, rx_data.timestamp, rx_data.data, rx_data.randnum);
    }
}

void main(void)
{
    LOG_DBG("tx started");
    button_init();

    /*while (1) {
        k_sleep(K_MSEC(100));

    }*/
}

//K_THREAD_DEFINE(tx_id, STACKSIZE, tx_task, NULL, NULL, NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(rx_id, STACKSIZE, rx_task, NULL, NULL, NULL, PRIORITY, 0, 0);
