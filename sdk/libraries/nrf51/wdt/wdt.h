#ifndef WDT_H
#define WDT_H

#include <stdint.h>
/**
 * Function for initializing watch dog timer
 *
 * @param        timeout   Timeout of watch dog timer, ms
 */
void wdt_init(uint32_t timeout);

/**
 * Function for starting watch dog timer
 */
void wdt_start(void);

/**
 * Function for feeding watch dog timer
 */
void wdt_feed(void);

/**
 * Function for initializing watch dog feeding timer
 */
void wdt_timer_init(void);

/**
 * Function for starting feeding timer
 *
 * @param  interval   Timer interval in ms
 * @param  prescaler  Timer prescaler
 */
void wdt_timer_start(uint32_t interval, uint32_t prescaler);

/**
 * Function for stop feeding timer
 */
void wdt_timer_stop(void);

#endif
