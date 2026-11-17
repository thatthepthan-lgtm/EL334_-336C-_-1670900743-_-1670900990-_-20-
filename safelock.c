/*
 * safelock.c - Safe Lock application (state machine)
 *
 * Keys (in LOCKED and SET_CODE):
 *   S1 : dial the current digit up   0 -> 1 -> ... -> 9 -> 0
 *   S2 : accept the digit and move to the next position
 *        (after the 4th digit the code is checked / saved)
 *   S3 : backspace (go back one position); in SET_CODE at the first
 *        position with nothing typed it cancels
 * Keys (in OPEN):
 *   S2 : change the code      S3 : lock now      any key : restart relock timer
 *
 * Display while typing:  typed digits are hidden as '-', the digit being
 * dialed blinks, unused positions show '_'.     e.g.  "- - 7 _"
 *
 * The buzzer sounds ONLY for: a key press (short click), a correct code
 * (success melody) and a wrong code (error beeps).
 *
 * Three wrong codes in a row -> LOCKOUT: error beeps, LEDs flash, keys ignored,
 * the display counts down "L 30" ... "L  1". Each further lock-out doubles
 * the time (30, 60, 120, 240 s). A correct code resets both counters.
 *
 * This module uses only the driver APIs (no registers), so the same file can
 * be compiled and tested on a PC (see tests/).
 */
#include "safelock.h"
#include "switch.h"
#include "led.h"
#include "seg7.h"
#include "melody.h"
#include "timer.h"

#define BLINK_MS    250U

static uint8_t    s_secret[SL_CODE_LEN] = { 1U, 2U, 3U, 4U };   /* factory code */
static uint8_t    s_entry[SL_CODE_LEN];
static uint8_t    s_pos;            /* digits already accepted         */
static uint8_t    s_dial;           /* value of the digit being dialed */

static sl_state_t s_state;
static uint32_t   s_stateStart;     /* ms when the state was entered   */
static uint32_t   s_lastKey;        /* ms of the last key press        */
static uint32_t   s_secondMark;     /* ms of the last 1-second step    */

static uint8_t    s_wrong;          /* wrong codes since last success  */
static uint8_t    s_lockouts;       /* lock-outs since last success    */
static uint16_t   s_lockRemain;     /* seconds left in lock-out        */

/* ---------------------------------------------------------------- helpers */

static uint32_t elapsed(uint32_t since) { return timer_millis() - since; }

static void entry_reset(void)
{
    s_pos  = 0U;
    s_dial = 0U;
}

static uint8_t code_matches(void)
{
    uint8_t i, diff = 0U;
    for (i = 0U; i < SL_CODE_LEN; i++) {
        diff |= (uint8_t)(s_entry[i] ^ s_secret[i]);    /* no early exit */
    }
    return (uint8_t)(diff == 0U);
}

static uint16_t lockout_seconds(uint8_t lockouts)
{
    uint32_t t = SL_LOCKOUT_BASE_S;
    while (lockouts > 1U && t < SL_LOCKOUT_MAX_S) {
        t <<= 1;
        lockouts--;
    }
    return (uint16_t)((t > SL_LOCKOUT_MAX_S) ? SL_LOCKOUT_MAX_S : t);
}

static void enter_state(sl_state_t st)
{
    s_state      = st;
    s_stateStart = timer_millis();
    s_secondMark = s_stateStart;
    s_lastKey    = s_stateStart;

    switch (st) {
    case SL_BOOT:     break;                        /* silent start */
    case SL_LOCKED:   entry_reset();                break;
    case SL_WRONG:    melody_play(MELODY_ERROR);    break;
    case SL_OPEN:     break;
    case SL_SET_MSG:  melody_play(MELODY_KEY);      break;
    case SL_SET_CODE: entry_reset();                break;
    case SL_SAVED:    melody_play(MELODY_KEY);      break;  /* 4th S2 press */
    case SL_LOCKOUT:
        s_lockouts++;
        s_lockRemain = lockout_seconds(s_lockouts);
        melody_play(MELODY_ERROR);      /* 3rd wrong code */
        sw_flush();
        break;
    default:          break;
    }
}

