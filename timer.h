/*
 * timer.h - PIT channel 0 : 1 ms system tick
 */
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

void     timer_init(void);          /* enable PIT module, start channel 0 at 1 kHz */
uint32_t timer_millis(void);        /* milliseconds since timer_init()             */
uint32_t timer_busClockHz(void);    /* PIT input clock (bus clock) in Hz           */

#endif /* TIMER_H */
