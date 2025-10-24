/*----------------------------------------------------------------------------

 Copyright 2025, GHJ Morsink


   Purpose:
      Implements waveform generation

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include <stdint.h>
#ifdef _lint
 #ifdef ____ATTR_PURE__
   #undef __ATTR_PURE__
 #endif
 #ifdef __attribute__
   #undef __attribute__
 #endif
 #define __ATTR_PURE__
 #define __attribute__(var)
#endif
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include "waveform.h"
#include "timer.h"
#include "log.h"
#include "board.h"
#include "serial.h"

/***------------------------- Defines -----------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/
/* The data is given as flat types; structures give overhead in the generated code */
static uint8_t    currentState;  /* FSM state */
static uint16_t   currentTime;   /* current time reference */
/***------------------------ Global Data --------------------------------***/

//! Keep these in sequence and together, as they are stored in eeprom
sSetting_t        sSetChannel;

uint8_t EEMEM     NonVolatileSettings[SETTING_SIZE];  /* eeprom copy */
uint16_t EEMEM    uSerialNumber;

/***------------------------ Local functions ----------------------------***/

/***------------------------ Global functions ---------------------------***/
/*----------------------------------------------------------------------
    vInitWaveform
      Initialize this module
----------------------------------------------------------------------*/
void vInitWaveform( void )
{
   uint8_t  size;
   uint8_t  *ptr = &sSetChannel.uStartFlag;      /* initialize at beginning of settings */
   uint8_t  check;

   currentState = 0;
   check = 0xff;

   for ( size = 0; size < SETTING_SIZE; size++ )  /* read all settings from eeprom */
   {
      *ptr = eeprom_read_byte(&NonVolatileSettings[size]);
      check = check & *ptr;
      ptr++;
   }
   if ( check == 0xff )                            /* if nothing in eeprom */
   {
      sSetChannel = (sSetting_t) { 0, 0, { 25, 25 }, { 1000, 50, 50, 50, 1000 } };
   }
}

/*--------------------------------------------------
 Generate waveforms within the RoundRobin system
 --------------------------------------------------*/
void vDoWaveform( void )
{
   uint16_t    temp;

   {
      if ( (sSetChannel.uStartFlag == 0) || (sSetChannel.uStartFlag > 3) )  /* if set to 'off': set the FSM to startpoint */
      {
         currentState = 0;
      }
      vGetSystemTimer(&temp);                      /* get the time */

      switch ( currentState )                      /* run for each channel the FSM */
      {
         case 0 :                                  /* nothing happening, all in zero position */
            if ( sSetChannel.uStartFlag == 1 )
            {
               vLogString(PSTR("START"));
               vSendCR();
               currentState = 1;                   /* go to pre wait for starting pulsing */
               currentTime = temp;                 /* set current time */
            } else
            {
               sSetChannel.uStartFlag = 0;
            }
            break;
         case 1 :                                  /* pre pulsing wait time */
            if ( temp >= currentTime )             /* time isn't rolled-over */
            {
               temp = temp - currentTime;
            } else
            {
               temp = temp  + (UINT16_MAX - currentTime);  /* rolled-over! */
            }
            if ( temp >= sSetChannel.uTimes[0])
            {
               sSetChannel.uStartFlag = 2;         /* indicate it */
               vGetSystemTimer(&currentTime);      /* save current timecount */
               currentState = 2;                   /* going to pulse gen */
            }
            break;
         case 2 :                                  /* uninterupted pos.pulse,interphase,and neg.pulse */
            LED_ON();
            setVoltage(0, sSetChannel.uVoltages[0]);  /* set positive output voltage */
            setPulsePositive();                    /* start the pulse */
            StartADC();                            /* get measurements pos pulse */
            delay_100us(sSetChannel.uTimes[1]);
            clearPulse();                          /* no output */
            StoreADC(0);
            StartADC();                            /* get measurements after pospulse */
            if ( sSetChannel.uTimes[2] > 0)
            {
               delay_100us(sSetChannel.uTimes[2]);
            }
            StoreADC(1);
            setVoltage(0, sSetChannel.uVoltages[1]);  /* set negative output voltage */
            if ( sSetChannel.uTimes[3] > 0)
            {
               setPulseNegative();                 /* start the pulse */
               StartADC();                         /* get measurements neg pulse */
               delay_100us(sSetChannel.uTimes[3]);
               clearPulse();                       /* no output */
               StoreADC(2);
               StartADC();                         /* get measurements after pulse */
            }
            vGetSystemTimer(&currentTime);
            currentState = 3;
            LED_OFF();
            if ( sSetChannel.uTimes[3] > 0)
            {
               StoreADC(3);
            }
            vSendHex(PSTR(""), (uint8_t *) &auAnalogValues[0], 3*4);      /* display measurements */
            vSendCR();
            break;
         case 3 :                                  /* Waiting for next pulse (in between the terminal can work) */
            if ( temp >= currentTime )             /* time isn't rolled-over */
            {
               temp = temp - currentTime;
            } else
            {
               temp = temp  + (UINT16_MAX - currentTime);  /* rolled-over! */
            }
            if ( temp >= sSetChannel.uTimes[4])    /* check period */
            {
               currentState = 2;                   /* going to pulse gen */
               vGetSystemTimer(&currentTime);      /* save current timecount */
            }
            break;
         default:                                  /* shouldn't occur */
            currentState = 0;
            break;
      }
   }
}


/* EOF */
