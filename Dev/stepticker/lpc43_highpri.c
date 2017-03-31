/****************************************************************************
 * configs/bambino-200e/src/stm32_highpri.c
 *
 *   Copyright (C) 2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <nuttx/arch.h>

#include <arch/irq.h>
#include <arch/chip/chip.h>
#include <arch/board/board.h>

#include "up_internal.h"
#include "ram_vectors.h"
#include "lpc43_timer.h"

#include "bambino-200e.h"

#ifdef CONFIG_BAMBINO_HIGHPRI

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/

#ifndef CONFIG_ARCH_CHIP_LPC43XX
#  warning This only only been verified with CONFIG_ARCH_CHIP_LPC43XX
#endif

#ifndef CONFIG_ARCH_HIPRI_INTERRUPT
#  error CONFIG_ARCH_HIPRI_INTERRUPT is required
#endif

#ifndef CONFIG_ARCH_RAMVECTORS
#  error CONFIG_ARCH_RAMVECTORS is required
#endif

#ifndef CONFIG_LPC43_TMR0
#  error CONFIG_LPC43_TMR0 is required
#endif

#ifndef CONFIG_BAMBINO_TMR0_FREQUENCY
#  warning CONFIG_BAMBINO_TMR0_FREQUENCY defaulting to BOARD_FCLKOUT_FREQUENCY
#  define CONFIG_BAMBINO_TMR0_FREQUENCY BOARD_FCLKOUT_FREQUENCY
#endif

#ifndef CONFIG_BAMBINO_TMR0_PERIOD
#  warning CONFIG_BAMBINO_TMR0_PERIOD defaulting to 1MS
#  define CONFIG_BAMBINO_TMR0_PERIOD (CONFIG_BAMBINO_TMR0_FREQUENCY / 1000)
#endif

#ifndef CONFIG_ARCH_IRQPRIO
#  error CONFIG_ARCH_IRQPRIO is required
#endif

/* Redefined here to improve performance */

#define putreg8(v,a)   (*(volatile uint8_t *)(a) = (v))
#define putreg32(v,a)  (*(volatile uint32_t *)(a) = (v))

#define LPC43_TMR_IR   0x40084000
#define GPIO_TEST_ADDR 0x400f402a  /* GPIO1_10 address */

/****************************************************************************
 * Private Data
 ****************************************************************************/

struct highpri_s
{
  FAR struct timer_lowerhalf_s *dev;  /* TIM6 driver instance */
  volatile uint64_t basepri[16];
  volatile uint64_t handler;
  volatile uint64_t thread;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct highpri_s g_highpri;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: tmr0_handler
 *
 * Description:
 *   This is the handler for the high speed TMR0 interrupt.
 *
 ****************************************************************************/

void tmr0_handler(void)
{
  uint8_t basepri;
  static uint8_t tobit = 0;
  int index;

  /* Acknowledge the timer interrupt */

  putreg32(0x0f, LPC43_TMR_IR);

  /* togle GPIO1_10 */
  putreg8(tobit, GPIO_TEST_ADDR);
  tobit ^= 1;

  /* Increment the count associated with the current basepri */

  /*basepri = getbasepri();
  index   = ((basepri >> 4) & 15);
  g_highpri.basepri[index]++;*/

  /* Check if we are in an interrupt handle */

  //if (up_interrupt_context())
    {
      g_highpri.handler++;
    }
  /*else
    {
      g_highpri.thread++;
    }*/
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: highpri_main
 *
 * Description:
 *   Main entry point in into the high priority interrupt test.
 *
 ****************************************************************************/

int highpri_main(int argc, char *argv[])
{
  FAR struct timer_lowerhalf_s *dev;
  uint64_t basepri[16];
  uint64_t handler;
  uint64_t thread;
  uint64_t total;
  uint32_t seconds;
  int prescaler;
  int ret;
  int i;

  printf("highpri_main: Started\n");

  /* Configure pin TEST as GPIOs */
  
  lpc43_pin_config(PINCONFIG_TEST);
  lpc43_gpio_config(GPIO_TEST);

  /* Configure basic timer TMR0 and enable interrupts */

  dev = lpc43_tmr_init(0);
  if (!dev)
    {
      fprintf(stderr, "highpri_main: ERROR: stm32_tim_init(6) failed\n");
      return EXIT_FAILURE;
    }

  g_highpri.dev = dev;

  prescaler = LPC43_TMR_SETCLOCK(dev, CONFIG_BAMBINO_TMR0_FREQUENCY);
  printf("TIMER0 CLKIN=%d Hz, Frequency=%d Hz, prescaler=%d\n",
         BOARD_FCLKOUT_FREQUENCY, 1000000, prescaler);

  LPC43_TMR_SETPERIOD(dev, CONFIG_BAMBINO_TMR0_PERIOD);
  printf("TMR0 period=%d cyles; interrupt rate=%d Hz\n",
         CONFIG_BAMBINO_TMR0_PERIOD,
         CONFIG_BAMBINO_TMR0_FREQUENCY/CONFIG_BAMBINO_TMR0_PERIOD);

  /* Attach TIM0 ram vector */

  ret = up_ramvec_attach(LPC43M4_IRQ_TIMER0, tmr0_handler);
  if (ret < 0)
    {
      fprintf(stderr, "highpri_main: ERROR: up_ramvec_attach failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Set the priority of the TMR0 interrupt vector */

  ret = up_prioritize_irq(LPC43M4_IRQ_TIMER0, NVIC_SYSH_HIGH_PRIORITY);
  if (ret < 0)
    {
      fprintf(stderr, "highpri_main: ERROR: up_prioritize_irq failed: %d\n", ret);
      return EXIT_FAILURE;
    }

  /* Enable the timer interrupt at the NVIC and at TMR0 */

  up_enable_irq(LPC43M4_IRQ_TIMER0);
  LPC43_TMR_ENABLEINT(dev, 0);

  /* Monitor interrupts */

  seconds = 0;
  for (;;)
    {
      /* Flush stdout and wait a bit */

      fflush(stdout);
      sleep(1);
      seconds++;

      /* Sample counts so that they are not volatile.  Missing a count now
       * and then is a normal consequence of this design.
       */

      for (i = 0; i < 16; i++)
        {
          basepri[i] = g_highpri.basepri[i];
        }

      handler = g_highpri.handler;
      thread  = g_highpri.thread;

      /* Then print out what is happening */

      printf("Elapsed time: %d seconds\n\n", seconds);
      for (i = 0, total = 0; i < 16; i++)
        {
          total += basepri[i];
        }

      if (total > 0)
        {
          for (i = 0; i < 16; i++)
            {
              if (basepri[i] > 0)
                {
                  printf("  basepri[%02x]: %lld (%d%%)\n",
                         i << 4, basepri[i],
                         (int)((100* basepri[i] + (total / 2)) / total));
                }
            }
        }

      total = handler + thread;
      if (total > 0)
        {
          printf("  Handler:     %lld (%d%%)\n",
                 handler,  (int)((100*handler + (total / 2)) / total));
          printf("  Thread:      %lld (%d%%)\n\n",
                 thread,   (int)((100*thread + (total / 2)) / total));
        }
    }

  return EXIT_SUCCESS;
}

#endif /* CONFIG_VIEWTOOL_HIGHPRI */
