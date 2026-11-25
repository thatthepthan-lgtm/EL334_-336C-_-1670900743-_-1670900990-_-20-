/*
 * safelock.h - Application layer: Safe Lock state machine
 */
#ifndef SAFELOCK_H
#define SAFELOCK_H

#include <stdint.h>

#define SL_CODE_LEN         4U      /* digits in the secret code               */
#define SL_MAX_ATTEMPTS     3U      /* wrong tries before lock-out             */
#define SL_LOCKOUT_BASE_S   30U     /* first lock-out time (seconds)           */
#define SL_LOCKOUT_MAX_S    240U    /* lock-out doubles each time up to this   */
#define SL_FLASH_S          10U     /* LEDs flash fast for the first N seconds */
#define SL_AUTO_RELOCK_S    30U     /* open door relocks after N s idle        */
#define SL_ENTRY_TIMEOUT_S  15U     /* half-typed code is cleared after N s    */
#define SL_MSG_MS           1500U   /* time a message ("Err1", "donE") stays   */

typedef enum {
    SL_BOOT = 0,        /* "SAFE" splash                       */
    SL_LOCKED,          /* waiting for the code                */
    SL_WRONG,           /* "Err n" message                     */
    SL_OPEN,            /* door unlocked                       */
    SL_SET_MSG,         /* "SEt" message before new code       */
    SL_SET_CODE,        /* entering a new code                 */
    SL_SAVED,           /* "donE" message                      */
    SL_LOCKOUT          /* 3 wrong codes: countdown, keys off  */
} sl_state_t;

void       safelock_init(void);
void       safelock_update(void);   /* call continuously from main loop */

/* read-only access for testing / debugging */
sl_state_t safelock_state(void);
uint8_t    safelock_wrongCount(void);
uint16_t   safelock_lockoutRemaining(void);

#endif /* SAFELOCK_H */
