/*******************************************************************
**                                                                **
**   File Name:  diag.h                                           **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	Defines and external declarations needed by the various   **
**      diagnostics modules.                                     **
**                                                                **
**                                                                **
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
#ifndef _DIAG_H_INCL
#define _DIAG_H_INCL
  #ifndef _DIAGAPI_H_INCL
    #include "diag_api.h"
  #endif

#define MAX_CHANSET_CHANNELS 10
/* Global diagnostic settings */
extern struct s_diag_configuration 
{
	unsigned short chanset[MAX_CHANSET_CHANNELS];
	unsigned short power_level;
	unsigned short synth_mode;
} diag_configuration;

#define SYNTH_SPEEDUP 0
#define SYNTH_NORMAL  1

extern void diag_init(void);

#define MAX_PARAMS 11
typedef struct s_params
{
	char *params[MAX_PARAMS];
	int  param_count;
} PARAMS;
/* All command parameters are passed as an 
 | array of strings*/

/*LED defines*/
#define LED_BITS_OFF		0
#define LED_BITS_GREEN	    0x10
#define LED_BITS_RED		0x08
#define LED_BITS_YELLOW	    (LED_BITS_RED | LED_BITS_GREEN)    
#define WDI			        0x20

#define TEST_POINT_1()  *(volatile unsigned char *)(0x1c00000) ^= 0xff
#define TEST_POINT_2()  *(volatile unsigned char *)(0x1d00000) ^= 0xff
#define TEST_POINT_3()  *(volatile unsigned char *)(0x1e00000) ^= 0xff
/*in nvram.c*/
extern int uSdelay(long dly);

#define DELAY_MS(x)			(uSdelay(1000*x))
#define DELAY_US(x)			(uSdelay(x))


/*functions declared in diag_hw.c*/
extern DIAG_RESULT diag_hw_memtest(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_flmread(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_flmwrite(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_adc(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_rfdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_sym_on(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_sym_off(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_led(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_wdi(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_hw_serialEE(PARAMS *, OUT_FUNC);

int DiagPrintf(char const *fmt, ...);
int DiagPrintfU1(char const *fmt, ...);
void DiagInitUart2(unsigned short baud_rate,  char *term_chars, unsigned short term_len);
/*external functions needed by diag_hw.c*/
extern void init_ADC2(void);
extern void SetLED(char LED);
extern void MemTest(void);				/*in memtest.s*/


/*functions declared in diag_radio.c*/
extern DIAG_RESULT diag_rf_channel(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_rfdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_pwr(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_rcven(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_txen(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_antsel(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_txdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_ldet(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_pdet(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_rssi(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_rcvdata(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_ber(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_bler(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_bit_det(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_rx_polarity(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_ofdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_topaz_regs(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_progsynth(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_rf_patest(PARAMS *, OUT_FUNC);

/*functions declared in diag_cdpd.c*/
extern DIAG_RESULT diag_cdpd_chanset(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_power(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_cdpdstat(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_macstat(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_rrmestat(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_mdlpstat(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_mnrpstat(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_lmeimsg(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_rssi(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cdpd_airlink(PARAMS *, OUT_FUNC);

/*functions declared in diag_misc.c*/
extern DIAG_RESULT diag_poke(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_peek(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_tcb(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_version(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_reset(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_log(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_sleep(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_t203(PARAMS *, OUT_FUNC);

/* in util/diagapp.c */
extern DIAG_RESULT diag_app(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_gps(PARAMS *, OUT_FUNC);

/*functions declared in startup.c*/
extern DIAG_RESULT diag_stack(PARAMS *, OUT_FUNC);

/*functions declared in buffer.c*/
extern DIAG_RESULT diag_buffer(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_queue(PARAMS *, OUT_FUNC);

/*functions in diag_cal.c*/
extern DIAG_RESULT diag_cal_rssi(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cal_txpwr(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cal_rfdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cal_ofdac(PARAMS *, OUT_FUNC);
extern DIAG_RESULT diag_cal_xtal(PARAMS *, OUT_FUNC);
#endif

