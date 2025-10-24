/*----------------------------------------------------------------------------

 Copyright 2013, GHJ Morsink


 Author: MorsinkG

   Purpose:
      Implements log function

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#ifndef LOG_H_
#define LOG_H_

#include <avr/pgmspace.h>

/*--------------------------------------------------
 Send a string
 --------------------------------------------------*/
extern void NA_WriteBuffer( unsigned char *pbOut, const unsigned int size );

/*--------------------------------------------------
 Transfer to stringrepresentation, incl CR
 3/1 bytes  (24/8 bits)
 --------------------------------------------------*/
extern void vSetHex3( unsigned long uHex, unsigned char *acString );
extern void vSetHex1( unsigned char uHex, unsigned char *acString );

/*--------------------------------------------------
 Send Hex value
 --------------------------------------------------*/
//extern void vSendHex( unsigned long uHex );

/*----------------------------------------------------------------------
    For debug messages: send first string, followed by hex representation second string with iLen
    No check is done on the serial-output buffer
----------------------------------------------------------------------*/
extern void vSendHex( const char *szHeader, unsigned char *acData, unsigned int iLen );

/*----------------------------------------------------------------------
      For service messages: send first string
----------------------------------------------------------------------*/
extern void vLogInfo( const char *szHeader );
extern void vLogString( const char *szHeader );

/*--------------------------------------------------
Prints an uint8 variable in base 10.
 --------------------------------------------------*/
extern void print_uint16_base10(uint16_t n);

/*--------------------------------------------------
vSendCR
    send a new line
 --------------------------------------------------*/
extern void vSendCR( void );

/*--------------------------------------------------
 Send a separation
 --------------------------------------------------*/
extern void SendCommaSpace(void);

#endif /* LOG_H_ */

