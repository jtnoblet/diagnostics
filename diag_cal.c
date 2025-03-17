/*******************************************************************
**                                                                **
**   File Name:  diag_cal.c                                       **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 ** 
**   	RSSI and TX power level calibration routines.             **
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
#include "defines.h"
#include "NVram.h"
#include "typedefs.h"
#include "g_types.h"
#include "physical.h"
#include "topaz.h"
#include "synth.h"
#include "diag.h"
#include "lilrad.h"

unsigned char ucCalStatus;

static const char strDisplay[] = "DISPLAY";
static const char strWrite[] =   "WRITE";
static const char strClear[] =   "CLEAR";
static const char strRestore[] = "RESTORE";
static const char strTempCorrection[] = "TEMPCORR";
static const char strTempADC[] = "TEMPADC";
static const char strRstCalStat[] = "RSTCALSTAT";

static void fill_in_table(unsigned char table[]);
/******************************************************************
*  Calibrate the table the converts A/D values into dBm.
*  param[0] == "RSSICAL"
*  param[1] == "DISPLAY" the current RAM copy
*              "WRITE" the RAM copy to EEPROM
*              "CLEAR" zero out the RAM copy
*              "RESTORE" from EEPROM to RAM
*               dBm  calibration point value
*              "TEMPCORR" a b c  (ax^2 +bx + c)
*              "TEMPADC" m b     (mx + b)
*              "RSTCALSTAT"  reset the calibration status to "OK"
                      
*  param[2] == an optional hex or dec number if param[1] was a dBm
*              value
*  param[2,3,4] == optional coefficients for temperature/RSSI and ADC
******************************************************************/
DIAG_RESULT
diag_cal_rssi(PARAMS *p, OUT_FUNC caller_out)
{
	char *ptr;
	short table_index;
	short cmdline_rssi;
	unsigned char cal_value;
	int i;
	long lTemp; 

	if (p->param_count < 2 /*|| p->param_count > 3*/)
		return DIAG_RESULT_ERROR;

	if (strcmp(p->params[1], strDisplay) == 0)
	{
		/*display the current RAM copy of the RSSI calibration table */
		for (i=0; i< RawCallen; i++)
		{
			caller_out("%hd        0x%x\n", -113+i, RSSIcal[i]);
		}
	}
	else if (strcmp(p->params[1], strWrite) == 0)
	{
		/*fill in some numbers for any 0 entries in the table*/
		fill_in_table(RSSIcal);
		NVsave(RSSIcal, NVlocRawCal, RawCallen );
		cal_checksum_update();
	}
	else if (strcmp(p->params[1], strClear) == 0)
	{
		for (i=0; i< RawCallen; i++)
		{
			RSSIcal[i] = 0;
		}
	}
	else if (strcmp(p->params[1], strRestore) == 0)
	{
		NVrestore( RSSIcal, NVlocRawCal, RawCallen);
	}
	else if (strcmp(p->params[1], strTempCorrection) == 0)
	{
		/*if no parameters, display the current settings*/
		if (p->param_count == 2)
		{
			caller_out("%ld, %ld, %ld\n", 
                 RSSI_temperature_coeff[0],
                 RSSI_temperature_coeff[1],
                 RSSI_temperature_coeff[2]);
		}
		/*get the parameters and save them*/
		else
		{
			for (i=2; i<=4 ; i++)
			{
				lTemp = strtol(p->params[i], &ptr, 10);
				if (ptr == p->params[i] || i > p->param_count)
					RSSI_temperature_coeff[i-2] = 0;
				else
					RSSI_temperature_coeff[i-2] = lTemp;
			}
			NVsave((unsigned char *)&RSSI_temperature_coeff[0], 
				   NVlocRSSITempCoeff,
				   RSSITempCoeffLen);
			cal_checksum_update();
		}
	}
	else if (strcmp(p->params[1], strTempADC) == 0)
	{
		/*if no parameters, display the current settings*/
		if (p->param_count == 2)
		{
			caller_out("%ld, %ld\n", 
                 Temperature_ADC_coeff[0],
                 Temperature_ADC_coeff[1]);
		}
		/*get the parameters and save them*/
		else
		{
			for (i=2; i<=3 ; i++)
			{
				lTemp = strtol(p->params[i], &ptr, 10);
				if (ptr == p->params[i] || i > p->param_count)
					Temperature_ADC_coeff[i-2] = 0;
				else
					Temperature_ADC_coeff[i-2] = lTemp;
			}
			NVsave((unsigned char *)&Temperature_ADC_coeff[0], 
				   NVlocTempADCCoeff,
				   TempADCCoeffLen);
			cal_checksum_update();
		}
	}
	else if (strcmp(p->params[1], strRstCalStat) == 0)
	{
		ucCalStatus = 0;
		PutNVbyte(NVlocCalStatus, ucCalStatus);
		cal_checksum_update();
	}
	else if (strcmp(p->params[1], "CKSUM") == 0)
	{
		if (cal_checksum_OK())
		{
			caller_out("checksum OK\n");
		}
		else
		{
			caller_out("checksum error\n");
		}
	}
	else
	{
		/* cmdline_rssi should be in the range of -113 to -50 in order *
		*  to correspond to the positions 0 to 64 in the array        */
		cmdline_rssi = (short) strtol(p->params[1], &ptr, 10);
		if (ptr == p->params[1])    /*see if there is a number there*/
			return DIAG_RESULT_ERROR;
		table_index = 113 + cmdline_rssi;
		if (table_index < 0 || table_index > 63)   /*check the range*/
			return DIAG_RESULT_ERROR;
		if (p->param_count == 3)    /*see if there is a "forced" value*/
		{
			cal_value = (unsigned char) strtol(p->params[2], &ptr, 0);
			if (ptr != p->params[2])
			{
				RSSIcal[table_index] = cal_value;
			}
		}
		else
		{
			cal_value = read_rssi();
			/*correct the calibration value for temperature before storing*/

			RSSIcal[table_index] = cal_value;
		}
		caller_out("RSSI: %hd, ADC: 0x%X\n", cmdline_rssi, cal_value);
	}
	return DIAG_RESULT_OK;			
}

