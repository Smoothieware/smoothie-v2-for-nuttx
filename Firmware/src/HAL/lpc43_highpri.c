#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <arch/irq.h>
#include <arch/chip/chip.h>
#include <arch/board/board.h>
#include "arch/common/up_internal.h"
#include "arch/armv7-m/ram_vectors.h"
#include "arch/armv7-m/nvic.h"

#include "lpc43_timer.h"

#ifndef ARCH_BOARD_H
#error "No ARCH_BOARD_H defined"
#endif

#include ARCH_BOARD_H

#include "stdio.h"

#include <math.h>

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

#ifndef CONFIG_ARCH_IRQPRIO
#  error CONFIG_ARCH_IRQPRIO is required
#endif

#define putreg32(v,a)  (*(volatile uint32_t *)(a) = (v))
#define getreg32(a)    (*(volatile uint32_t *)(a))

static void (*tick_handler)();
static void (*untick_handler)();
// the period for the unstep tick
static uint32_t delay_period;

_ramfunc_ static void tmr0_handler(void)
{
    uint32_t r = getreg32(LPC43_TMR0_IR);

    /* Acknowledge the timer interrupt(s) */
    putreg32(r, LPC43_TMR0_IR);

    if(r & 0x02) { // MR1 match interrupt
        // disable the MR1 match interrupt as it is a one shot
        uint32_t regval = getreg32(LPC43_TIMER0_BASE + LPC43_TMR_MCR_OFFSET);
        regval &= ~0x08; // disable MR1 match interrupt
        putreg32(regval, LPC43_TIMER0_BASE + LPC43_TMR_MCR_OFFSET);
        // call upstream handler
        untick_handler();
    }

    if(r & 0x01) tick_handler(); // MR0 match interrupt
}

// frequency in HZ, delay in microseconds
int highpri_tmr0_setup(uint32_t frequency, uint32_t delay, void *mr0handler, void *mr1handler)
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

    // setup step tick period
    uint32_t period1 = clk_frequency / frequency;
    LPC43_TMR_SETPERIOD(dev, period1);
    printf("TMR0 MR0 period=%d cyles; interrupt rate=%d Hz\n", period1, clk_frequency / period1);

    // calculate ideal period for MR1 for unstep interrupt (can't use NUTTX for this)
    // we do not set it here as it will need to add the current TC when it is enabled
    // note that the MR1 match interrupt starts off disabled
    delay_period = floorf(delay / (1000000.0F / clk_frequency)); // delay is in us
    printf("TMR0 MR1 period=%d cycles; pulse width=%d us\n", delay_period, (delay_period*1000000)/clk_frequency);

    // setup the upstream handlers for each interrupt
    tick_handler = mr0handler;
    untick_handler = mr1handler;

    /* Attach TIM0 ram vector */
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
    return clk_frequency % period1;
}

// called from within TMR0 ISR so must be in SRAM
_ramfunc_ void highpri_tmr0_mr1_start()
{
    // we read the current TC and add that to the period we need so we get the full pulse width
    // as it takes so much time to call this from when the MR0 happend we could be be past the MR1 period
    // by adding in the current TC we guarantee we are going to hit the MR1 match point
    // TODO we should really check this does not exceed the MR0 match otherwise unstep will not happen this step cycle
    uint32_t tc = getreg32(LPC43_TIMER0_BASE + LPC43_TMR_TC_OFFSET);
    putreg32(delay_period+tc, LPC43_TIMER0_BASE + LPC43_TMR_MR1_OFFSET);

    // enable the MR1 match interrupt
    uint32_t regval = getreg32(LPC43_TIMER0_BASE + LPC43_TMR_MCR_OFFSET);
    regval |= 0x08; // enable MR1 match interrupt, but keep going with MR0
    putreg32(regval, LPC43_TIMER0_BASE + LPC43_TMR_MCR_OFFSET);
}

_ramfunc_  void fire_pendsv()
{
    putreg32(NVIC_INTCTRL_PENDSVSET, NVIC_INTCTRL); // may work on 4330 in nuttx
}

static void (*pendsv_handler)();
_ramfunc_ static int pendsv_irq(int irq, FAR void *context, FAR void *arg)
{
    if(pendsv_handler != NULL) {
        pendsv_handler();
    }
    return 0;
}

void setup_pendsv(void *handler)
{
    pendsv_handler= handler;
    if(handler != NULL) {
        irq_attach(LPC43_IRQ_PENDSV, pendsv_irq, NULL);
    }else{
        irq_detach(LPC43_IRQ_PENDSV);
    }
}
