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

// TODO move ramfunc define to a utils.h
#define _ramfunc_ __attribute__ ((section(".ramfunctions"),long_call,noinline))

#ifndef CONFIG_ARCH_HIPRI_INTERRUPT
#  error CONFIG_ARCH_HIPRI_INTERRUPT is required
#endif

#ifndef CONFIG_ARCH_RAMVECTORS
#  error CONFIG_ARCH_RAMVECTORS is required
#endif

#ifndef CONFIG_LPC43_TMR0
#  error CONFIG_LPC43_TMR0 is required
#endif

#ifndef CONFIG_LPC43_TMR1
#  error CONFIG_LPC43_TMR1 is required
#endif

#ifndef CONFIG_ARCH_IRQPRIO
#  error CONFIG_ARCH_IRQPRIO is required
#endif

#define putreg32(v,a)  (*(volatile uint32_t *)(a) = (v))
#define getreg32(a)    (*(volatile uint32_t *)(a))

static void (*tick_handler)();

_ramfunc_ static void tmr0_handler(void)
{
    /* Acknowledge the timer interrupt */
    putreg32(0x0f, LPC43_TMR0_IR);
    tick_handler();
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

    uint32_t clk_frequency = 102000000; // 102Mhz
    prescaler = LPC43_TMR_SETCLOCK(dev, clk_frequency);
    printf("TIMER0 CLKIN=%d Hz, Frequency=%d Hz, prescaler=%d\n",
           BOARD_FCLKOUT_FREQUENCY, clk_frequency, prescaler);

    uint32_t period = clk_frequency / frequency;
    LPC43_TMR_SETPERIOD(dev, period);
    printf("TMR0 period=%d cyles; interrupt rate=%d Hz\n",
           period, clk_frequency / period);

    /* Attach TIM0 ram vector */
    tick_handler = handler;
    ret = up_ramvec_attach(LPC43M4_IRQ_TIMER0, tmr0_handler);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_ramvec_attach failed for tmr0: %d\n", ret);
        return -1;
    }

    /* Set the priority of the TMR0 interrupt vector */

    ret = up_prioritize_irq(LPC43M4_IRQ_TIMER0, NVIC_SYSH_HIGH_PRIORITY);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_prioritize_irq failed for tmr0: %d\n", ret);
        return -1;
    }

    /* Enable the timer interrupt at the NVIC and at TMR0 */

    up_enable_irq(LPC43M4_IRQ_TIMER0);
    LPC43_TMR_ENABLEINT(dev, 0);

    // return the inaccuracy of the frequency if it does not exactly divide the frequency
    return clk_frequency % period;
}

// setup the one shot timer used for unstep, delay is in microseconds
// We can't use NUTTX low level calls as they always set it up as repeating not one shot
static void (*untick_handler)();
_ramfunc_ static void tmr1_handler(void)
{
    /* Acknowledge the timer interrupt */
    putreg32(0x0f, LPC43_TMR1_IR);
    untick_handler();
}

int highpri_tmr1_setup(uint32_t delay, void (*handler)())
{
    struct timer_lowerhalf_s *dev;
    int prescaler;
    int ret;

    /* Configure basic timer TMR1 and enable interrupts */
    dev = (struct timer_lowerhalf_s *)lpc43_tmr_init(1);
    if (!dev) {
        printf("highpri_setup: ERROR: lpc43_tmr_init(1) failed\n");
        return -1;
    }

    uint32_t clk_frequency = 102000000; // 102Mhz
    prescaler = LPC43_TMR_SETCLOCK(dev, clk_frequency);
    printf("TIMER1 CLKIN=%d Hz, Frequency=%d Hz, prescaler=%d\n",
           BOARD_FCLKOUT_FREQUENCY, clk_frequency, prescaler);

    uint32_t period = clk_frequency / (delay * 1000000); // delay is in us
    LPC43_TMR_SETPERIOD(dev, period);
    printf("TMR1 period=%d cycles; pulse width=%d us\n",
           period, clk_frequency / (period * 1000000));

    putreg32(period, LPC43_TIMER1_BASE + LPC43_TMR_MR0_OFFSET);
    putreg32(0, LPC43_TIMER1_BASE + LPC43_TMR_CCR_OFFSET); /* do not use capture */

    // Attach TIM1 ram vector
    untick_handler = handler;
    ret = up_ramvec_attach(LPC43M4_IRQ_TIMER1, tmr1_handler);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_ramvec_attach failed for tmr1: %d\n", ret);
        return -1;
    }

    // Set the priority of the TMR1 interrupt vector
    // TODO should be higher than stepticker but NUTTX won't allow this as it has only 1 high priority interrupt
    ret = up_prioritize_irq(LPC43M4_IRQ_TIMER1, NVIC_SYSH_HIGH_PRIORITY);
    if (ret < 0) {
        printf("highpri_setup: ERROR: up_prioritize_irq failed for tmr1: %d\n", ret);
        return -1;
    }

    // set the timer for one shot
    uint32_t regval = getreg32(LPC43_TIMER1_BASE + LPC43_TMR_MCR_OFFSET);
    regval |= 5; // match on MR0, stop on match
    putreg32(regval, LPC43_TIMER1_BASE + LPC43_TMR_MCR_OFFSET);

    // Enable the timer interrupt at the NVIC, DO NOT start at this time
    up_enable_irq(LPC43M4_IRQ_TIMER1);

    // return the inaccuracy of the frequency if it does not exactly divide the frequency
    return clk_frequency % period;
}

//called from within TMR0 ISR so must be in SRAM
_ramfunc_ void highpri_tmr1_start()
{
  // Start timer1
  putreg32(TMR_TCR_RESET, LPC43_TIMER1_BASE + LPC43_TMR_TCR_OFFSET);
  putreg32(TMR_TCR_EN, LPC43_TIMER1_BASE + LPC43_TMR_TCR_OFFSET);
}
