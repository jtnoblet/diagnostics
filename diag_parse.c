/*******************************************************************
**                                                                **
**   File Name:  diag_parse.c                                     **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	This file contains the parsing functions and parse table  **
**      for the LC-CDPD diagnostics.                             **
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
#include <ctype.h>
#include "diag.h"

/* Global diagnostic settings */
struct s_diag_configuration diag_configuration;

/*declarations for static functions in this file*/
static int ParseCmdlineTokens(char *, PARAMS *);
static void StrToUpper(char *);
static void DiagDisplayError(DIAG_RESULT, OUT_FUNC);
extern void sli_send_response(unsigned char res_code);

typedef struct {
	char *command;
	DIAG_RESULT (*diag_func)(PARAMS *, OUT_FUNC);
} DIAG_PARSE_TABLE;

static const DIAG_PARSE_TABLE diag_parse_table[] = { 
	/* CMD_NAME     FUNCTION  */
	{ "HW1",       diag_hw_memtest },
	{ "HW2",       diag_hw_flmread },
	{ "HW3",       diag_hw_flmwrite },
	{ "HW4",       diag_hw_adc },
	{ "HW5",       diag_hw_rfdac },
	{ "HW6",       diag_hw_sym_on },
	{ "HW7",       diag_hw_sym_off },
	{ "HW8",       diag_hw_led },
	{ "HW9",       diag_hw_wdi },
   
	{ "RF1",       diag_rf_channel },
	{ "RF2",       diag_rf_rfdac },
	{ "RF3",       diag_rf_pwr },
	{ "RF4",       diag_rf_rcven },
	{ "RF5",       diag_rf_txen },
/* 	{ "RF6",       diag_rf_antsel }, */  /* Removed by Bankim */
	{ "RF7",       diag_rf_txdac },
	{ "RF8",       diag_rf_ldet },
	{ "RF9",       diag_rf_pdet },
	{ "RF10",      diag_rf_rssi },
	{ "RF11",      diag_rf_rcvdata },
	{ "RF12",      diag_rf_ber },
	{ "RF13",      diag_rf_bler },
	{ "RF14",      diag_rf_bit_det },
	{ "RF15",      diag_rf_rx_polarity },
	{ "RF16",      diag_rf_ofdac },
	{ "RF17",      diag_rf_topaz_regs },
	{ "SYNTH",     diag_rf_progsynth },
    { "PATEST",    diag_rf_patest },

	{ "CHANSET",   diag_cdpd_chanset },
	{ "POWER",     diag_cdpd_power },
	{ "CDPDSTAT",  diag_cdpd_cdpdstat },
	{ "MAC",       diag_cdpd_macstat },
	{ "RRME",      diag_cdpd_rrmestat },
	{ "MDLP",      diag_cdpd_mdlpstat },
	{ "MNRP",      diag_cdpd_mnrpstat },
	{ "LMEIMSG",   diag_cdpd_lmeimsg },
	{ "RSSI",      diag_cdpd_rssi },
	{ "AIRLINK",   diag_cdpd_airlink },

	{ "POKE",      diag_poke },
	{ "PEEK",      diag_peek },
	{ "TCB",       diag_tcb },
	{ "STACK",     diag_stack },
	{ "BUFFER",    diag_buffer },
	{ "QUEUE",     diag_queue },
	{ "VER",       diag_version },
    { "RESET",     diag_reset },
	{ "LOG",       diag_log },

	{ "RSSICAL",   diag_cal_rssi },
	{ "TXCAL",     diag_cal_txpwr },
	{ "RFDAC",     diag_cal_rfdac },
	{ "OFDAC",     diag_cal_ofdac },
	{ "XTAL",      diag_cal_xtal },
	{ "SLEEP",     diag_sleep },
	{ "T203",      diag_t203 },
	{ "GPS",       diag_gps },
	{ "APP",       diag_app },
	{ 0, 0}    /*0 here to mark the end of the table*/
};

/***********************************************************************
* Parse the command to see which diagnostics function is to be 
* executed.
* This is the function which is called from the AT command parser and
* the debug task.
***********************************************************************/
DIAG_RESULT 
diag_parse_command(char *command_pointer, OUT_FUNC caller_output)
{
	register int i = 0;
	PARAMS lParams;
	DIAG_RESULT rtn;

	/*convert the command pointer to upper case*/
	StrToUpper(command_pointer);

	/*parse the (space-delimited) tokens in the command*/
	if (ParseCmdlineTokens(command_pointer, &lParams))
	{

		rtn = DIAG_COMMAND_NOT_FOUND;

		/*try to find a match for token 0 in the parse table*/
		while(diag_parse_table[i].command)
		{
			if (strcmp(diag_parse_table[i].command, lParams.params[0]) == 0)
			{
				/*a match was found so execute the function*/		 
				rtn = diag_parse_table[i].diag_func(&lParams, caller_output);
				break;
			}
			i++;
		}
	}
	else
	{
		rtn = DIAG_RESULT_BAD_PARAM;  /* probably too many parameters */
	}
	DiagDisplayError(rtn, caller_output);
	return (rtn);

}
/***********************************************************************
*  Display any errors returned by the called function.
***********************************************************************/
static void 
DiagDisplayError(DIAG_RESULT err, OUT_FUNC caller_output)
{
	extern unsigned char mcp_verboseMode;

	switch (err)
	{
   	case DIAG_RESULT_OK:
		sli_send_response(0);
        break;

	case DIAG_COMMAND_NOT_FOUND:
		if(mcp_verboseMode)
			caller_output("\r\nERROR-command not found\r\n");
		else
			caller_output("\r\n4\r\n");
		break;

    case DIAG_RESULT_ERROR:
    case DIAG_RESULT_BAD_PARAM:
	default:
		sli_send_response(4);
        break;
    }
}

/***********************************************************************
*  Convert a string to all upper-case.
***********************************************************************/
static void 
StrToUpper(char *p)
{
	while(*p)
	{
		*p = toupper(*p);
		p++;
	}
}

/***********************************************************************
*  Parse any tokens from the command line string.  These are placed in an
*  array of up to ten token pointers.
***********************************************************************/
#define CMD_LINE_DELIMITERS " \n\r"
static int 
ParseCmdlineTokens(char *cmdline, PARAMS *p)
{
	register int i = 0;
	
	/*initialize the values in p*/
	memset(p, 0, sizeof(PARAMS));

	/*the first command-line parameter will be the basic command*/
	p->params[0] = strtok(cmdline, CMD_LINE_DELIMITERS);
	p->param_count = 1;

	for (i=1; i<MAX_PARAMS; i++)
	{
		p->params[i] = strtok((char *)NULL, CMD_LINE_DELIMITERS);
		if (!(p->params[i]))
			break;

		p->param_count++;
	}
	if (strtok((char *)NULL, CMD_LINE_DELIMITERS))
	{
		return 0;
	}
	else
	{
		return 1;
	}
		
}


void 
diag_init(void)
{
	memset(&diag_configuration, 0, sizeof(struct s_diag_configuration));
}