/* -------------------------------------------------------- key handling */

/* Shared digit-entry logic for LOCKED and SET_CODE.
 * Returns 1 when the 4th digit has been accepted.                         */
static uint8_t entry_key(sw_event_t key)
{
    switch (key) {
    case SW_S1:
        s_dial = (uint8_t)((s_dial + 1U) % 10U);
        melody_play(MELODY_KEY);
        break;
    case SW_S2:
        s_entry[s_pos++] = s_dial;
        s_dial = 0U;
        if (s_pos >= SL_CODE_LEN) {
            return 1U;
        }
        melody_play(MELODY_KEY);
        break;
    case SW_S3:
        if (s_pos > 0U) {
            s_pos--;
        }
        s_dial = 0U;
        melody_play(MELODY_KEY);
        break;
    default:
        break;
    }
    return 0U;
}

static void check_code(void)
{
    if (code_matches()) {
        s_wrong    = 0U;
        s_lockouts = 0U;
        melody_play(MELODY_SUCCESS);
        enter_state(SL_OPEN);
    } else {
        s_wrong++;
        if (s_wrong >= SL_MAX_ATTEMPTS) {
            enter_state(SL_LOCKOUT);
        } else {
            enter_state(SL_WRONG);
        }
    }
}

static void handle_key(sw_event_t key)
{
    s_lastKey = timer_millis();

    switch (s_state) {
    case SL_LOCKED:
        if (entry_key(key)) {
            check_code();
        }
        break;

    case SL_OPEN:
        if (key == SW_S2) {
            enter_state(SL_SET_MSG);
        } else if (key == SW_S3) {
            melody_play(MELODY_KEY);
            enter_state(SL_LOCKED);
        } else {
            melody_play(MELODY_KEY);        /* S1 : just keeps the door open */
        }
        break;

    case SL_SET_CODE:
        if (key == SW_S3 && s_pos == 0U && s_dial == 0U) {
            melody_play(MELODY_KEY);        /* cancel, keep the old code */
            enter_state(SL_OPEN);
        } else if (entry_key(key)) {
            uint8_t i;
            for (i = 0U; i < SL_CODE_LEN; i++) {
                s_secret[i] = s_entry[i];
            }
            enter_state(SL_SAVED);
        }
        break;

    default:
        /* BOOT, WRONG, SET_MSG, SAVED, LOCKOUT : keys are ignored */
        break;
    }
}

/* ---------------------------------------------------- time-based events */

static void handle_time(void)
{
    switch (s_state) {
    case SL_BOOT:
        if (elapsed(s_stateStart) >= SL_MSG_MS) {
            enter_state(SL_LOCKED);
        }
        break;

    case SL_LOCKED:
        if ((s_pos > 0U || s_dial > 0U) &&
            elapsed(s_lastKey) >= SL_ENTRY_TIMEOUT_S * 1000U) {
            entry_reset();                  /* forgotten half-typed code */
        }
        break;

    case SL_WRONG:
        if (elapsed(s_stateStart) >= SL_MSG_MS) {
            enter_state(SL_LOCKED);
        }
        break;

    case SL_OPEN:
        if (elapsed(s_lastKey) >= SL_AUTO_RELOCK_S * 1000U) {
            enter_state(SL_LOCKED);         /* silent auto relock */
        }
        break;

    case SL_SET_MSG:
        if (elapsed(s_stateStart) >= SL_MSG_MS) {
            enter_state(SL_SET_CODE);
        }
        break;

    case SL_SET_CODE:
        if (elapsed(s_lastKey) >= SL_AUTO_RELOCK_S * 1000U) {
            enter_state(SL_OPEN);           /* gave up, old code kept */
        }
        break;

    case SL_SAVED:
        if (elapsed(s_stateStart) >= SL_MSG_MS) {
            enter_state(SL_OPEN);
        }
        break;

    case SL_LOCKOUT:
        while (s_lockRemain > 0U && elapsed(s_secondMark) >= 1000U) {
            s_secondMark += 1000U;
            s_lockRemain--;
        }
        if (s_lockRemain == 0U) {
            s_wrong = 0U;
            sw_flush();                     /* drop keys pressed while locked */
            enter_state(SL_LOCKED);
        }
        break;

    default:
        break;
    }
}

