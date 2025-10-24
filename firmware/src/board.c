/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink

   Purpose:


   Contains:

   Module:
      Stimulator

------------------------------------------------------------------------------
*/

/***------------------------- Includes ----------------------------------***/
#include <avr/io.h>
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
#include <avr/interrupt.h>
#include <stdint.h>
#include "board.h"

/***------------------------- Defines ------------------------------------***/

/*--------------------------------------------------
SPI devices defines
 --------------------------------------------------*/

#define MCP4802_CS         3            /* port PC3 selects MCP4802 */
#define COMMAND_DAC0       0x1000         /* command to set DAC0  0b0001xxxx bit13=0 for 2* gain, bit12=1 for active mode operation */
#define COMMAND_DAC1       0x9000         /* command to set DAC1  0b1001xxxx */

#define mcp4802_select()   PORTC &= ~(1 << MCP4802_CS)
#define mcp4802_deselect() PORTC |= (1 << MCP4802_CS)

/* The macro version */
#define vXmtSPI(dat)       {SPDR=(dat); loop_until_bit_is_set(SPSR,SPIF);}

/*--------------------------------------------------
Functions for generating pulses
 --------------------------------------------------*/

#define Pos_Port           0
#define Neg_Port           1

#define PortBInput(pin)    DDRB &= ~(1 << pin)
#define PortBOutput(pin)   DDRB |= (1 << pin)
#define PortBZero(pin)     PORTB &= ~(1 << pin)

#define ADSETTING    0xDF               /* set divider=128, ADIE=1, ADEN=1 ADIF=1, start conversion */
#define NREADY       1                  /* not ready */

/***------------------------- Types -------------------------------------***/

/***----------------------- Local Types ---------------------------------***/

/***------------------------- Local Data --------------------------------***/

static uint8_t  uFlags;

static uint8_t  AMeas0;           /* storage for ADC analog measurements */
static uint8_t  AMeas1;
static uint8_t  AMeas2;


/***------------------------ Global Data --------------------------------***/
uint8_t auAnalogValues[3*4];      /* pos.pulse,rest,neg.pulse,rest : each 3 values */

/***------------------------ Local functions ----------------------------***/

/*--------------------------------------------------
write data to MCP48x2 DAC
 --------------------------------------------------*/
static void vWriteDAC(uint8_t value, uint8_t adc)
{
   uint16_t both;

   value = (value << 2) + value;    /* value*5 (value is 0..50, adc gets 0..250) */
   both = value << 4;               /* arrange it in 16 bits command */
   if(adc == 0)
   {
      both |= COMMAND_DAC0;
   }
   else
   {
      both |= COMMAND_DAC1;
   }
   mcp4802_select();
   vXmtSPI( (uint8_t)(both >> 8) );       /* give command and */
   vXmtSPI( (uint8_t)(both & 0xFF) );     /* data */
   mcp4802_deselect();
}

/***------------------------ Global functions ---------------------------***/
void vInitBoard(void)
{
   cli();                               /* no interrupts anymore */
   /* init ports */
   DDRD  = 0xCC;  //0b11001100;        /* Set the port to correct configuration (pd2..pd7 output) */
   PORTD = 0x08;                       /* None of the H-Bridges enabled */
   DDRB  = 0x2F;  //0b00101111;        /* Set the port to correct configuration (pb0/1=outp, PB2=CS, PB3=MOSI, PB4=MISO, PB5=CLK */
   PORTB = 0x00;  //0b11111100;        /* PB0/1 = 0 */
   DDRC  = 0x28;  //0b00101000;        /* PC3  is CS, PC5 is led */
   PORTC = 0x20;                       /* PC5 '1' is off */
   DIDR0 = 0x07;                       /* disable input buffer on adc channels */
   PRR = 0;
   ADCSRB = 0;
   /* init SPI (datadirection is already set) (must be done after port dir initialization) */
   SPCR = 0x50;                        /* Enable SPI function in master mode 0 (fast interface) */
   SPSR = 0x00;                        /* SPI normal mode */
   PortBZero(Neg_Port);
   PortBZero(Pos_Port);
   PortBOutput(Neg_Port);  /* set both op amps to 0 V */
   PortBOutput(Pos_Port);

   sei();
}

/*--------------------------------------------------
 Set a voltage using the pots as DAC
 --------------------------------------------------*/
void setVoltage(uint8_t ch, uint8_t decivolts)
{
   vWriteDAC(decivolts, ch);
}

/*--------------------------------------------------
Set output
 This will enable the channel in either pos or neg side
 --------------------------------------------------*/
void setPulsePositive(void)
{
   PortBZero(Neg_Port);
   PortBOutput(Neg_Port);
   PortBInput(Pos_Port);   /* enable voltage to the op amp */
}

void setPulseNegative(void)
{
   PortBZero(Pos_Port);
   PortBOutput(Pos_Port);
   PortBInput(Neg_Port);   /* enable voltage to the op amp */
}

/*--------------------------------------------------
 Disable outputs
 --------------------------------------------------*/
void clearPulse(void)
{
   PortBOutput(Neg_Port);  /* set both op amps to 0 V */
   PortBOutput(Pos_Port);
}



/* ADC functions */
/*--------------------------------------------------
Start on first channel
 --------------------------------------------------*/
void StartADC(void)
{
   uFlags = NREADY;
   ADMUX = 0x20;                  /* start measuring voltage/current */
   ADCSRA = ADSETTING;           /* start single measurement 1<<ADPS2 | 1<<ADPS1 | 1<<ADPS0 | 1<<ADEN | 1<<ADSC; ADIE, ADIF */
}

/*--------------------------------------------------
This part waits for ADC completion, and must not be optimized!
 --------------------------------------------------*/
#pragma GCC push_options
#pragma GCC optimize ("O0")             /* no optimization! */
static void waitFlag(void)
{
   while (uFlags)
   {
      __asm("nop");                     /* wait for 'ready' */
   }
}
#pragma GCC pop_options

 /*--------------------------------------------------
  Store to correct position
  --------------------------------------------------*/
void StoreADC(uint8_t uOffset)
{
   uint8_t  i;

   if(uOffset == 0)
   {
      for (i=0; i < 3*4; i++)
      {
         auAnalogValues[i] = 0xFF;  /* set to 'no read' */
      }
   }
   waitFlag();

   i = (uOffset<<1)+uOffset;        /* uOffset*3 */
   auAnalogValues[i++] = AMeas0;
   auAnalogValues[i++] = AMeas1;
   auAnalogValues[i] = AMeas2;
}

/*--------------------------------------------------
 analog input interrupt vector
 --------------------------------------------------*/
#ifdef _lint
void ISR_ADC_vect( void )
#else
ISR( ADC_vect )
#endif
{
   register uint8_t  uChannel = ADMUX & 0x07;

   ADCSRA = 0x90;                       /* clear the interrupt flag, disable interrupt and ADC conversion */
   switch ( uChannel )
   {
      case 0x00 :                       /* current measurement */
         AMeas0 = ADCH;
         ADMUX = 0x21;                  /* next channel */
         ADCSRA = ADSETTING;
         break;
      case 0x01 :                       /* emitting voltage (correction value) */
         AMeas1 = ADCH;
         ADMUX = 0x22;                  /* next channel */
         ADCSRA = ADSETTING;
         break;
      case 0x02 :                       /* tissue voltage */
         AMeas2 = ADCH;

         /*lint -fallthrough */
      default:
         uFlags &= ~NREADY;             /* ready */
         break;
   }
}

/* EOF */
