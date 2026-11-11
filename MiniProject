/*
 * switch.c - Switch driver with interrupt and debounce
 *
 * S1..S3 are pulled up by 10k on the shield; pressing a key pulls the pin LOW,
 * so a press starts with a FALLING edge. Mechanical contacts bounce for a few
 * milliseconds, which would create many edges for one press.
 *
 * Debounce state machine per key (runs on the 1 ms PIT0 tick):
 *
 *   SW_IDLE         falling-edge interrupt armed (IRQC = 1010b)
 *        | PORTB ISR: disarm IRQ, cnt = 20 ms
 *        v
 *   SW_PRESS_WAIT   wait 20 ms, then read the pin
 *        | still LOW  -> report one press, go to SW_RELEASE_WAIT
 *        | HIGH       -> it was noise, re-arm, back to SW_IDLE
 *        v
 *   SW_RELEASE_WAIT pin must stay HIGH for 20 ms continuously
 *        | then clear ISF, re-arm IRQ, back to SW_IDLE
 *
 * The interrupt is disabled while a key is being debounced, so bounces on
 * press and on release cannot produce extra events.
 */
#include "board.h"
#include "switch.h"

#define SW_COUNT        3U
#define DEBOUNCE_MS     20U
#define IRQC_FALLING    0x0AU       /* PORTx_PCR IRQC: interrupt on falling edge */
#define IRQC_DISABLED   0x00U

typedef enum { SW_IDLE = 0, SW_PRESS_WAIT, SW_RELEASE_WAIT } sw_state_t;

static const uint8_t SW_PIN[SW_COUNT] = { SW1_PIN, SW2_PIN, SW3_PIN };

static volatile uint8_t s_state[SW_COUNT];
static volatile uint8_t s_count[SW_COUNT];

/* Small FIFO of key events: written by PIT0 ISR, read by main */
#define SW_QUEUE_SIZE 8U
static volatile uint8_t s_queue[SW_QUEUE_SIZE];
static volatile uint8_t s_head = 0U;    /* next write */
static volatile uint8_t s_tail = 0U;    /* next read  */

static uint8_t sw_isPressed(uint8_t i)
{
    return (uint8_t)((PTB->PDIR & MASK(SW_PIN[i])) == 0U);
}

static void sw_setIrqc(uint8_t i, uint32_t irqc)
{
    /* Write ISF = 1 to clear any stale flag, keep MUX, replace IRQC */
    uint32_t pcr = PORTB->PCR[SW_PIN[i]];
    pcr &= ~(PORT_PCR_IRQC_MASK);
    pcr |= PORT_PCR_IRQC(irqc) | PORT_PCR_ISF_MASK;
    PORTB->PCR[SW_PIN[i]] = pcr;
}

static void sw_push(uint8_t event)
{
    uint8_t next = (uint8_t)((s_head + 1U) % SW_QUEUE_SIZE);
    if (next != s_tail) {           /* drop the event if the queue is full */
        s_queue[s_head] = event;
        s_head = next;
    }
}

void sw_init(void)
{
    uint8_t i;
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    for (i = 0U; i < SW_COUNT; i++) {
        PORTB->PCR[SW_PIN[i]] = PORT_PCR_MUX(1) | PORT_PCR_ISF_MASK;  /* GPIO */
        PTB->PDDR &= ~MASK(SW_PIN[i]);                              /* input */
        s_state[i] = SW_IDLE;
        s_count[i] = 0U;
        sw_setIrqc(i, IRQC_FALLING);
    }
    NVIC_SetPriority(PORTB_IRQn, PRIO_PORTB_SW);
    NVIC_ClearPendingIRQ(PORTB_IRQn);
    NVIC_EnableIRQ(PORTB_IRQn);
}

/* PORTB interrupt: first falling edge of a key press */
void PORTB_IRQHandler(void)
{
    uint32_t flags = PORTB->ISFR;
    uint8_t i;

    PORTB->ISFR = flags;                        /* clear all flags we saw (w1c) */
    for (i = 0U; i < SW_COUNT; i++) {
        if ((flags & MASK(SW_PIN[i])) && (s_state[i] == SW_IDLE)) {
            sw_setIrqc(i, IRQC_DISABLED);       /* ignore the bounces           */
            s_count[i] = DEBOUNCE_MS;
            s_state[i] = SW_PRESS_WAIT;
        }
    }
}

void sw_tick1ms(void)
{
    uint8_t i;
    for (i = 0U; i < SW_COUNT; i++) {
        switch (s_state[i]) {
        case SW_PRESS_WAIT:
            if (--s_count[i] == 0U) {
                if (sw_isPressed(i)) {
                    sw_push((uint8_t)(SW_S1 + i));   /* valid press */
                    s_count[i] = DEBOUNCE_MS;
                    s_state[i] = SW_RELEASE_WAIT;
                } else {
                    s_state[i] = SW_IDLE;            /* glitch      */
                    sw_setIrqc(i, IRQC_FALLING);
                }
            }
            break;

        case SW_RELEASE_WAIT:
            if (sw_isPressed(i)) {
                s_count[i] = DEBOUNCE_MS;            /* still held / bouncing */
            } else if (--s_count[i] == 0U) {
                s_state[i] = SW_IDLE;
                sw_setIrqc(i, IRQC_FALLING);         /* ready for next press  */
            }
            break;

        default:
            break;
        }
    }
}

sw_event_t sw_getEvent(void)
{
    sw_event_t ev = SW_NONE;
    __disable_irq();
    if (s_tail != s_head) {
        ev = (sw_event_t)s_queue[s_tail];
        s_tail = (uint8_t)((s_tail + 1U) % SW_QUEUE_SIZE);
    }
    __enable_irq();
    return ev;
}

void sw_flush(void)
{
    __disable_irq();
    s_tail = s_head;
    __enable_irq();
}
