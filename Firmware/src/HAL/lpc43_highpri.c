#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <arch/irq.h>
#include <arch/chip/chip.h>
#include <arch/board/board.h>
#include "arch/common/up_internal.h"
#include "arch/armv7-m/ram_vectors.h"
#include "lpc43_timer.h"
#include "arch/board/bambino-200e.h"

#include "stdio.h"

#ifndef CONFIG_ARCH_HIPRI_INTERRUPT
#  error CONFIG_ARCH_HIPRI_INTERRUPT is required
#endif

#ifndef CONFIG_ARCH_RAMVECTORS
#  error CONFIG_ARCH_RAMVECTORS is required
#endif

#ifndef CONFIG_LPC43_TMR0
#  error CONFIG_LPC43_TMR0 is required
#endif

#ifndef CONFIG_ARCH_IRQPRIO
#  error CONFIG_ARCH_IRQPRIO is required
#endif

#define putreg32(v,a)  (*(volatile uint32_t *)(a) = (v))
#define LPC43_TMR_IR   0x40084000

static void (*app_handler)();

static void tmr0_handler(void)
{
    /* Acknowledge the timer interrupt */
    putreg32(0x0f, LPC43_TMR_IR);
    app_handler();
}

// frequency in HZ
int highpri_tmr0_setup(uint32_t frequency, void (*handler)())
{
    struct timer_lowerhalf_s *dev;
    int prescaler;
    int ret;

    /* Configure basic timer TMR0 and enable interrupts */
    dev = (struct timer_lowerhalf_s *)lpc43_tmr_init(0);
    if (!dev) {
        printf("highpri_setup: ERROR: lpc43_tmr_init(6) failed\n");
        return -1;
    }

    uint32_t clk_frequency= 102000000; // 102Mhz
    prescaler = LPC43_TMR_SETCLOCK(dev, clk_frequency);
    printf("TIMER0 CLKIN=%d Hz, Frequency=%d Hz, prescaler=%d\n",
           BOARD_FCLKOUT_FREQUENCY, clk_frequency, prescaler);

    uint32_t period = clk_frequency / frequency;
    LPC43_TMR_SETPERIOD(dev, period);
    printf("TMR0 period=%d cyles; interrupt rate=%d Hz\n",
           period, clk_frequency / period);

    /* Attach TIM0 ram vector */
    app_handler= handler;
    ret = up_ramvec_attach(LPC43M4_IRQ_TIMER0, tmr0_handler);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_ramvec_attach failed: %d\n", ret);
        return -1;
    }

    /* Set the priority of the TMR0 interrupt vector */

    ret = up_prioritize_irq(LPC43M4_IRQ_TIMER0, NVIC_SYSH_HIGH_PRIORITY);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_prioritize_irq failed: %d\n", ret);
        return -1;
    }

    /* Enable the timer interrupt at the NVIC and at TMR0 */

    up_enable_irq(LPC43M4_IRQ_TIMER0);
    LPC43_TMR_ENABLEINT(dev, 0);

    return period;
}