DIAG_RESULT
diag_cal_txpwr(PARAMS *p, OUT_FUNC caller_out)
{
	const signed char txpwr_levels[TxPwrlen] = 
             { -2, -2, -2, -6, -10, -14, -18, -22, -22, -22, -22};
	/*          0   1   2   3    4    5    6    7    8    9   10 */
	const signed char txpwr_booster_levels[TxPwrlen] = 
             { -2, -2, -2, -6, -10, -14, -18, -22, -26, -30, -34};
	/*          0   1   2   3    4    5    6    7    8    9   10 */
	char *ptr;
	short cmdline_rssi;
	unsigned char cal_value;
	int i;
	DIAG_RESULT rtn = DIAG_RESULT_ERROR;

	if (p->param_count < 2 || p->param_count > 3)
		return DIAG_RESULT_ERROR;

	if (strcmp(p->params[1], strDisplay) == 0)
	{
		/*display the current RAM copy of the RSSI calibration table */
		for (i=0; i< TxPwrlen; i++)
		{
			caller_out("%hd        0x%x\n", i, TxPwr[i]);
		}
		rtn = DIAG_RESULT_OK;
	}
	else if (strcmp(p->params[1], strWrite) == 0)
	{
		NVsave(TxPwr, NVlocTxPwr, TxPwrlen );
		cal_checksum_update();
		rtn = DIAG_RESULT_OK;
	}
	else if (strcmp(p->params[1], strClear) == 0)
	{
		for (i=0; i< TxPwrlen; i++)
		{
			TxPwr[i] = 0;
		}
		rtn = DIAG_RESULT_OK;
	}
	else if (strcmp(p->params[1], strRestore) == 0)
	{
		if(sregs[SREG_113].value == 0)
		{
			NVrestore( TxPwr, NVlocTxPwr, TxPwrlen);
			TxPwr[8] = TxPwr[7]; 
			TxPwr[9] = TxPwr[7]; 
			TxPwr[10] = TxPwr[7]; 
			rtn = DIAG_RESULT_OK;
		}
		else if(sregs[SREG_113].value == 1)
		{
			NVrestore( TxPwr, NVlocTxPwr, TxPwrlen);
			rtn = DIAG_RESULT_OK;
		}
	}
	else
	{
		cmdline_rssi = (short) strtol(p->params[1], &ptr, 10);
		if (ptr == p->params[1])    /*see if there is a number there*/
			return DIAG_RESULT_ERROR;
		cal_value = (unsigned char) strtol(p->params[2], &ptr, 0);
		if (ptr == p->params[2])
			return DIAG_RESULT_ERROR;

		if(sregs[SREG_113].value == 0)
		{
			for (i=0; i<TxPwrlen; i++)
			{
				if(txpwr_levels[i] == cmdline_rssi)
				{
					TxPwr[i] = cal_value;
					rtn = DIAG_RESULT_OK;
				}
			}
		}
		else if(sregs[SREG_113].value == 1)
		{
			for (i=0; i<TxPwrlen; i++)
			{
				if(txpwr_booster_levels[i] == cmdline_rssi)
				{
					TxPwr[i] = cal_value;
					rtn = DIAG_RESULT_OK;
				}
			}
		}
	} 
	return  rtn;		
}

