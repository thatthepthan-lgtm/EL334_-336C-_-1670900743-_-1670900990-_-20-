/*
 * EL334 Mini-Project (1/69) : Safe Lock - Electronic Safe
 * Board  : NXP FRDM-K64F + Arduino Multi-Function Shield
 * Group  : 20   Section : 336C
 * Members: 1670900743 นายรังสิพล ชนประชา, 1670900990 นายทัตเทพ ทนันชัย
 *
 * main() only initialises the drivers and runs the application loop.
 * Every device has its own module:
 *   led.c      LEDs D1..D4                (GPIO)
 *   switch.c   S1..S3                     (GPIO + PORTB interrupt + debounce)
 *   seg7.c     4-digit 7-segment display  (74HC595, multiplexed by PIT0)
 *   buzzer.c   piezo buzzer               (tone by PIT1 interrupt)
 *   melody.c   melodies                   (note timing by PIT0)
 *   timer.c    PIT0 1 ms system tick
 *   safelock.c application state machine
 */
#include "board.h"
#include "led.h"
#include "seg7.h"
#include "switch.h"
#include "timer.h"
#include "buzzer.h"
#include "safelock.h"

int main(void)
{
    /* 1. outputs and inputs first, so the tick ISR finds them ready */
    led_init();
    seg7_init();
    sw_init();

    /* 2. timers: PIT0 tick, then PIT1 tone (needs the PIT clock from timer_init) */
    timer_init();
    buzzer_init();

    /* 3. application */
    safelock_init();

    for (;;) {
        safelock_update();
        __WFI();            /* sleep until the next interrupt (at most 1 ms) */
    }
}
