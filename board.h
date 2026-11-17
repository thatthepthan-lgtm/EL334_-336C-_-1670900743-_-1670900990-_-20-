/*
 * board.h
 * EL334 Mini-Project : Safe Lock (Electronic Safe)
 *
 * Pin map: FRDM-K64F  <->  Arduino Multi-Function Shield
 *
 *   Shield device        Arduino pin   K64F pin   Active level
 *   -------------------  -----------   --------   ------------
 *   Switch S1            A1            PTB3       LOW  (10k pull-up on shield)
 *   Switch S2            A2            PTB10      LOW
 *   Switch S3            A3            PTB11      LOW
 *   LED D1               D13           PTD1       LOW  (LED anode to +5V)
 *   LED D2               D12           PTD3       LOW
 *   LED D3               D11           PTD2       LOW
 *   LED D4               D10           PTD0       LOW
 *   Buzzer LS1 (via Q1)  D3            PTA1       LOW  (PNP 3906 driver)
 *   74HC595 latch        D4            PTB23      -
 *   74HC595 shift clock  D7            PTC3       -
 *   74HC595 serial data  D8            PTC12      -
 *
 * The on-board RGB LED of the FRDM-K64F is NOT used (project rule).
 */
#ifndef BOARD_H
#define BOARD_H

#include "MK64F12.h"

#define MASK(x)         (1UL << (x))

/* Push buttons (PORTB, interrupt capable) */
#define SW1_PIN         3U
#define SW2_PIN         10U
#define SW3_PIN         11U

/* LEDs (PORTD) */
#define LED1_PIN        1U
#define LED2_PIN        3U
#define LED3_PIN        2U
#define LED4_PIN        0U

/* Buzzer (PORTA) */
#define BUZZER_PIN      1U

/* 4-digit 7-segment display through two 74HC595 */
#define SEG_LATCH_PIN   23U     /* PTB23 */
#define SEG_CLK_PIN     3U      /* PTC3  */
#define SEG_DATA_PIN    12U     /* PTC12 */

/* Interrupt priorities (0 = highest). Tone timing is the most critical. */
#define PRIO_PIT1_TONE  1U
#define PRIO_PIT0_TICK  2U
#define PRIO_PORTB_SW   3U

#endif /* BOARD_H */
