/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements debouncing and actions on buttons

   Contains:

   Module:

------------------------------------------------------------------------------
*/
/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
#ifndef _lint
#include <avr/interrupt.h>
#endif
#include "board.h"
#include "buttons.h"
#include "terminal.h"                   /* for the actions */
#include "timer.h"                      /* for time-out counter */

/***------------------------- Defines -----------------------------------***/

#define DEBOUNCETIME      10           /* wait time in 10ms units for de-bouncing */



/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static uint8_t   uPrevRun;               /* data of previous button state */
static uint8_t   uPrevRec;
static uint16_t  prevTime;   /* current time reference */
static uint8_t   uRunB;
static uint8_t   uRecB;

/***------------------------ Global Data --------------------------------***/

/***------------------------ Local functions ----------------------------***/

/***------------------------ Global functions ---------------------------***/

/*--------------------------------------------------

 --------------------------------------------------*/
void vInitButtons( void )
{
   uPrevRun = 0;
   uPrevRec = 0;
   vGetSystemTimer( &prevTime );
   uRunB = GETRUNBUTTON();
   uRecB = GETRECBUTTON();
}

/*--------------------------------------------------
button testing
 --------------------------------------------------*/
void vDoButtons( void )
{
   uint16_t    temp;
   uint8_t     currButton;


   vGetSystemTimer(&temp);             /* get the time */
   if ( temp >= prevTime )             /* time isn't rolled-over */
   {
      temp = temp - prevTime;
   } else
   {
      temp = temp  + (UINT16_MAX - prevTime);  /* rolled-over! */
   }
   if ( temp >= DEBOUNCETIME )
   {
      vGetSystemTimer( &prevTime );     /* set for next loops */

      /* --- run button --- */
      currButton = GETRUNBUTTON();
      if ( currButton == uPrevRun )
      {
         if ( currButton != uRunB )     /* change in state? */
         {
            uRunB = currButton;
            if ( currButton == 0 )      /* pressed state? */
            {
               vChangeRunState();
            }
         }
      }
      uPrevRun = currButton;

      /* --- record button --- */
      currButton = GETRECBUTTON();
      if ( currButton == uPrevRec )
      {
         if ( currButton != uRecB )     /* change in state? */
         {
            uRecB = currButton;
            if ( currButton == 0 )      /* pressed state? */
            {
               vChangeRecState();
            }
         }
      }
      uPrevRec = currButton;
   }
}


/* EOF */
