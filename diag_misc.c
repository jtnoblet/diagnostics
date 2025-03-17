/*******************************************************************
**                                                                **
**   File Name:  diag_misc.c                                      **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	This file contains some miscellaneous diagnostic functions**
**	    that didn't seem to belong in any of the other modules.   **
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "typedefs.h"
#include "g_types.h"
#include "g_cdpd_const.h"
#include "diag.h"
#include "mem_map.h"
#include "NVram.h"
#include "swi.h"			/* need to disable interrupts */
#include "version.h"
#include "g_cfg.h"
#include "../../util/src/ulcp_ext.h"

extern unsigned char mcp_verboseMode;

/*****************************************************************
*  Parameters are the address and the byte value.
*****************************************************************/
DIAG_RESULT 
diag_poke(PARAMS *p, OUT_FUNC caller_out)
{
	unsigned long address;
	unsigned char value;
	unsigned char fill;
	char *ptr;

	if (p->param_count < 3)    /* 0=poke, 1=address, 2=value 3=fill */
		return DIAG_RESULT_ERROR;

	address = strtoul(p->params[1], &ptr, 0);
	if (ptr == p->params[1])
		return DIAG_RESULT_ERROR;
	value = (unsigned char) strtoul(p->params[2], &ptr, 0);
	if (ptr == p->params[2])
		return DIAG_RESULT_ERROR;
	fill = (unsigned char) strtoul(p->params[3], &ptr, 0);
	if (ptr == p->params[3])
		fill = 0;
	/* TODO:  check memory bounds */
	if (address < 2048)
	{
		/*write to Serial EE*/
		if (fill)
		{
			while(fill--)
			{
				NVsave(&value, (int)address, 1 );
				address++;
			}
		}
		else
		{
			NVsave(&value, (int)address, 1 );
		}
	}
	else if (address >= FLASH_BASE_ADDR && address <= FLASH_END_ADDR)
	{
		caller_out("Flash poke not supported\n");
		return DIAG_RESULT_ERROR;
	}
	else /* if (address >= SRAM_BASE_ADDR && address <= SRAM_END_ADDR) */
	{
		if (*p->params[4] == 'W')
		{
			address &= 0xfffffffc;  /*make sure divisible by 4*/
			*(unsigned long *) address = value;
		}
		else
		{
			*(unsigned char *) address = value;
		}
	}
	caller_out("poke (%d) 0x%02x to address 0x%x\n", fill, value, address);


	return DIAG_RESULT_OK;
}

/*****************************************************************
*  Display the contents of memory.
*  Parameters are the start address and (optional) length.
*  Input is in hex (0x...) or decimal.
*****************************************************************/
static char *
bytetohex(unsigned char value, char *string)
{
	const char hex_table[] = "0123456789ABCDEF";
	*string = hex_table[(value & 0xf0) >> 4];
	*(string + 1) = hex_table[value &0x0f];
	*(string +2) = 0;
	return string;
}
DIAG_RESULT 
diag_peek(PARAMS *p, OUT_FUNC caller_out)
{
	char temp[80];
	char lstr[15];
	char print_string[17];
	char *ptr;
	unsigned char value;
	unsigned long start_addr, end_addr, length;

	/*parse the start address and length*/
	start_addr = strtoul(p->params[1], &ptr, 0);



	if (ptr == p->params[1])
		return DIAG_RESULT_ERROR;
	if (p->param_count > 2)
	{
		length = strtoul(p->params[2], &ptr, 0);
		if (length < 16)
			length = 16;
		if (ptr == p->params[2])  /* strtoul didn't find anything */
			return DIAG_RESULT_ERROR;
	}
	else
	{
		length = 16;
	}
	start_addr &= 0xfffffff0;	/* always start the display at
								     * an even 16 byte boundary */
	end_addr = start_addr + length;
		
	if (start_addr >= SRAM_BASE_ADDR)
	{
		if (end_addr >= SRAM_END_ADDR)
			return DIAG_RESULT_ERROR;
	}
	else if (start_addr >= FLASH_BASE_ADDR)
	{
		if (end_addr >= FLASH_END_ADDR)
			return DIAG_RESULT_ERROR;
	}
	else if (start_addr < 2048)
	{
		if (end_addr > 2048)
		{
			end_addr = 2048;
		}
	}
	print_string[16] = 0;
	while (start_addr < end_addr)
	{
		if ((start_addr % 16) == 0)
		{
			sprintf(temp, "%08lX:  ", start_addr);
		}
		if (start_addr < 2048)
			NVrestore((UINT8 *)&value, start_addr, 1 );
		else
			value = *(unsigned char *)start_addr;
		strcat(temp, bytetohex(value, lstr));
		strcat(temp, " ");
		if (isprint(value))
			print_string[start_addr % 16] = value;
		else
			print_string[start_addr % 16] = '.';
		start_addr++;
		/* if at an even 16 byte boundary, or if at the end of the 
		*  range of peek addresses, print out the display string */
		if ( ((start_addr % 16) == 0) || (start_addr == end_addr))
			caller_out("%s | %s\n", temp, print_string);
		
	}

	return DIAG_RESULT_OK;
}



