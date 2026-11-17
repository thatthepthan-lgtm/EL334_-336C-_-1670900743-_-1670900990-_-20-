/*
 * buzzer.c - Buzzer driver
 *
 * Hardware : PTA1 -> R9 10k -> base of PNP transistor Q1 (3906) -> LS1.
 *            The emitter of Q1 is at +5V (shield supply).
 *
 *   PTA1 output LOW  : base current flows, Q1 conducts      -> buzzer ON
 *   PTA1 output HIGH : the K64F HIGH is only 3.3V, so the base is still
 *                      ~1.7V below the emitter and Q1 stays partly ON
 *                      -> the buzzer can keep sounding quietly!
 *   PTA1 input (Hi-Z): almost no base current, Q1 off        -> buzzer OFF
 *
 * So the pin output latch is always 0 and the buzzer is switched by the
 * direction bit (PDDR): output = sound, input = silence.
 *
 * Tone     : PIT channel 1 interrupts at 2 x f. The ISR toggles the
 *            direction bit, giving a square wave of frequency f.
 *            LDVAL = bus / (2 * f) - 1
 */
#include "board.h"
#include "buzzer.h"
#include "timer.h"

#define BUZZER_SOUND()   (PTA->PDDR |=  MASK(BUZZER_PIN))   /* drive LOW  */
#define BUZZER_SILENT()  (PTA->PDDR &= ~MASK(BUZZER_PIN))   /* Hi-Z input */

void buzzer_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
    PORTA->PCR[BUZZER_PIN] = PORT_PCR_MUX(1);   /* GPIO, no pull resistor     */
    PTA->PCOR = MASK(BUZZER_PIN);               /* output latch = 0 (forever) */
    BUZZER_SILENT();                            /* start silent (input)       */

    PIT->CHANNEL[1].TCTRL = 0U;
    PIT->CHANNEL[1].TFLG  = PIT_TFLG_TIF_MASK;
    NVIC_SetPriority(PIT1_IRQn, PRIO_PIT1_TONE);
    NVIC_ClearPendingIRQ(PIT1_IRQn);
    NVIC_EnableIRQ(PIT1_IRQn);
}

void buzzer_tone(uint16_t freqHz)
{
    if (freqHz == 0U) {
        buzzer_off();
        return;
    }
    PIT->CHANNEL[1].TCTRL = 0U;                                 /* stop    */
    PIT->CHANNEL[1].LDVAL = (timer_busClockHz() / (2U * (uint32_t)freqHz)) - 1U;
    PIT->CHANNEL[1].TFLG  = PIT_TFLG_TIF_MASK;
    PIT->CHANNEL[1].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK; /* restart */
}

void buzzer_on(void)
{
    PIT->CHANNEL[1].TCTRL = 0U;
    BUZZER_SOUND();
}

void buzzer_off(void)
{
    PIT->CHANNEL[1].TCTRL = 0U;                 /* stop the tone timer first, */
    PIT->CHANNEL[1].TFLG  = PIT_TFLG_TIF_MASK;  /* drop a pending toggle so   */
    NVIC_ClearPendingIRQ(PIT1_IRQn);            /* it cannot re-enable sound, */
    BUZZER_SILENT();                            /* then release the pin       */
}

/* PIT channel 1 ISR : one half period has elapsed -> flip the pin direction */
void PIT1_IRQHandler(void)
{
    PIT->CHANNEL[1].TFLG = PIT_TFLG_TIF_MASK;
    (void)PIT->CHANNEL[1].TFLG;
    PTA->PDDR ^= MASK(BUZZER_PIN);
}
