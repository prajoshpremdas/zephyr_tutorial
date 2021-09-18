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

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

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

#define MY_WORK_STACK_SIZE      512
#define MY_WORK_PRIORITY        5

#define INIT_PRIORITY           10

K_THREAD_STACK_DEFINE(my_stack_area, MY_WORK_STACK_SIZE);
static struct k_work_q my_work_q;

struct device_info {
    struct k_work work;
    char name[16];
} my_device;

static struct gpio_callback button_cb_data;

static void work_worker(struct k_work *item)
{
    static bool led_is_on = false;
    const struct device *dev;
    struct device_info *the_device =
        CONTAINER_OF(item, struct device_info, work);

    printk("Name of device %s\n", the_device->name);

    dev = device_get_binding(LED0);
    if (dev == NULL) {
        return;
    }

    led_is_on = !led_is_on;
    gpio_pin_set(dev, LED0_PIN, (int)led_is_on);
}

static int work_init(const struct device *device)
{
    ARG_UNUSED(device);

    k_work_q_start(&my_work_q, my_stack_area,
                   K_THREAD_STACK_SIZEOF(my_stack_area), MY_WORK_PRIORITY);
    k_thread_name_set(&my_work_q.thread, "my_workq");

    /* initialize name info for a device */
    strcpy(my_device.name, "my_device");

    /* initialize work item for printing device's error messages */
    k_work_init(&my_device.work, work_worker);

    return 0;
}

static void button_pressed(const struct device *dev, struct gpio_callback *cb,
            uint32_t pins)
{
    printk("Button pressed at %" PRIu32 "\n", k_cycle_get_32());
    k_work_submit_to_queue(&my_work_q, &my_device.work);
}

static int button_init(const struct device *device)
{
    const struct device *button;
    int ret;

    ARG_UNUSED(device);

    button = device_get_binding(SW0_GPIO_LABEL);
    if (button == NULL) {
        printk("Error: didn't find %s device\n", SW0_GPIO_LABEL);
        return -1;
    }

    ret = gpio_pin_configure(button, SW0_GPIO_PIN, SW0_GPIO_FLAGS);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n",
               ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
        return ret;
    }

    ret = gpio_pin_interrupt_configure(button,
                       SW0_GPIO_PIN,
                       GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n",
            ret, SW0_GPIO_LABEL, SW0_GPIO_PIN);
        return ret;
    }

    gpio_init_callback(&button_cb_data, button_pressed, BIT(SW0_GPIO_PIN));
    gpio_add_callback(button, &button_cb_data);
    printk("Set up button at %s pin %d\n", SW0_GPIO_LABEL, SW0_GPIO_PIN);

    return 0;
}

static int led_init(const struct device *device)
{
    const struct device *dev;
    int ret;

    ARG_UNUSED(device);

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



SYS_INIT(work_init, POST_KERNEL, INIT_PRIORITY);
SYS_INIT(button_init, POST_KERNEL, INIT_PRIORITY);
SYS_INIT(led_init, POST_KERNEL, INIT_PRIORITY);
