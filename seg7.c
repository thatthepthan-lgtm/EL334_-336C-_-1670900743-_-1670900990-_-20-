/*
 * seg7.c - 7-segment display driver
 *
 * Two 74HC595 are cascaded. For every digit we shift 16 bits (MSB first):
 *   1st byte : segment pattern, active-LOW, bit order DP G F E D C B A
 *   2nd byte : digit select     (0xF1, 0xF2, 0xF4, 0xF8 = left .. right)
 * then pulse LATCH so both registers update together.
 *
 * Only one digit is lit at a time. seg7_refresh() is called every 1 ms by
 * the PIT0 ISR and moves to the next digit, so each digit is refreshed every
 * 4 ms (250 Hz) - fast enough that the eye sees all four digits steadily.
 */
#include "board.h"
#include "seg7.h"

/* Segment bits, active-HIGH inside the program (inverted when shifted out) */
#define SA 0x01U
#define SB 0x02U
#define SC 0x04U
#define SD 0x08U
#define SE 0x10U
#define SF 0x20U
#define SG 0x40U
#define SDP 0x80U

static const uint8_t DIGIT_SELECT[4] = { 0xF1U, 0xF2U, 0xF4U, 0xF8U };

static const uint8_t DIGIT_FONT[10] = {
    SA|SB|SC|SD|SE|SF,      /* 0 */
    SB|SC,                  /* 1 */
    SA|SB|SD|SE|SG,         /* 2 */
    SA|SB|SC|SD|SG,         /* 3 */
    SB|SC|SF|SG,            /* 4 */
    SA|SC|SD|SF|SG,         /* 5 */
    SA|SC|SD|SE|SF|SG,      /* 6 */
    SA|SB|SC,               /* 7 */
    SA|SB|SC|SD|SE|SF|SG,   /* 8 */
    SA|SB|SC|SD|SF|SG       /* 9 */
};

static volatile uint8_t s_buffer[4];    /* what is shown, 1 bit = 1 segment on */
static uint8_t s_scanPos = 0U;          /* digit being refreshed (ISR only)    */

static void seg7_shiftByte(uint8_t value)
{
    uint8_t i;
    for (i = 0U; i < 8U; i++) {
        if (value & 0x80U) {
            PTC->PSOR = MASK(SEG_DATA_PIN);
        } else {
            PTC->PCOR = MASK(SEG_DATA_PIN);
        }
        PTC->PSOR = MASK(SEG_CLK_PIN);      /* rising edge shifts the bit in */
        PTC->PCOR = MASK(SEG_CLK_PIN);
        value <<= 1;
    }
}

void seg7_init(void)
{
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTC_MASK;

    PORTB->PCR[SEG_LATCH_PIN] = PORT_PCR_MUX(1);
    PORTC->PCR[SEG_CLK_PIN]   = PORT_PCR_MUX(1);
    PORTC->PCR[SEG_DATA_PIN]  = PORT_PCR_MUX(1);

    PTB->PCOR  = MASK(SEG_LATCH_PIN);
    PTC->PCOR  = MASK(SEG_CLK_PIN) | MASK(SEG_DATA_PIN);
    PTB->PDDR |= MASK(SEG_LATCH_PIN);
    PTC->PDDR |= MASK(SEG_CLK_PIN) | MASK(SEG_DATA_PIN);

    seg7_clear();
}

void seg7_refresh(void)
{
    PTB->PCOR = MASK(SEG_LATCH_PIN);                    /* hold outputs      */
    seg7_shiftByte((uint8_t)~s_buffer[s_scanPos]);      /* segments, act-low */
    seg7_shiftByte(DIGIT_SELECT[s_scanPos]);            /* which digit       */
    PTB->PSOR = MASK(SEG_LATCH_PIN);                    /* update outputs    */
    s_scanPos = (uint8_t)((s_scanPos + 1U) & 0x03U);
}

uint8_t seg7_encode(char c)
{
    if (c >= '0' && c <= '9') {
        return DIGIT_FONT[c - '0'];
    }
    switch (c) {
    case 'A': case 'a': return SA|SB|SC|SE|SF|SG;
    case 'b': case 'B': return SC|SD|SE|SF|SG;
    case 'C':           return SA|SD|SE|SF;
    case 'c':           return SD|SE|SG;
    case 'd': case 'D': return SB|SC|SD|SE|SG;
    case 'E': case 'e': return SA|SD|SE|SF|SG;
    case 'F': case 'f': return SA|SE|SF|SG;
    case 'H': case 'h': return SB|SC|SE|SF|SG;
    case 'L': case 'l': return SD|SE|SF;
    case 'n': case 'N': return SC|SE|SG;
    case 'O':           return SA|SB|SC|SD|SE|SF;
    case 'o':           return SC|SD|SE|SG;
    case 'P': case 'p': return SA|SB|SE|SF|SG;
    case 'r': case 'R': return SE|SG;
    case 'S': case 's': return SA|SC|SD|SF|SG;
    case 't': case 'T': return SD|SE|SF|SG;
    case 'U':           return SB|SC|SD|SE|SF;
    case '-':           return SG;
    case '_':           return SD;
    case '.':           return SDP;
    default:            return 0U;          /* blank / unknown */
    }
}

void seg7_setChar(uint8_t pos, char c)
{
    if (pos < 4U) {
        s_buffer[pos] = seg7_encode(c);
    }
}

void seg7_showText(const char *text)
{
    uint8_t i;
    for (i = 0U; i < 4U; i++) {
        char c = ' ';
        if (text != 0 && *text != '\0') {
            c = *text++;
        }
        s_buffer[i] = seg7_encode(c);
    }
}

void seg7_clear(void)
{
    uint8_t i;
    for (i = 0U; i < 4U; i++) {
        s_buffer[i] = 0U;
    }
}
