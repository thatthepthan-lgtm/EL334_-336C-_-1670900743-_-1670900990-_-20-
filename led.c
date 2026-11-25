/*
 * led.c - LED driver (GPIO output on PORTD)
 *
 * Each LED anode is tied to +5V through 510 ohm, the cathode goes to the MCU
 * pin. Writing 0 to the pin turns the LED on, writing 1 turns it off.
 */
#include "board.h"
#include "led.h"

/* MCU pin number for D1..D4, indexed by bit position of the LED mask */
static const uint8_t LED_PIN[4] = { LED1_PIN, LED2_PIN, LED3_PIN, LED4_PIN };

/* Convert a logical LED mask (bit0..bit3 = D1..D4) into a PORTD pin mask */
static uint32_t led_toPortMask(uint8_t leds)
{
    uint32_t m = 0U;
    uint8_t i;
    for (i = 0U; i < 4U; i++) {
        if (leds & (1U << i)) {
            m |= MASK(LED_PIN[i]);
        }
    }
    return m;
}

void led_init(void)
{
    uint8_t i;
    SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;             /* clock gate for PORTD   */
    for (i = 0U; i < 4U; i++) {
        PORTD->PCR[LED_PIN[i]] = PORT_PCR_MUX(1);   /* ALT1 = GPIO            */
    }
    PTD->PSOR  = led_toPortMask(LED_ALL);           /* drive HIGH = all off   */
    PTD->PDDR |= led_toPortMask(LED_ALL);           /* set as outputs         */
}

void led_setMask(uint8_t mask)
{
    uint32_t on  = led_toPortMask(mask & LED_ALL);
    uint32_t off = led_toPortMask((uint8_t)(~mask) & LED_ALL);
    PTD->PCOR = on;     /* active-low : clear = on  */
    PTD->PSOR = off;    /*              set   = off */
}

void led_on(uint8_t leds)     { PTD->PCOR = led_toPortMask(leds); }
void led_off(uint8_t leds)    { PTD->PSOR = led_toPortMask(leds); }
void led_toggle(uint8_t leds) { PTD->PTOR = led_toPortMask(leds); }
