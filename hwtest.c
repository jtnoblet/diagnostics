
#include <stdio.h>
#include <string.h> 
#include <stdarg.h> 
#include <ctype.h>
#include "defines.h"

#include "typedefs.h"
#include "topaz.h"
#include "physical.h"
#include "rubyii.h"
#include "lilrad.h"

#include "g_types.h"
#include "g_cfg.h"
#include "g_proto.h"
#include "g_cdpd_const.h"
#include "g_sli_mcp.h"
#include "g_sli_defs.h"

#include "uart2.h"

#include "diag.h"

#include "NVram.h"

extern void UART2_nmea_input_no_block( UINT8 ibyte );

static unsigned short uart2_rx_buf_len;
/******************************************************************
*  These are the initialization steps needed for the diagnostic (print)
*  task.  Some of the radio stuff is likely done in the airlink code,
*  but is here temporarily.
*******************************************************************/
void 
init_diag_task(void)
{
	/*initialize Ruby clock*/
	/*write to the external clock select register*/
	CPU_CLK = 0x2;

	initTopaz();
}
#if 0
int		
main(void)
{
	char parse_string[80];
	
	/*initialize Ruby clock*/
	/*write to the external clock select register*/
	CPU_CLK = 0x2;

	DiagInitUart2(19200, "\r", 80);

	DiagPrintf("HWTEST Initialized Ruby Clock.\n");

	initTopaz();

	puts("HWTEST Initialized Topaz.\n");



	while (1)
	{
		gets(parse_string);
	
		diag_parse_command(parse_string, DiagPrintf);

	} 
	return 0;
}
#endif

#define  MAX_RX_BUF_LEN 80

UINT8 *UART2_msg[2];
/**********************************************************************
*  This is the function that gets called when the UART2 driver has finished
*  sending a buffer.
**********************************************************************/
void 
DiagTransmitBufComplete(void)
{

}
/************************************************************************
| This is the function that is called by the UART2 driver when a complete
| buffer has been received.  In ASCII mode, a complete buffer is indicated 
| by a CR.
| Null-terminated if the appropriate #define is set in uart2.c
|
| len is the number of characters received
************************************************************************/

void 
DiagReceiveBufComplete(UINT16 len)
{
	UINT8 *current_buf, *next_buf;
	static int index = 0;

	int enqueue_status; 

	current_buf = UART2_msg[index];
	index ^= 1;
	next_buf = UART2_msg[index];

	/* call UART2_read_buffer with the maximum length */
	UART2_read_buffer( next_buf + 2, uart2_rx_buf_len ) ;

	*current_buf  = DIAG_CMD_MSG_TYPE;
	*(current_buf+1) = (UINT8) len;  /*len was passed as a parameter */

	steal_buf(current_buf);      /* change ownership to the current task */
	/* post a message to the diagnostics task */
	if(enqueue(current_buf, PRINT_TASK_ID, PRINT_QUEUE) != OS_NO_ERR)
	{
		enqueue_status = 1;
	}
	else
	{
		enqueue_status = 0;
	}
}

UINT8 *test_string = (UINT8 *)"";

void 
DiagInitUart2(unsigned short baud_rate, char *term_chars, unsigned short term_len)
{
	extern UINT8 LogFreeQ;
	UINT16 stat;

	static int already_initialized = FALSE;

	if (already_initialized)
		return;

	already_initialized = TRUE;

	/* Get these two buffers for communicating UART2 data from the
	ISR level to the diagnostics task.  They are probably not going to
	be released.  Trying to call get_buf from within an interrupt context 
	didn't seem to work. */

	UART2_msg[0] = (UINT8 *)get_buf(LogFreeQ);
	UART2_msg[1] = (UINT8 *)get_buf(LogFreeQ);

	uart2_rx_buf_len = term_len;
	/*initialize the UART2 driver */

    /* Initialize UART2. */
	if (sregs[SREG_120].value == 0)
	{
		stat = UART2_init(
			ASCII_NO_BLOCK,
			baud_rate,
			UART2_ascii_input_no_block,
			UART2_ascii_output,
			DiagReceiveBufComplete,
			DiagTransmitBufComplete,
			term_chars) ;
	}
	else
	{
		stat = UART2_init(
			ASCII_NO_BLOCK,
			baud_rate,
			UART2_nmea_input_no_block,
			UART2_ascii_output,
			DiagReceiveBufComplete,
			DiagTransmitBufComplete,
			term_chars) ;

	}
	/*send a test string */
	UART2_write_buffer(test_string, (UINT16)strlen((char *)test_string) );		   
	/* call UART2_read_buffer with the maximum length */	
	UART2_read_buffer( (UINT8 *)(UART2_msg[0] + 2), 
             term_len ) ;
	UART2_read_buffer( (UINT8 *)(UART2_msg[0] + 2), 
             term_len ) ;


}

