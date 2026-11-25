/*
 * led.h - Four LEDs D1..D4 on the Multi-Function Shield (active-low)
 */
#ifndef LED_H
#define LED_H

#include <stdint.h>

#define LED_D1  0x01U   /* bit0 : D1 (PTD1) */
#define LED_D2  0x02U   /* bit1 : D2 (PTD3) */
#define LED_D3  0x04U   /* bit2 : D3 (PTD2) */
#define LED_D4  0x08U   /* bit3 : D4 (PTD0) */
#define LED_ALL 0x0FU

void led_init(void);
void led_setMask(uint8_t mask);         /* bit = 1 -> LED on, all 4 LEDs at once */
void led_on(uint8_t leds);
void led_off(uint8_t leds);
void led_toggle(uint8_t leds);

#endif /* LED_H */
