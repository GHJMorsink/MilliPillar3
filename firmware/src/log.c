/*----------------------------------------------------------------------------

 Copyright 2023, GHJ Morsink


   Purpose:
      Implements log function

   Contains:

   Module:

------------------------------------------------------------------------------
*/
#include "serial.h"
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

typedef char PROGMEM prog_char;

#define CHARTABLE    0                  /* use code conversion */
#define MAX_STRING   64                 /* Length of buffer in uart routines */


/*----------------------------------------------------------------------
    vByteToHex

    Set string of two hex-characters from a byte
----------------------------------------------------------------------*/
#if CHARTABLE
static const char *acHex = "0123456789ABCDEF";
#define cHex(c)      ((unsigned char) acHex[c])         /* use charactertable */

#else
static unsigned char cHex( unsigned char cRef )  /* use code */
{
   register unsigned char uRet;

   uRet = cRef + '0';
   if ( uRet > '9' )
   {
      uRet += 7;
   }
   return uRet;
}
#endif


/*****************************************************************************
   Function :  NA_WriteBuffer

   this function will try to write the contents of a buffer to a port

   In :  port_h         Handle to the PORT
         pbOut          pointer to buffer with data to send
         size           size of the input buffer
         pCount         pointer to store the  count of bytes actually sent

   Out : -

   Returns :   NASuccess  buffer sent completely
               NAFailure  error occurred (no port)
*****************************************************************************/
void NA_WriteBuffer( unsigned char *pbOut, const unsigned char size )
{
    unsigned char   uSentCount;

    for ( uSentCount = 0; uSentCount < size; uSentCount++ )
    {

      vSerialPutChar( pbOut[ uSentCount ] );                        /* send 1 character */
    }
    return;
}

/*--------------------------------------------------
 Transfer to stringrepresentation
 3 bytes  (24 bits)
 --------------------------------------------------*/
void vSetHex3( unsigned long uHex, unsigned char *acString )
{
    register unsigned char cData;

    cData = (uHex & 0x00F00000) >> 20;
    acString[0] = cHex(cData);
    cData = (uHex & 0x000F0000) >> 16;
    acString[1] = cHex(cData);
    cData = (uHex & 0xF000) >> 12;
    acString[2] = cHex(cData);
    cData = (uHex & 0x0F00) >> 8;
    acString[3] = cHex(cData);
    cData = (uHex & 0x00F0) >> 4;
    acString[4] = cHex(cData);
    cData = uHex & 0x000F;
    acString[5] = cHex(cData);
    acString[6] = '\r';
}

/*--------------------------------------------------
 Send Hex value
 --------------------------------------------------*/
void vSetHex1( unsigned char uHex, unsigned char *acString  )
{
    register unsigned char cData;

    cData = (uHex & 0xF0) >> 4;
    acString[0] = cHex(cData);
    cData = uHex & 0x0F;
    acString[1] = cHex(cData);
    acString[2] = '\r';
}

/*----------------------------------------------------------------------
    vSendHex

    For hex messages: send first string, followed by hex representation second string with iLen
    No check is done on the serial-output buffer
----------------------------------------------------------------------*/
/*lint -e850 */
void vSendHex( const prog_char *szHeader, unsigned char *acData, unsigned int iLen )
{
    unsigned int  iTotalLen, i;
    unsigned char acTotalString[ MAX_STRING ];
    unsigned char cData;

    iTotalLen = strlen_P( szHeader );
    if ( iTotalLen > (MAX_STRING - 11 ) )
    {
        return ;
    }
    memcpy_P( acTotalString, szHeader, iTotalLen );

    acTotalString[ iTotalLen ] = ':';
    iTotalLen += 1;

    for ( i = 0; i < iLen; i++ )
    {
        acTotalString[ iTotalLen ] = ' ';
        cData = (acData[ i ] & 0x00F0) >> 4;
        acTotalString[ iTotalLen + 1 ] = cHex(cData);
        cData = acData [ i ] & 0x000F;
        acTotalString[ iTotalLen + 2 ] = cHex(cData);
        iTotalLen += 3;
        if ( iTotalLen > ( MAX_STRING - 5 ) )  /* protect against 'out of boundary' */
        {
            i = iLen;
        }
    }
    acTotalString[ iTotalLen ] = '\r';
    iTotalLen += 1;

    NA_WriteBuffer( acTotalString, iTotalLen );
}

/*----------------------------------------------------------------------
    vLogInfo

      For service messages: send first string
----------------------------------------------------------------------*/
void vLogInfo( const prog_char *szHeader )
{
    unsigned int  iTotalLen;
    unsigned char acTotalString[ MAX_STRING ];

    iTotalLen = strlen_P( szHeader );
    if ( iTotalLen > (MAX_STRING - 2) )
    {
        iTotalLen = MAX_STRING - 2;
    }
    memcpy_P( acTotalString, szHeader, iTotalLen );
    acTotalString[ iTotalLen ] = '\r';
    iTotalLen += 1;
    acTotalString[ iTotalLen ] = '\n';
    iTotalLen += 1;
    NA_WriteBuffer( acTotalString, iTotalLen );
}

/*----------------------------------------------------------------------
    vLogString

      For service messages: send first string
----------------------------------------------------------------------*/
void vLogString( const prog_char *szHeader )
{
    unsigned int  iTotalLen;
    unsigned char acTotalString[ MAX_STRING ];

    iTotalLen = strlen_P( szHeader );
    if ( iTotalLen > (MAX_STRING) )
    {
        iTotalLen = MAX_STRING;
    }
    memcpy_P( acTotalString, szHeader, iTotalLen );
    NA_WriteBuffer( acTotalString, iTotalLen );
    vSerialPutChar( ' ' );              /* always add a space */
}

/*--------------------------------------------------
Prints an uint8 variable in base 10.
 --------------------------------------------------*/
void print_uint16_base10(uint16_t n)
{
   uint8_t digits[5];                   /* uint16 has maximal 5 digits */
   uint8_t cnt, zeroflag;

   for ( cnt = 0; cnt < 5; cnt++ )
   {
      digits[cnt] = n % 10;
      n /= 10;
   }
   zeroflag = 0;
   cnt = 5;
   while ( cnt > 0 )
   {
      cnt -=  1;
      if ( (digits[cnt] != 0) || (cnt == 0) )
      {
         zeroflag = 1;                  /* last digit must be shown; all after non-zero digit must be shown */
      }
      if ( zeroflag != 0 )
      {
         /* Show the digit(s) in the correct sequence */
         vSerialPutChar( ('0' + digits[cnt]) );
      }
   }
}

/*--------------------------------------------------
vSendCR
    send a new line
 --------------------------------------------------*/
void vSendCR( void )
{
    vSerialPutChar( '\r' );
    vSerialPutChar( '\n' );
}

/*--------------------------------------------------
 Send a separation
 --------------------------------------------------*/
void SendCommaSpace(void)
{
   vSerialPutChar( ',' );
   vSerialPutChar( 0x20 );
}



/* EOF */
