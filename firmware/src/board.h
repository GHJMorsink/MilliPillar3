/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Stimulator

------------------------------------------------------------------------------
*/


#ifndef BOARD_H_
#define BOARD_H_

#include <stdint.h>

/* Leds */
#define ONOFFLED           6           /* PD6 */
#define RECLED             7           /* PD7 */


#define LED_OFF()       PORTD &= ~(1 << ONOFFLED)
#define LED_ON()        PORTD |= (1 << ONOFFLED)
#define RLEDOFF()       PORTD &= ~(1 << RECLED)
#define RLEDON()        PORTD |= (1 << RECLED)

/* Buttons */
#define  RUNBUTTON      4
#define  RECBUTTON      5

#define GETRUNBUTTON()    (PIND & (1 << RUNBUTTON) )
#define GETRECBUTTON()    (PIND & (1 << RECBUTTON) )


extern uint8_t auAnalogValues[3*4];      /* pos.pulse,rest,neg.pulse,rest : each 3 values */

extern void vInitBoard(void);           /* Initialize all board items */

/*--------------------------------------------------
 The controls for the pulses
 --------------------------------------------------*/
extern void setVoltage(uint8_t ch, uint8_t decivolts);
extern void setPulsePositive(void);
extern void setPulseNegative(void);
extern void clearPulse(void);

/* ADC functions */
extern void StartADC(void);
extern void StoreADC(uint8_t uOffset);

#endif /* BOARD_H_ */

