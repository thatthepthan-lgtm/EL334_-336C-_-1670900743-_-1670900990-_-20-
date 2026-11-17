/*
 * melody.h - Non-blocking melody player (timed by the 1 ms PIT0 tick)
 */
#ifndef MELODY_H
#define MELODY_H

#include <stdint.h>

/* The buzzer is used only for these three sounds */
typedef enum {
    MELODY_KEY = 0,     /* short click when a switch is pressed    */
    MELODY_SUCCESS,     /* correct code : rising melody            */
    MELODY_ERROR,       /* wrong code   : two low beeps            */
    MELODY_COUNT
} melody_id_t;

void    melody_play(melody_id_t id);
void    melody_stop(void);
uint8_t melody_isPlaying(void);
void    melody_tick1ms(void);       /* called from PIT0_IRQHandler only */

#endif /* MELODY_H */
