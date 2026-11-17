/*
 * melody.c - Melody player
 *
 * A melody is an array of notes {frequency, duration}. melody_play() starts
 * the first note; melody_tick1ms() (from the 1 ms PIT0 ISR) counts down the
 * duration and moves to the next note, so the main program never waits.
 * The last NOTE_GAP_MS of every note are silent so that repeated notes are
 * heard as separate notes.
 */
#include "board.h"
#include "melody.h"
#include "buzzer.h"

/* Note frequencies (Hz), equal temperament, A4 = 440 Hz */
#define REST    0U
#define N_G4    392U
#define N_C5    523U
#define N_E5    659U
#define N_G5    784U
#define N_C6    1047U
#define N_CLICK 2000U

#define NOTE_GAP_MS 12U

typedef struct {
    uint16_t freq;  /* Hz, 0 = rest */
    uint16_t ms;    /* duration     */
} note_t;

typedef struct {
    const note_t *notes;
    uint8_t       length;
    uint8_t       loop;
} melody_t;

static const note_t M_KEY[]     = { {N_CLICK,40} };
static const note_t M_SUCCESS[] = { {N_C5,100}, {N_E5,100}, {N_G5,100}, {N_C6,150},
                                    {REST,60},  {N_G5,100}, {N_C6,350} };
static const note_t M_ERROR[]   = { {N_G4,220}, {REST,80},  {N_G4,400} };

#define LEN(a) ((uint8_t)(sizeof(a) / sizeof((a)[0])))

/* order must match melody_id_t in melody.h */
static const melody_t MELODIES[MELODY_COUNT] = {
    { M_KEY,     LEN(M_KEY),     0U },
    { M_SUCCESS, LEN(M_SUCCESS), 0U },
    { M_ERROR,   LEN(M_ERROR),   0U },
};

/* Player state, shared between main context and PIT0 ISR */
static const melody_t *volatile s_cur = 0;
static volatile uint8_t  s_index   = 0U;
static volatile uint16_t s_remain  = 0U;
static volatile uint8_t  s_playing = 0U;

static void melody_startNote(void)
{
    const note_t *n = &s_cur->notes[s_index];
    s_remain = n->ms;
    buzzer_tone(n->freq);
}

void melody_play(melody_id_t id)
{
    if (id >= MELODY_COUNT) {
        return;
    }
    NVIC_DisableIRQ(PIT0_IRQn);     /* make the update atomic w.r.t. the tick */
    s_cur     = &MELODIES[id];
    s_index   = 0U;
    s_playing = 1U;
    melody_startNote();
    NVIC_EnableIRQ(PIT0_IRQn);
}

void melody_stop(void)
{
    NVIC_DisableIRQ(PIT0_IRQn);
    s_playing = 0U;
    buzzer_off();
    NVIC_EnableIRQ(PIT0_IRQn);
}

uint8_t melody_isPlaying(void) { return s_playing; }

void melody_tick1ms(void)
{
    if (!s_playing) {
        return;
    }
    if (s_remain > 0U) {
        s_remain--;
    }
    if (s_remain == NOTE_GAP_MS) {
        buzzer_off();                       /* short silence between notes */
    }
    if (s_remain == 0U) {
        s_index++;
        if (s_index >= s_cur->length) {
            if (s_cur->loop) {
                s_index = 0U;
            } else {
                s_playing = 0U;
                buzzer_off();
                return;
            }
        }
        melody_startNote();
    }
}
