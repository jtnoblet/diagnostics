/*******************************************************************
**                                                                **
**   File Name:  diag_api.h                                       **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	Description of application program interface to LC_CDPD   **
**      diagnostics functions.                                    **
**      This file should be included in any source file that      **
**      needs to access the diagnostics functions.                **
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
#ifndef _DIAGAPI_H_INCL
#define _DIAGAPI_H_INCL


typedef enum  {   DIAG_RESULT_OK, 
                  DIAG_RESULT_ERROR,
				  DIAG_RESULT_BAD_PARAM,
				  DIAG_COMMAND_NOT_FOUND
              } DIAG_RESULT;

typedef int (*OUT_FUNC)(const char *fmt, ...);

/*this is the prototype for the diagnostics parser.  It expects a pointer to 
the command string (the stuff following the "AT") and a pointer to the function
to be used for displaying any results.  */
DIAG_RESULT diag_parse_command(char *command_pointer, OUT_FUNC caller_output);

/* Calling this function sets a diagnostics configuration variable that
will keep the modem tuned to a particular channel*/
extern void diag_set_channel(unsigned short channel_number);
/* Calling this function sets a diagnostics configuration variable that
keeps the modem transmitting at a fixed power level*/
extern void diag_set_power_level(unsigned short power_level);

#endif
