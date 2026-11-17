/*
 * switch.h - Push buttons S1..S3, PORTB falling-edge interrupt + debounce
 */
#ifndef SWITCH_H
#define SWITCH_H

#include <stdint.h>

typedef enum {
    SW_NONE = 0,
    SW_S1   = 1,
    SW_S2   = 2,
    SW_S3   = 3
} sw_event_t;

void       sw_init(void);
sw_event_t sw_getEvent(void);   /* oldest pending press, SW_NONE if none   */
void       sw_flush(void);      /* discard pending presses                 */
void       sw_tick1ms(void);    /* called from PIT0_IRQHandler only        */

#endif /* SWITCH_H */
