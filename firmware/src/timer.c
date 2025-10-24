/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:
      Implements general timer functions

   Contains:

   Module:

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/

#include <avr/io.h>
#include <avr/interrupt.h>

#include "timer.h"

/***------------------------- Defines ------------------------------------***/

//#define SIXTEEN_MS  187
#define TEN_MS      117
#define FIVE_MS     58
#define TWO_MS      30
#define ONE_MS      250          /* with smaller prescaler! (div 64) */

#define T1TIME_100US    25       /* for 16MHz clock */

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

uint16_t    uSystemTimerCounter;

/***------------------------ Global Data --------------------------------***/

/***------------------------ Global functions ---------------------------***/
/*--------------------------------------------------
 Initialize hardware (interrupts are disabled)
 --------------------------------------------------*/
void vInitTimer( void )
{
   TCCR0A = 0;                          /* compare COM0A and COM0B disconnected; WGM normal mode */
   TCCR0B  = 3; //0x04;                 /* Prescaler div 64; normal mode */
   TCNT0  = 256 - ONE_MS;               /* count starting at -58: every 5ms timer-overflow 117: 10ms*/
   TIMSK0 |= (1 << TOIE0);              /* At overflow enable interrupt */
   TIFR0 |=  (1 << TOV0);               /* clear TOV0 */

   uSystemTimerCounter = 0;

   /* timer1 is used as a 100us delay timer */
   /* setup: normal mode WGM13:0 = 0, TCNT1 determines time until TOV1 will be 1 (overflow) */
   TCCR1A = 0;
   TCCR1B = 3; /* clock is crystal/64 --> 1us on 8MHz system, 0.5us on 16Mhz system */
   TCNT1 = 65536 - T1TIME_100US;
   TIFR1 |= (1 << TOV1);   /* clear TOV1 by writing a 1 */

}


/***------------------------ Interrupt functions ------------------------***/
/*--------------------------------------------------
 System clock
 Runs at 5/10/16 ms per tick (32 ticks per second)
 or 1ms
 --------------------------------------------------*/
#ifdef _lint
void TIMER0_OVF_vect( void )
#else
ISR(TIMER0_OVF_vect)
#endif
{
   TCNT0  = 256 - ONE_MS;   /* count starting at -58: every 1/5/10/16ms timer-overflow */
   uSystemTimerCounter += 1;
}

/*--------------------------------------------------
 Deliver a mutexed copy of the system timer
 --------------------------------------------------*/
void vGetSystemTimer( uint16_t *puTimer )
{
   cli();
   *puTimer = uSystemTimerCounter;
   sei();
}

/*--------------------------------------------------
Delay function for 100 us:  waits count times 100us
 --------------------------------------------------*/
#pragma GCC push_options
#pragma GCC optimize ("O0")             /* no optimization! */
void delay_100us( uint16_t count )
{
   while (count > 0)
   {
      TCNT1 = 65536 - T1TIME_100US;
      TIFR1 |= (1 << TOV1);                /* clear TOV1 by writing 1 */
      loop_until_bit_is_set(TIFR1, TOV1);  /* wait until the flag is set */
      count--;                             /* next */
   }
}
#pragma GCC pop_options

/* EOF */
