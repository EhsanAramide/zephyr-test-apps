/*
 * Idea: Implementing a system which consists of four thread and semaphores,
 * each thread waits for a semaphore which is given by an ISR.
 * Then each thread toggle state of corresponding LED and any before them.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

// ------------- Start {Peripherals Configs} -------------
static const struct gpio_dt_spec switch0_dt = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec switch1_dt = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static const struct gpio_dt_spec switch2_dt = GPIO_DT_SPEC_GET(DT_ALIAS(sw2), gpios);
static const struct gpio_dt_spec switch3_dt = GPIO_DT_SPEC_GET(DT_ALIAS(sw3), gpios);

static struct gpio_callback switch0_cb;
static struct gpio_callback switch1_cb;
static struct gpio_callback switch2_cb;
static struct gpio_callback switch3_cb;

static struct gpio_dt_spec led0_dt = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct gpio_dt_spec led1_dt = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
static struct gpio_dt_spec led2_dt = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);
static struct gpio_dt_spec led3_dt = GPIO_DT_SPEC_GET(DT_ALIAS(led3), gpios);
// ------------- End {Peripherals Configs} -------------

// ------------- Start {Thread Configs} -------------
#define THREAD_ENTRY_DEFINE(en) void en(void *var1, void *var2, void *var3)
#define THREADS_PRIORITY 5

K_SEM_DEFINE(led0_sem, 0, 4);
K_SEM_DEFINE(led1_sem, 0, 4);
K_SEM_DEFINE(led2_sem, 0, 4);
K_SEM_DEFINE(led3_sem, 0, 4);

THREAD_ENTRY_DEFINE(thread_led0)
{
    ARG_UNUSED(var1);
    ARG_UNUSED(var2);
    ARG_UNUSED(var3);

    while (1)
    {
        (void)k_sem_take(&led0_sem, K_FOREVER);
        gpio_pin_toggle_dt(&led0_dt);
    }
}

THREAD_ENTRY_DEFINE(thread_led1)
{
    ARG_UNUSED(var1);
    ARG_UNUSED(var2);
    ARG_UNUSED(var3);

    while (1)
    {
        (void)k_sem_take(&led1_sem, K_FOREVER);
        gpio_pin_toggle_dt(&led1_dt);
    }
}

THREAD_ENTRY_DEFINE(thread_led2)
{
    ARG_UNUSED(var1);
    ARG_UNUSED(var2);
    ARG_UNUSED(var3);

    while (1)
    {
        (void)k_sem_take(&led2_sem, K_FOREVER);
        gpio_pin_toggle_dt(&led2_dt);
    }
}

THREAD_ENTRY_DEFINE(thread_led3)
{
    ARG_UNUSED(var1);
    ARG_UNUSED(var2);
    ARG_UNUSED(var3);

    while (1)
    {
        (void)k_sem_take(&led3_sem, K_FOREVER);
        gpio_pin_toggle_dt(&led3_dt);
    }
}

K_THREAD_DEFINE(thread_led0_data,
                1024,
                thread_led0,
                NULL, NULL, NULL,
                THREADS_PRIORITY,
                0, 0);

K_THREAD_DEFINE(thread_led1_data,
                1024,
                thread_led1,
                NULL, NULL, NULL,
                THREADS_PRIORITY + 1,
                0, 0);

K_THREAD_DEFINE(thread_led2_data,
                1024,
                thread_led2,
                NULL, NULL, NULL,
                THREADS_PRIORITY + 2,
                0, 0);

K_THREAD_DEFINE(thread_led3_data,
                1024,
                thread_led3,
                NULL, NULL, NULL,
                THREADS_PRIORITY + 3,
                0, 0);

// ------------- End {Thread Configs} -------------

// ------------- Start {ISR Config} -------------
static int counter = 0;
void switch_isr_handler(const struct device *dev,
                        struct gpio_callback *cb,
                        uint32_t pins)
{
    printk("We are in interrupt %d\n", counter++);
    if (cb->pin_mask == BIT(switch0_dt.pin))
    {
        k_sem_give(&led0_sem);
    }
    else if (cb->pin_mask == BIT(switch1_dt.pin))
    {
        k_sem_give(&led0_sem);
        k_sem_give(&led1_sem);
    }
    else if (cb->pin_mask == BIT(switch2_dt.pin))
    {
        k_sem_give(&led0_sem);
        k_sem_give(&led1_sem);
        k_sem_give(&led2_sem);
    }
    else if (cb->pin_mask == BIT(switch3_dt.pin))
    {
        k_sem_give(&led0_sem);
        k_sem_give(&led1_sem);
        k_sem_give(&led2_sem);
        k_sem_give(&led3_sem);
    }
}
// ------------- End {ISR Config} -------------

#define GPIOS_ISNOT_READY_DT(spec1, spec2, spec3, spec4) \
    (!gpio_is_ready_dt(&spec1)) ||                       \
        (!gpio_is_ready_dt(&spec2)) ||                   \
        (!gpio_is_ready_dt(&spec3)) ||                   \
        (!gpio_is_ready_dt(&spec4))

#define GPIOS_PIN_CONFIGURE_NONZERO(spec1, spec2, spec3, spec4, state) \
    (gpio_pin_configure_dt(&spec1, state) != 0) ||                     \
        (gpio_pin_configure_dt(&spec2, state) != 0) ||                 \
        (gpio_pin_configure_dt(&spec3, state) != 0) ||                 \
        (gpio_pin_configure_dt(&spec4, state) != 0)

#define GPIOS_PIN_INT_CONFIGURE_NONZERO(spec1, spec2, spec3, spec4, state) \
    (gpio_pin_interrupt_configure_dt(&spec1, state) != 0) ||               \
        (gpio_pin_interrupt_configure_dt(&spec2, state) != 0) ||           \
        (gpio_pin_interrupt_configure_dt(&spec3, state) != 0) ||           \
        (gpio_pin_interrupt_configure_dt(&spec4, state) != 0)

int main(void)
{
    if (GPIOS_ISNOT_READY_DT(switch0_dt, switch1_dt, switch2_dt, switch3_dt))
    {
        printk("GPIO device isn't ready %d\n", __LINE__);
    }

    if (GPIOS_PIN_CONFIGURE_NONZERO(switch0_dt, switch1_dt, switch2_dt, switch3_dt, GPIO_INPUT))
    {
        printk("GPIO pins can not configure as input\n");
    }

    // Because input gpios set to pull-up with active low state,
    // then it's better to have falling edge detection interrupt.
    if (GPIOS_PIN_INT_CONFIGURE_NONZERO(switch0_dt, switch1_dt, switch2_dt, switch3_dt, GPIO_INT_EDGE_TO_ACTIVE))
    {
        printk("GPIO interrupts can not be configured\n");
    }

    gpio_init_callback(&switch0_cb, switch_isr_handler, BIT(switch0_dt.pin));
    gpio_init_callback(&switch1_cb, switch_isr_handler, BIT(switch1_dt.pin));
    gpio_init_callback(&switch2_cb, switch_isr_handler, BIT(switch2_dt.pin));
    gpio_init_callback(&switch3_cb, switch_isr_handler, BIT(switch3_dt.pin));

    gpio_add_callback(switch0_dt.port, &switch0_cb);
    gpio_add_callback(switch1_dt.port, &switch1_cb);
    gpio_add_callback(switch2_dt.port, &switch2_cb);
    gpio_add_callback(switch3_dt.port, &switch3_cb);

    if (GPIOS_ISNOT_READY_DT(led0_dt, led1_dt, led2_dt, led3_dt))
    {
        printk("GPIO device isn't ready %d\n", __LINE__);
    }

    if (GPIOS_PIN_CONFIGURE_NONZERO(led0_dt, led1_dt, led2_dt, led3_dt, GPIO_OUTPUT))
    {
        printk("GPIO pins can not configure as output\n");
    }

    gpio_pin_set_dt(&led0_dt, 0);
    gpio_pin_set_dt(&led1_dt, 0);
    gpio_pin_set_dt(&led2_dt, 0);
    gpio_pin_set_dt(&led3_dt, 0);

    while (1)
    {
        k_msleep(1000);
    }

    return 0;
}