/***************************************************************************
*  The initial value for the RFDAC needs to be stored in the calibration 
*  table.
*  If no parameters are passed, report the current value of the RFDAC.
***************************************************************************/
DIAG_RESULT
diag_cal_rfdac(PARAMS *p, OUT_FUNC caller_out)
{
	char *ptr;
	unsigned char rfdac_value;

	if (p->param_count == 1)
	{
		rfdac_value = TOPAZ->rfdac;	   
	}
	else if (p->param_count == 2)
	{
		if (strcmp(p->params[1], strWrite) == 0)
		{
			/*write whatever has been programmed into the RFDAC into 
			* NV memory*/
			rfdac_value = TOPAZ->rfdac;
			PutNVbyte(NVlocRFDAC, rfdac_value);
			cal_checksum_update();
			caller_out("save ");
		}
		else if (strcmp(p->params[1], strRestore) == 0)
		{
			rfdac_value = GetNVbyte(NVlocRFDAC);
			TzCompWR(rfdac_value);
			caller_out("restore ");
		}
		else
		{
			rfdac_value = (UINT8)strtol(p->params[1], &ptr, 0);
			if (ptr == p->params[1])
				return DIAG_RESULT_ERROR;
			else
			{
				/*program the passed value into the RFDAC */
				TzCompWR(rfdac_value);
			}
		}
	}
	else
	{
		return DIAG_RESULT_ERROR;
	}
	caller_out("%d (hex %02X)\n", rfdac_value, rfdac_value);

	return DIAG_RESULT_OK;	
}

/***************************************************************************
*  The initial value for the OFDAC needs to be stored in the calibration 
*  table.
*  If no parameters are passed, report the current value of the OFDAC.
***************************************************************************/
DIAG_RESULT
diag_cal_ofdac(PARAMS *p, OUT_FUNC caller_out)
{
	char *ptr;
	unsigned char ofdac_value;

	if (p->param_count == 1)
	{
		ofdac_value = TOPAZ->ofdac;
	}
	else if (p->param_count == 2)
	{
		if (strcmp(p->params[1], strWrite) == 0)

		{
			/*write whatever has been programmed into the RFDAC into 
			* NV memory*/
			ofdac_value = TOPAZ->ofdac;
			PutNVbyte(NVlocOFDAC, ofdac_value);
			cal_checksum_update();
			caller_out("save ");
		}
		else if (strcmp(p->params[1], strRestore) == 0)
		{
			ofdac_value = GetNVbyte(NVlocOFDAC);
			TzRxOffsetWR(ofdac_value);
			caller_out("restore ");
		}
		else
		{
			ofdac_value = (UINT8)strtol(p->params[1], &ptr, 0);
			if (ptr == p->params[1])
				return DIAG_RESULT_ERROR;
			else
			{
				/*program the passed value into the OFDAC */
				TzRxOffsetWR(ofdac_value);
			}
		}
	}
	else
	{
		return DIAG_RESULT_ERROR;
	}
	caller_out("%d (hex %02X)\n", ofdac_value, ofdac_value);
	return DIAG_RESULT_OK;	
}
/* Store the antenna preference setting.  This will be restored on power-up. */