/*****************************************************************
*
*****************************************************************/
DIAG_RESULT 
diag_tcb(PARAMS *params, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}

DIAG_RESULT 
diag_version(PARAMS *p, OUT_FUNC caller_out)
{

	caller_out("Software Version %s\n", CORE_SW_VERSION);
	return DIAG_RESULT_OK;
}

DIAG_RESULT
diag_reset(PARAMS *p, OUT_FUNC caller_out)
{
	/*
	* WARNING:
	*	This code will restart the system and should never
	*	return to any following statements.
				*/
	SWI_DisableINT();	/* disable both IRQ and FIQ interrupts */
	__main();			/* inner restart vector */
	/*
	* WARNING:
	*	point of NO return
	*/

}

/***************************************************************************
*  This function is used to set the logging port to UART1 or UART2.
*  It may also be expanded to set other logging parameters.
***************************************************************************/
DIAG_RESULT
diag_log(PARAMS *p, OUT_FUNC caller_out)
{
	extern void SetLogPort(UINT8);
	extern UINT8 GetLogPort(void);
	char *ptr;
	UINT8 port_number;

	if (strcmp(p->params[1], "PORT")==0)
	{
		/*get the port number */
		port_number = (UINT8) strtoul(p->params[2], &ptr, 0);
		if (ptr != p->params[2])
		{
			SetLogPort(port_number);
		}
		caller_out("Log Port: %hu\r\n", GetLogPort());
	}
	else
	{
		return DIAG_RESULT_ERROR;
	}
	return DIAG_RESULT_OK;
}

DIAG_RESULT
diag_sleep(PARAMS *p, OUT_FUNC caller_out)
{
	UINT8 flag;
	extern BOOLEAN iUseSleep;

	if(p->params[1])
	{
		if((strcmp(p->params[1], "0") == 0) ||
		   (strcmp(p->params[1], "1") == 0) ||
		   (strcmp(p->params[1], "2") == 0))
		{
			flag = atoi(p->params[1]);
			user_modify_req(CFG_T203_FLAG, &flag, 1, AIRLINK_TASK_ID, AIRLINK_QUEUE);
			return DIAG_RESULT_OK;
		}
		else
		{
			return DIAG_RESULT_ERROR;
		}
	}
	else
	{
		/* show the current mode */
		if(mcp_verboseMode)
		{
			caller_out("Sleep Mode = %d\r\n", iUseSleep);

		}
		else
		{
			caller_out("%d\r\n", iUseSleep);
		}
		return DIAG_RESULT_OK;
	}
}

DIAG_RESULT
diag_t203(PARAMS *p, OUT_FUNC caller_out)
{
	INT32 val;
	extern unsigned int glb_t203_value;

	if(p->params[1])
	{
		val = atoi(p->params[1]);
		if((val == 0) || (val >= 30))
		{
			user_modify_req(CFG_T203, &val, 4, AIRLINK_TASK_ID, AIRLINK_QUEUE);
			return DIAG_RESULT_OK;
		}
		else
		{
			return DIAG_RESULT_ERROR;
		}
	}
	else
	{
		/* show the current value */
		if(mcp_verboseMode)
			caller_out("T203 Timer = %ld sec\r\n", (unsigned long)glb_t203_value);
		else
			caller_out("%ld\r\n", (unsigned long)glb_t203_value);
		return DIAG_RESULT_OK;
	}
}

#if 0
DIAG_RESULT
diag_led(PARAMS *p, OUT_FUNC caller_out)
{
	extern BOOLEAN led_status;
	extern 	void SetLED(char LED);

	if(p->params[1])
	{
		if((strcmp(p->params[1], "1") == 0) ||
		   (strcmp(p->params[1], "0") == 0))
		{
			if(strcmp(p->params[1], "1") == 0)
				led_status = 0;
			else
				led_status = 1;

			/* Turn off the led, the timer logic will take care of the rest */
			SetLED(0); 
			return DIAG_RESULT_OK;
		}
		else
		{
			return DIAG_RESULT_ERROR;
		}
	}
	else
	{
		/* show the current value */
		if(mcp_verboseMode)
			caller_out("LED: %s\r\n", (led_status == 0) ? "1" : "0" );
		else
			caller_out("%s\r\n", (led_status == 0) ? "1" : "0" );
			
		return DIAG_RESULT_OK;
	}
}
#endif

