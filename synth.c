#define SYNTH_C   

#include "g_types.h"
#include "g_proto.h"
#include "g_cdpd_const.h"


/*************************************************************************
 * PORTABLE DEFINITIONS FILE
 *
 * This file defines the constants and types that will be used throughout
 * the CDPD project.
 *
 * Several of the definintions here conflict with or are redundant with
 * the Airlink file TYPEDEFS.H.  Conditional inclusion has been added
 * here to remove the conflict.  Because of these conditionals, if
 * TYPEDEFS.H must be included in a file with this file, TYPEDEFS.H
 * should be included first.
 *
 * $Log:   P:/cdpd/vcs/include/portdefs.h_v  $
 * 
 *    Rev 1.6   Apr 11 1996 10:10:40   shak_c
 * Added Boolean type to support Airlink code.
 *
 *    Rev 1.5   Feb 26 1996 14:18:18   gearyc
 * Added a define
 *
 * Modified December 27, 1995 by G. Chopoff (GIC) added UWORD
 *
 *    Rev 1.3   Dec 13 1995 11:32:12   shak_c
 * Fixed up header style
 *
 *    Rev 1.2   Dec 01 1995 18:52:54   gearyc
 * CDPD 1.1 RF works
 * Modified November 27, 1995 by G. Chopoff Added uchar define
 *
 *    Rev 1.1   Nov 20 1995 17:41:26   shak_c
 * Added conditional compilation of certain types to avoid
 * conflicts with Airlink defined types
 *
 *    Rev 1.0   23 Jan 1995 15:00:36   shak_c
 * Initial checkin
 *
 * REVISION HISTORY
 * DATE		BY	DESCRIPTION
 * 3Nov94	CS	Creation *
 *************************************************************************/

/* The following is a check to prevent conflicts with the Airlink
 * file, TYPEDEFS.H.  Eventually, this file should be merged with
 * TYPEDEFS.H.
 */

#ifndef _typedefs_h
#define _typedefs_h

#define	PASS	0
#define FAIL	1
typedef unsigned int    UWORD;		/*GIC*/
typedef	unsigned char	uchar;
typedef enum {x,y} SwState;
typedef enum {z,w} Bool;
#define ON 1					/* needed for Tz calls */
#define OFF 0

#endif
#include "defines.h"
#include "rubyii.h"
#include "r2serial.h"
#include "topazdrv.h"
#include "swi.h"
#include "physical.h"
#include "topaz.h"
#include "synth.h"


/**************************************************************************/

UINT8 
ldet( void )
{
  return PIOC.data & 1 ;
}

/**************************************************************************/
/* The EEPROM_DATA bit in the PIO direction register is already taken care of
*  in NVinit().  NVinit is called before this function.
**************************************************************************/
void 
init_PIO( void )
{
  /* Bit 0: LDET,        input.
     Bit 1: ANTSEL,      output.
     Bit 2: RCV_EN,      output.
     Bit 3: LED_GREEN,   output.
     Bit 4: LED_RED,     output.
     Bit 5: WATCH_DOG,   output.
     Bit 6: EEPROM_CLK,  output.
     Bit 7: EEPROM_DATA, both.
  */
  PIOC.dir |= 0x01 ;
}

/**************************************************************************/

void 
antsel( UINT8 sel )
{
  if( sel == ANT1 )
  {
    PIOC.data |= ANT1 ;
  }
  else
  {
    PIOC.data &= ~ANT1 ;
  }
}

/**************************************************************************/

void 
rcv_en( UINT8 en )
{
  if( en == RCV_ENABLE )
  {
    PIOC.data |= RCV_ENABLE ;
  }
  else
  {
    PIOC.data &= ~RCV_ENABLE ;
  }
}

/**************************************************************************/

/* See p. 8 and p. 23 of topaz manual. */

void 
tx_en( UINT8 on_or_off )
{
/* Select port 0 (pin 41) as an output. */
	TOPAZ->ioport &= ~0x10;   /*configure bit 0 for output*/

	if( on_or_off == TX_ON )
	{
		TOPAZ->ioport |= 0x01;    /*set bit 0*/
	}
	else
	{
		TOPAZ->ioport &= ~0x01;   /*clear bit 0*/
	}
}

/**************************************************************************/

UINT8 
read_rssi( void )
{
	UINT8 r1 ;

	/* Turn on A to D. */
	TzRssiA2D( ON ) ;

	/* Select the mux. */
	TzSelMux( 0 ) ;

	TzStartMeas();		/* start an A/D measurement */
	TzWaitMeas();		/* wait for it to complete */
	r1 = TzRssiRD();	/* get RSSI value */

	/* Apply slope correction factor. From p. 27 of the TOPAZ manual:
     V = (adcrefp - adcrefn) * (2 * r1 + 1)/ 512 
    
  */

  return r1 ;
}

/**************************************************************************/

void 
rfdac( void )
{
  INT8 r1 ;
  INT32 df ;

  /* Read rxmeas. */
  r1 = (INT8)TOPAZ->rxmeas ;

  /* Convert to duty factor. See p. 26 of topax manual. */
  /*  df = ((128 + r1) * 100) / 256 ; */
  df = ((128 + r1) * 100) >> 8 ;
}

/**************************************************************************/
void pet_watchdog( void )
{
  static UINT8 pet ;

  /* Set pio-d5 to output. */
  PIOC.dir &= 0xDF ;

  /* Toggle the watchdog. */
  if( pet == 0 )
  {
    pet = 1 ;
    PIOC.data |=  0x20 ;
  }
  else
  {
    pet = 0 ;
    PIOC.data &= ~0x20 ;
  }
}


#if 0
/**************************************************************************/

int main( void )
{
  UINT32 i ;
  volatile UINT32 j, k ;
  while( 1 )
  {
    /* Frequencies must be integral multiples of 30000. */
    for( i=819990000; i<=855000000; i+=30000 )
    {
      set_freq_norm( i ) ;
      for( j=0; j<6000; j++ )
      {
        k++ ;
      }
    }
  }
}
#endif