#define CURRENT_ANTENNA()  ((PIOC.data & 0x2) ? ANT1 : ANT2)

DIAG_RESULT
diag_cal_antenna(PARAMS *p, OUT_FUNC caller_out)
{
#if 0
	char *ptr;
	unsigned char antenna_value;

	if (p->param_count == 1)
	{
		antenna_value = CURRENT_ANTENNA(); 
 	}
	else if (p->param_count == 2)
	{
		if (strcmp(p->params[1], strWrite) == 0)
		{
			/*write whatever has been programmed into the RFDAC into 
			* NV memory*/
			PutNVbyte(NVlocAntenna, antenna_value);
			caller_out("save ");
		}
		else if (strcmp(p->params[1], strRestore) == 0)
		{
			antenna_value = GetNVbyte(NVlocAntenna);
			caller_out("restore ");
		}
		else
		{
			antenna_value = (UINT8)strtol(p->params[1], &ptr, 0);
			if (ptr == p->params[1])
				return DIAG_RESULT_ERROR;
			else
			{
				/*program the passed value into the RFDAC */
				TzCompWR(rfdac_value);
			}
		}
	}
	else
	{
		return DIAG_RESULT_ERROR;
	}
	caller_out("%d (hex %02X)\n", rfdac_value, rfdac_value);

	return DIAG_RESULT_OK;	
#endif
	return DIAG_RESULT_ERROR;
}
/***********************************************************************
*  Go through the RSSI table and fill in any blank spots with interpolated
*  values.
*  This function is intended to operate on the global RSSIcal table.
*  If the end points are not calibrated, the calibration can not be
*  considered complete.
***********************************************************************/
static void 
fill_in_table(unsigned char table[])
{
	struct s_calpair
	{
		unsigned char value;
		unsigned char index;
	} v1, v2, temp;
	
	int delta;
	

	v1.index = 0;
	v1.value = table[v1.index];
	
	while (1)
	{
		for (v2.index = v1.index + 1; 
		     v2.index < RawCallen; 
			 v2.index++)
	    {
            /* look for a point that has been calibrated */
			if ((v2.value = table[v2.index]) != 0)
			{
				/*need to guard against 0's at the end of the table, 
                * as in no calibration value for -50 dBm*/
				break;
			}
		}
        /* check for end of table */
		if (v2.index == RawCallen)
			return;
		
		/*fill in the blank spots*/
		if ((v2.index - v1.index) > 1)
		{
			delta = v2.value - v1.value;	
			for (temp.index = v1.index + 1;
			     temp.index < v2.index;
				 temp.index++)
			{
				/*interpolate between the calibrated values*/
				RSSIcal[temp.index] = v1.value + 
                       (delta * (temp.index -v1.index))/(v2.index - v1.index);
			}
		}
		if (v2.index >= (RawCallen - 1))
			break;
		v1 = v2;
	}
}
DIAG_RESULT 
diag_cal_xtal(PARAMS *p, OUT_FUNC caller_out)
{

	unsigned char xtal_temp;
	char *ptr;
	if (p->param_count == 1)
	{
		/* return current value */
		caller_out("current crystal setting %hd (%s)\n", XTALvalue,
				   XTALvalue ? "14.85" : "14.40");
		return DIAG_RESULT_OK;
	}

	if (p->param_count == 2)
	{
		xtal_temp = (unsigned char) strtol(p->params[1], &ptr, 0);
		if (ptr != p->params[1])
		{
			if (xtal_temp == 0 || xtal_temp == 1)
			{
				
				XTALvalue = xtal_temp;
				PutNVbyte(NVlocXTAL, XTALvalue);
				cal_checksum_update();
				return DIAG_RESULT_OK;
			}
		}
	}

	return DIAG_RESULT_ERROR;
			
}

/* figure out if the saved checksum is correct for the stored values in 
the calibration area of the serial eeprom */

