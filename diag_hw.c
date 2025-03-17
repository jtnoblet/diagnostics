/*******************************************************************
**                                                                **
**   File Name:  diag_hw.c                                        **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	This file contains the diagnostic functions needed to     **
**		support hardware development and testing.                 **
**                                                                **
**   Revision History:                                            **
**   9/4/96   Jeff Noblet  Create                                 **
**                                                                **
**                                                                **
**                                                                **
**   Copyright (c) 1996, Uniden San Diego Research Center (USRC)  **
**   Unpublished Work. All Rights Reserved.                       **
**                                                                **
**                                                                **
*******************************************************************/

#include <stdio.h>
#include <string.h> 
#include <stdarg.h> 
#include <stdlib.h>
#include <ctype.h>
#include "defines.h"

#include "typedefs.h"
#include "topaz.h"
#include "physical.h"

#include "g_types.h"
#include "g_proto.h"
#include "g_cdpd_const.h"

#include "rubyii.h"
#include "NVram.h"

#include "uart.h"

#include "diag.h"

/*This is a "shadow register" for the PIO port*/
unsigned char PIO_register;

/****************************************************************************
| This function calls an ASM fuction to test the SRAM.  Since the ASM routine
| doesn't make any attempt to preserve registers or existing SRAM contents, this
| test should be considered destructive.
| The ASM routine will loop on any address that it can't write and read successfully.
| 
| Commented-out call to MemTest.  Should write one in C, or figure out how
| to add asm files to the make.
****************************************************************************/
DIAG_RESULT 
diag_hw_memtest(PARAMS *p, OUT_FUNC caller_out)
{
	/*MemTest runs forever*/
#if 0
	MemTest();
#endif
	return DIAG_RESULT_OK;
}

DIAG_RESULT 
diag_hw_flmread(PARAMS *p, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}

DIAG_RESULT 
diag_hw_flmwrite(PARAMS *p, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}


DIAG_RESULT 
diag_hw_adc(PARAMS *p, OUT_FUNC caller_out)
{
	int test_count = 1;
	int i;
	unsigned char adc_reading;



	if (p->params[1])
	{
		test_count = atoi(p->params[1]);
	}
	
	
	/*select ADC 2*/
	TOPAZ->config = 0x83;	
	caller_out("HWTEST Init ADC2.  Count = %d\n", test_count);
   

	for (i=1; i<=test_count; i++)
	{
		TzSelMux(2);  /*select the Temp sensor*/
	
		TzStartMeas();
		TzWaitMeas();
	
		adc_reading = TzRssiRD();

		/*print a message*/
		caller_out("HWTEST read 0x%X from ADC2 %d.\n", adc_reading, i);
		
		DELAY_MS(100);
	}
	return DIAG_RESULT_OK;
}

/*Test the function of the radio compensation DAC */
DIAG_RESULT 
diag_hw_rfdac(PARAMS *p, OUT_FUNC caller_out)
{
	uchar counter = 0;
	int test_count = 0;
	int i;

	test_count = atoi(p->params[1]);
	caller_out("Run RFDAC test.  Count=%d\n", test_count);
	TzCompD2A(ON);
	for (i=1; i<=test_count; i++)
	{
		
		counter = 0;
		while(counter < 0xFF)
		{
			TzCompWR(counter++);
			DELAY_MS(1);
		}
		while(counter > 0)
		{
			TzCompWR(counter--);
			DELAY_MS(1);
		}
		
	}
	TzCompD2A(OFF);

	return DIAG_RESULT_OK;
}

/***************************************************************************
| start generating a pseudo-random pattern with the transmit symbol generator
***************************************************************************/
DIAG_RESULT 
diag_hw_sym_on(PARAMS *p, OUT_FUNC caller_out)
{
	caller_out("Starting pattern test\n");
/*  This bit should be set already */
/*	TOPAZ->pwrcon = TOPAZ->pwrcon |= TZ_SYM; */
	TOPAZ->tmode = TOPAZ->tmode &= ~TZ_TESTBITS;
	TOPAZ->tmode = TOPAZ->tmode |= TZ_SRCBITS;

	return DIAG_RESULT_OK;
}

/*turn off the symbol generator test*/
/*refer to page 28 of the Topaz data sheet*/
DIAG_RESULT 
diag_hw_sym_off(PARAMS *p, OUT_FUNC caller_out)
{
	caller_out("Disabling pattern test\n");
/* Clearing this bit disables the SCC tx clock */
/*	TOPAZ->pwrcon = TOPAZ->pwrcon &= ~TZ_SYM; */
	TOPAZ->tmode = TOPAZ->tmode &= ~TZ_TESTBITS;
	TOPAZ->tmode = TOPAZ->tmode &= ~TZ_SRCBITS;

	return DIAG_RESULT_OK;
}

/*****************************************************************************
| This test lights the LED to red->green->yellow->off at 1/4 second per color.
| It runs for 10 seconds.
******************************************************************************/
DIAG_RESULT 
diag_hw_led(PARAMS *p, OUT_FUNC caller_out)
{
	int repeat = 1;
	int i;
	
	if (p->params[1])
	{
		repeat = atoi(p->params[1]);
	}
	for (i = 0; i < repeat; i++)
	{
		SetLED(LED_BITS_RED);
		DELAY_MS(250);
		SetLED(LED_BITS_GREEN);
		DELAY_MS(250);
		SetLED(LED_BITS_YELLOW);
		DELAY_MS(250);
		SetLED(LED_BITS_OFF);
		DELAY_MS(250);
	}

	return DIAG_RESULT_OK;
}

DIAG_RESULT 
diag_hw_wdi(PARAMS *p, OUT_FUNC caller_out)
{
	int i;
	
	for (i = 1; i < 10 ; i++)
	{
		PIOC.data |= WDI;  /*write to  PIO */
		DELAY_MS(100);

		PIOC.data &= ~WDI;  /*write to  PIO */
		DELAY_MS(100);
	}
   
	for (i = 0; i< 100; i++)
	{
        TEST_POINT_1();
	}
    caller_out("Done\n");

	return DIAG_RESULT_OK;
}

/******************************************************************************
| This function tests the write/read ability of the serial EEPROM.  It should be 
| a destructive test since any data that was stored in the EEPROM could be lost.
******************************************************************************/
DIAG_RESULT 
diag_hw_serialEE(PARAMS *p, OUT_FUNC caller_out)
{
	unsigned int ee_loc;
	unsigned char value;
	unsigned char saved_value;
	const unsigned char test_value = 0xA5;

	NVinit();  /*this is actually done in airmain.c in the full-up system*/

	for (ee_loc = 0; ee_loc < NV_SIZE; ee_loc++)
	{
		saved_value = GetNVbyte(ee_loc); /*save whatever was there*/ 
		
		PutNVbyte(ee_loc, test_value);   /*write a test value to the EEPROM*/
		value = GetNVbyte(ee_loc);       /*try reading the test value*/

		if (value != test_value)
		{
			caller_out("Serial EE test fails at location %u\n", ee_loc);
		}
		
		PutNVbyte(ee_loc, saved_value);  /*restore the previous value*/
	}
	return DIAG_RESULT_OK;
}

void 
SetLED(char LED)
{
	PIOC.data &= ~(LED_BITS_RED | LED_BITS_GREEN) ;  /*first clear the LED bits 
                                                in the PIO register*/
	PIOC.data |= LED;					  /*set the LED bits*/
 
}