/* -------------------------------------------------------------- outputs */

static void show_entry(uint8_t maskTyped)
{
    char text[5];
    uint8_t i;
    uint8_t blinkOn = (uint8_t)(((timer_millis() / BLINK_MS) & 1U) == 0U);

    for (i = 0U; i < SL_CODE_LEN; i++) {
        if (i < s_pos) {
            text[i] = maskTyped ? '-' : (char)('0' + s_entry[i]);
        } else if (i == s_pos) {
            text[i] = blinkOn ? (char)('0' + s_dial) : ' ';
        } else {
            text[i] = '_';
        }
    }
    text[4] = '\0';
    seg7_showText(text);
}

static void update_display(void)
{
    char text[5];

    switch (s_state) {
    case SL_BOOT:     seg7_showText("SAFE"); break;
    case SL_LOCKED:   show_entry(1U);        break;
    case SL_WRONG:
        text[0] = 'E'; text[1] = 'r'; text[2] = 'r';
        text[3] = (char)('0' + s_wrong); text[4] = '\0';
        seg7_showText(text);
        break;
    case SL_OPEN:     seg7_showText("OPEn"); break;
    case SL_SET_MSG:  seg7_showText("SEt "); break;
    case SL_SET_CODE: show_entry(0U);        break;
    case SL_SAVED:    seg7_showText("donE"); break;
    case SL_LOCKOUT:
        text[0] = 'L';
        text[1] = (s_lockRemain >= 100U) ? (char)('0' + (s_lockRemain / 100U)) : ' ';
        text[2] = (s_lockRemain >= 10U)  ? (char)('0' + (s_lockRemain / 10U) % 10U) : ' ';
        text[3] = (char)('0' + (s_lockRemain % 10U));
        text[4] = '\0';
        seg7_showText(text);
        break;
    default:          seg7_clear();          break;
    }
}

static void update_leds(void)
{
    uint8_t mask = 0U;
    uint32_t now = timer_millis();

    switch (s_state) {
    case SL_LOCKED:
    case SL_WRONG:
        mask = LED_D1;                              /* D1 : locked           */
        if (s_wrong >= 1U) { mask |= LED_D2; }      /* D2, D3 : wrong count  */
        if (s_wrong >= 2U) { mask |= LED_D3; }
        break;
    case SL_OPEN:
    case SL_SET_MSG:
    case SL_SET_CODE:
    case SL_SAVED:
        mask = LED_D4;                              /* D4 : open             */
        if (s_state == SL_OPEN &&
            elapsed(s_lastKey) >= (SL_AUTO_RELOCK_S - 5U) * 1000U &&
            ((now / BLINK_MS) & 1U)) {
            mask = 0U;                              /* blink: relock soon    */
        }
        break;
    case SL_LOCKOUT:
        if (elapsed(s_stateStart) < SL_FLASH_S * 1000U) {
            mask = ((now / 125U) & 1U) ? (LED_D1 | LED_D3) : (LED_D2 | LED_D4);
        } else {
            mask = ((now / 500U) & 1U) ? LED_D1 : 0U;
        }
        break;
    default:
        mask = 0U;
        break;
    }
    led_setMask(mask);
}

/* ----------------------------------------------------------- public API */

void safelock_init(void)
{
    s_wrong    = 0U;
    s_lockouts = 0U;
    entry_reset();
    enter_state(SL_BOOT);
    update_display();
    update_leds();
}

void safelock_update(void)
{
    sw_event_t key;
    while ((key = sw_getEvent()) != SW_NONE) {
        handle_key(key);
    }
    handle_time();
    update_display();
    update_leds();
}

sl_state_t safelock_state(void)             { return s_state; }
uint8_t    safelock_wrongCount(void)        { return s_wrong; }
uint16_t   safelock_lockoutRemaining(void)  { return s_lockRemain; }