int
cal_checksum_OK(void)
{
	int start, end;
	int i;
	unsigned char temp_cksum = 0;
	unsigned char current_cksum;
 
	
	start = NVlocRFDAC;
	end = NVlocCalCkSum;

	for (i = start; i <  end; i++)
	{
		temp_cksum += GetNVbyte(i);
	}
	
	current_cksum = GetNVbyte(NVlocCalCkSum );

/*		SysPrint(0, "Start = %d, end = %d\n", start, end);*/
	SysPrint(0, "Cksum %hd  Calculated %hd\n", current_cksum, temp_cksum);

	if (current_cksum == temp_cksum)
	{
		ucCalStatus &= ~CAL_STAT_ERROR;
		ucCalStatus |= CAL_STAT_OK;
		PutNVbyte(NVlocCalStatus, ucCalStatus);
		return TRUE;
	}
	else
	{
		ucCalStatus &= ~CAL_STAT_OK;
		ucCalStatus |= CAL_STAT_ERROR;
		PutNVbyte(NVlocCalStatus, ucCalStatus);
		return FALSE;
	}
}

/* compute the checksum for the calibration data and update the checksum
in nmvram.c */
void
cal_checksum_update(void)
{
	int start, end;
	int i;
	unsigned char current_cksum = 0;
 
	
	start = NVlocRFDAC;
	end = NVlocCalCkSum;

    SysPrint(0, "Start = %d, end = %d\n", start, end);
	for (i = start; i <  end; i++)
	{
		current_cksum += GetNVbyte(i);
	}
	SysPrint(0, "new checksum %hd\n", current_cksum);
	PutNVbyte(NVlocCalCkSum, current_cksum );
}

/* use default values for the calibration tables */
void 
cal_restore_defaults(void)
{
	/*assume that at least the RSSI and TX power values in EEPROM are ok*/
	/*get the RSSI table*/
	initRSSI();				

    /*get the tx power table*/
	initTxPwr();				
	
	/*get the temperature coefficients*/
	/*these are new parameters, so they are saved to the eeprom*/
	RSSI_temperature_coeff[0] = 16;
	RSSI_temperature_coeff[1] = -1960;
	RSSI_temperature_coeff[2] = 76370;
	NVsave((unsigned char *)&RSSI_temperature_coeff[0], 
		   NVlocRSSITempCoeff,
		   RSSITempCoeffLen);

	Temperature_ADC_coeff[0] = 39;
	Temperature_ADC_coeff[1] = -2792;
	NVsave((unsigned char *)&Temperature_ADC_coeff[0], 
		   NVlocTempADCCoeff,
		   TempADCCoeffLen);
	
	/*get the OFDAC setting */
	ucOFDACval = GetNVbyte(NVlocOFDAC);

	/*get the RFDAC setting */
	ucRFDACval = GetNVbyte(NVlocRFDAC);

	/*get the XTAL setting*/
	XTALvalue = 0;
	PutNVbyte(NVlocXTAL, XTALvalue);	
	
	ucCalStatus |= CAL_STAT_USING_DEFAULTS;
	PutNVbyte(NVlocCalStatus, ucCalStatus);
}



int
cal_restore_from_ee(void)
{
	/*get the RSSI table*/
	initRSSI();				/*initialize cal[] for raw2cal from NVram*/

    /*get the tx power table*/
	initTxPwr();			/*initialize TxPwr[] for Tx_power_to_DAC from NVram*/	
	
	/*get the temperature coefficients*/
	NVrestore((unsigned char *)&RSSI_temperature_coeff[0], 
              NVlocRSSITempCoeff, 
              RSSITempCoeffLen);

	NVrestore((unsigned char *)&Temperature_ADC_coeff[0], 
              NVlocTempADCCoeff, 
              TempADCCoeffLen);

	/*get the OFDAC setting */
	ucOFDACval = GetNVbyte(NVlocOFDAC);

	/*get the RFDAC setting */
	ucRFDACval = GetNVbyte(NVlocRFDAC);

	/*get the XTAL setting*/
	XTALvalue = GetNVbyte(NVlocXTAL);

	ucCalStatus = CAL_STAT_OK;
	return 0;
}


