/*
 * seg7.h - 4-digit 7-segment display (2 x 74HC595), multiplexed by PIT0
 */
#ifndef SEG7_H
#define SEG7_H

#include <stdint.h>

void    seg7_init(void);
void    seg7_refresh(void);                 /* called every 1 ms from PIT0 ISR      */
uint8_t seg7_encode(char c);                /* character -> segment pattern (1 = on) */
void    seg7_setChar(uint8_t pos, char c);  /* pos 0 = leftmost digit                */
void    seg7_showText(const char *text);    /* up to 4 characters                    */
void    seg7_clear(void);

#endif /* SEG7_H */
