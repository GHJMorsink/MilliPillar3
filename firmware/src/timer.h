/*----------------------------------------------------------------------------

 Copyright 2015, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements general timer functions

   Contains:

   Module:
------------------------------------------------------------------------------
*/

#ifndef TIMER_H_
#define TIMER_H_


/***------------------------- Includes ----------------------------------***/
#include <stdint.h>

/***------------------------- Types -------------------------------------***/


/***------------------------ Global Data --------------------------------***/

/*  The value increments every 2/5/10ms */
extern uint16_t     uSystemTimerCounter;              /* counting */

/***------------------------ Global functions ---------------------------***/

/*--------------------------------------------------
 Initialize hardware
 --------------------------------------------------*/
extern void vInitTimer( void );

/*--------------------------------------------------
 Deliver a mutexed copy of the system timer
 --------------------------------------------------*/
extern void vGetSystemTimer( uint16_t *puTimer );

extern void delay_100us( uint16_t count);

#endif /* TIMER_H_ */
