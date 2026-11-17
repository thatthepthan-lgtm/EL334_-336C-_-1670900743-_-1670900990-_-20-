/*
 * timer.c - Periodic Interrupt Timer, channel 0 (system tick)
 *
 * PIT0 interrupts every 1 ms. Its ISR is the "heartbeat" of the program:
 *   1. counts milliseconds (g_msTicks)
 *   2. refreshes one digit of the 7-segment display (multiplexing)
 *   3. runs the debounce state machine of the switches
 *   4. advances the melody player (note durations)
 *
 * LDVAL = (bus clock / 1000) - 1 , because the PIT counts LDVAL..0 (LDVAL+1 cycles).
 */
#include "board.h"
#include "system_MK64F12.h"
#include "timer.h"
#include "seg7.h"
#include "switch.h"
#include "melody.h"

/* Reset clock of the K64F (FEI mode, 32.768 kHz x 640). Some versions of
 * system_MK64F12.h (e.g. the Keil device pack) do not define this name.  */
#ifndef DEFAULT_SYSTEM_CLOCK
#define DEFAULT_SYSTEM_CLOCK    20971520U
#endif

static volatile uint32_t g_msTicks = 0U;
static uint32_t g_busClockHz = DEFAULT_SYSTEM_CLOCK;

/* Work out the bus clock from the current clock tree:
 * MCGOUTCLK = core * (OUTDIV1+1), bus = MCGOUTCLK / (OUTDIV2+1)            */
static uint32_t timer_calcBusClock(void)
{
    uint32_t div1, div2;
    SystemCoreClockUpdate();
    div1 = ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV1_MASK) >> SIM_CLKDIV1_OUTDIV1_SHIFT) + 1U;
    div2 = ((SIM->CLKDIV1 & SIM_CLKDIV1_OUTDIV2_MASK) >> SIM_CLKDIV1_OUTDIV2_SHIFT) + 1U;
    return (SystemCoreClock * div1) / div2;
}

void timer_init(void)
{
    g_busClockHz = timer_calcBusClock();

    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;       /* clock gate for the PIT module   */
    PIT->MCR = 0U;                          /* MDIS = 0 : module enabled       */

    PIT->CHANNEL[0].TCTRL = 0U;                             /* stop while configuring */
    PIT->CHANNEL[0].LDVAL = (g_busClockHz / 1000U) - 1U;    /* 1 ms period            */
    PIT->CHANNEL[0].TFLG  = PIT_TFLG_TIF_MASK;              /* clear old flag (w1c)   */
    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;

    NVIC_SetPriority(PIT0_IRQn, PRIO_PIT0_TICK);
    NVIC_ClearPendingIRQ(PIT0_IRQn);
    NVIC_EnableIRQ(PIT0_IRQn);
}

uint32_t timer_millis(void)     { return g_msTicks; }
uint32_t timer_busClockHz(void) { return g_busClockHz; }

/* PIT channel 0 interrupt service routine (vector name from startup file) */
void PIT0_IRQHandler(void)
{
    PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;   /* acknowledge                       */
    (void)PIT->CHANNEL[0].TFLG;                 /* read back: flush write buffer so  */
                                                /* the IRQ is not re-entered at once */
    g_msTicks++;
    seg7_refresh();         /* show the next digit                 */
    sw_tick1ms();           /* debounce S1..S3                     */
    melody_tick1ms();       /* note timing for the buzzer          */
}
