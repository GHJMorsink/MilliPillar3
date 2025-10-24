/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements waveform

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#ifndef WAVE_H_
#define WAVE_H_

#define CHANNELCOUNT       4            /* how many channels */
#define TIMECOUNT          5            /* all timing elements */

#include <stdint.h>
#include <avr/eeprom.h>

/***------------------------ Global Data --------------------------------***/
typedef struct sSetting_t
{
   uint8_t  uStartFlag;                /* Running flags 0=stopped, 1=starting, 2=pulsing */
   uint8_t  uRecordFlag;               /* Recording flag */
   uint8_t  uVoltages[2];              /* Voltage setting pos/neg (V1 and V2) */
   uint16_t uTimes[TIMECOUNT];         /* Timing: start-pause, pos.pulse T1, interphase T2, neg.pule T3, period T4 */
} sSetting_t;

extern sSetting_t   sSetChannel;
#define  SETTING_SIZE   sizeof(sSetting_t)

extern uint8_t  EEMEM NonVolatileSettings[SETTING_SIZE];
extern uint16_t EEMEM uSerialNumber;

/***------------------------ Global functions ---------------------------***/
/*----------------------------------------------------------------------
    vInitWaveform
      Initialize this module

----------------------------------------------------------------------*/
extern void vInitWaveform( void );

/*--------------------------------------------------
 Generate waveforms within the RoundRobin system
 --------------------------------------------------*/
extern void vDoWaveform( void );


#endif /* WAVE_H_ */

