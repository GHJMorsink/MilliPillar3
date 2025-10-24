/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Main for the stimulator

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/
#include <avr/interrupt.h>
#include <stdint.h>
#include <avr/wdt.h>
#include "board.h"                    /* system parameters for this board */
#include "serial.h"                   /* Serial connection */
#include "terminal.h"                 /* The command terminal */
#include "buttons.h"                  /* checking buttons pressed */
#include "waveform.h"                 /* The pulse generation */
#include "timer.h"

/***------------------------- Defines ------------------------------------***/


/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

/***------------------------ Global Data --------------------------------***/

/***------------------------ Global functions ---------------------------***/
 /*--------------------------------------------------
 Watchdog pre-main disable funtion
  --------------------------------------------------*/
uint8_t mcusr_mirror __attribute__ ((section (".noinit")));

void get_mcusr(void) __attribute__((naked)) __attribute__((section(".init3")));
void get_mcusr(void)
{
   mcusr_mirror = MCUSR;
   MCUSR = 0;
   wdt_disable();
}

/*--------------------------------------------------
The initialization
 --------------------------------------------------*/
static void vSetup(void)
{
   vInitBoard();                        /* for getting correct internal clock */
   vInitTimer();
   vSerialInit();
   vTerminalInit();
   vInitButtons();
   vInitWaveform();

}

/*--------------------------------------------------
The repeated loop
 --------------------------------------------------*/
static void vLoop(void)
{
   vDoTerminal();                    /* terminal functions */
   vDoButtons();     /* Check the button functions */
   vDoWaveform();                    /* waveform generation */
}



/*--------------------------------------------------
The cooperative RoundRobin
 --------------------------------------------------*/
int main(void)
{
   vSetup();

   for (;;)                             /* The cooperative RoundRobin loop */
   {
      vLoop();
   }
   return 0;
}

/* EOF */
