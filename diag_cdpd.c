/*******************************************************************
**                                                                **
**   File Name:  diag_cdpd.c                                      **
**                                                                **
**   Author:     Jeff Noblet                                      **
**                                                                **
**   Description:                                                 **
**   	This file contains the functions needed by the diagnostics**
**		module to get CDPD operational status and statistics.     **
**		support hardware development.                             **
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
#include <stdlib.h>
#include "diag.h"
#include "typedefs.h"
#include "g_types.h"
#include "g_cdpd_const.h"
#include "g_cdpd_msg.h"
#include "g_airlink_msg.h"
#include "g_proto.h"
#include "NVram.h"
#include "lilrad.h"
#include "synth.h"
#include "types.h"             /* airlink */
#include "msgs.h"
#include "airstat.h"			/* externs for MAC stats */
#include "rrm.h"               /* externs for Airlink RRME */
#include "mac.h"
#include "prim.h"
#include "timer.h"
#include "mdlp.h"
#include "mdlp_st.h"
/* in drvrs/unirad.c */
extern unsigned int	cur_channel;	/* current channel radio is set to */
extern unsigned int	cur_Tx_power;	/* current Tx power setting (0 to 7) */
extern unsigned int	cur_xmit;		/* current transmitter setting (ON/OFF) */
extern unsigned int     iCurrentSort;
extern unsigned int     iNumOnSort;
extern unsigned int     debug_bler;
extern unsigned char    bBlerHigh;

extern signed char cur_temperature;
extern signed char read_temperature(void);

static int 
channel_in_range(long chan)
{
	if ( (chan >= 1 && chan <= 312) ||
         (chan >= 667 && chan <= 716) ||
         (chan >= 991 && chan <= 1023) ||
         (chan >= 355 && chan <= 666) ||
         (chan >= 717 && chan <= 799))
		return TRUE;
	else
		return FALSE;
}

DIAG_RESULT
diag_cdpd_chanset(PARAMS *p, OUT_FUNC caller_out)
{
	DIAG_RESULT rtn = DIAG_RESULT_OK;
	int i;
	char temp[60] = "";
	long lTemp;
	char *t;



	if (p->param_count == 1)
	{
		/*return the current set*/
		for (i=0; ;i++)
		{
			if (diag_configuration.chanset[i])
				sprintf(temp+strlen(temp), "%hd ", diag_configuration.chanset[i]);
			else
				break;
		}
		caller_out("%s\n", temp);
	}
	else
	{
		if (strcmp(p->params[1], "CLEAR") == 0)
		{
			memset(&diag_configuration.chanset, 0,
                        sizeof(diag_configuration.chanset));
		}
		else if (strcmp(p->params[1], "WRITE") == 0)
		{
			/* save the setting to NV memory */
			NVsave((unsigned char *)&diag_configuration.chanset[0],
                       NVlocChanSet, ChanSetLen);
		}
		else if (strcmp(p->params[1], "RESTORE") == 0)
		{
			/* restore the settings from NV memory */
			NVrestore( (unsigned char *)&diag_configuration.chanset[0],
                       NVlocChanSet, ChanSetLen);
		}
		else
		{
			lTemp = strtoul(p->params[1], &t, 0);
			if (t == p->params[1])  /* not a number */
				return DIAG_RESULT_ERROR;

			for (i=1; i < p->param_count && i <= MAX_CHANSET_CHANNELS ; i++)
			{
				lTemp = strtoul(p->params[i], &t, 0);
				if (t != p->params[i])  /* found a number */
				{
					if (!channel_in_range(lTemp))
					{
						rtn = DIAG_RESULT_ERROR;
						caller_out("Channel %ld out of range.\n", lTemp);
					}
				}
			}
			if (rtn == DIAG_RESULT_ERROR)
				return rtn;
							
			for (i=1; i < p->param_count && i <= MAX_CHANSET_CHANNELS ; i++)
			{
				lTemp = strtoul(p->params[i], &t, 0);
				if (t != p->params[i])  /* found a number */
					diag_configuration.chanset[i-1] = (unsigned short)lTemp;
			}
			for (i=p->param_count; i < MAX_CHANSET_CHANNELS; i++)
			{
				diag_configuration.chanset[i-1] = 0;
			}
		}
	}
	return rtn;
}
/****************************************************
* This function can be called from the diagnostics
* routines or from elsewhere in the system.
****************************************************/
void
diag_set_channel(unsigned short channel_number)
{
	diag_configuration.chanset[0] = channel_number;
}

DIAG_RESULT
diag_cdpd_power(PARAMS *p, OUT_FUNC caller_out)
{
	DIAG_RESULT rtn = DIAG_RESULT_OK;
	unsigned short power_level;
	char *ptr;

	if (p->param_count == 2)
	{
		power_level = (unsigned short) strtoul(p->params[1], &ptr, 0);
		if (ptr == p->params[1])
			rtn = DIAG_RESULT_ERROR;
		else
		{
			diag_set_power_level(power_level);
		}
	}
	else
	{
		/*return the current power level*/
		if (diag_configuration.power_level & 0x8000)
		{
			caller_out("%hd (fixed)\n", diag_configuration.power_level & ~0x8000);
		}
		else
		{
			caller_out("%hd (calculated)\n" ,cur_Tx_power);
		}
	}
	return rtn;
}
/*********************************************************************
* This function can be called from anywhere to force the M-ES to operate
* at a fixed power level.  Sending "99" as the power level will allow
* the normal CDPD power control algorithm to control the power.
*********************************************************************/
void
diag_set_power_level(unsigned short power_level)
{
	if (power_level == 99)
		diag_configuration.power_level = 0;
	else
	{
		diag_configuration.power_level = power_level | 0x8000;
		/* The power level has probably changed, need to update *
		*  the TXDAC.  PhPowerReq() will read the diag_configuration *
		*  variable to determine the correct level. */
	}
	/* The power will be updated the next time an RSSI measurement is made */
}

const char *RStateStrings[] =
{
  "Inactive",          /* CDPD_INACTIVE      = 0, */
  "Wide Area Scan",    /* WIDE_AREA_SCAN     = 1, */
  "Wide Area Search",  /* WIDE_AREA_SEARCH   = 2, */
  "Chan Qual Check",   /* CHAN_QUALITY_CHECK = 3, */
  "Chan Acquired",     /* CHANNEL_ACQUIRED   = 4, */
  "Ref Chan Scan",     /* REF_CHAN_SCAN      = 5, */
  "Chan Search",       /* CHANNEL_SEARCH     = 6, */
  "Directed Hop",      /* DIRECTED_HOP       = 7, */
  "Sleep",             /* SLEEP              = 8, */
  "RSSI Chan Scan"     /* RSSI_CHAN_SCAN     = 9  */
  "RRM Disabled"       /* RRM_DISABLED       = 10 */
};

static char *
make_rssi_string(char *t)
{
	unsigned char RSSI;		/* return this with a value in range 0 to 63 */
	unsigned char raw_rssi;

    cur_temperature = read_temperature();
	raw_rssi = read_rssi();			/* get the RAW RSSI value */
#ifdef ARMULATOR
	RSSI = raw2cal(raw_rssi);				/* convert to calibrated value */
#else
	RSSI = raw2cal(raw_rssi, cur_temperature);				/* convert to calibrated value */
#endif

	sprintf(t, "%hd (a/d 0x%X)\n", -113 + RSSI, raw_rssi);

	return t;
}

#define CELL_ID_STRING "Cell ID : "

DIAG_RESULT
diag_cdpd_cdpdstat(PARAMS *params, OUT_FUNC caller_out)
{
	ULONG lTEI = 0;
	char temp[30];
	extern BOOLEAN CurrentlyRegistered(void);
	extern RRM_STATE_DEF iRrmeState;

	caller_out("%25s%u\n", "Channel : ", cur_channel);

/* show RRME state */
	caller_out("%25s%s\n", "RRME State : ", RStateStrings[iRrmeState]);

/* display the current power level*/
	if (diag_configuration.power_level & 0x8000)
	{
		sprintf(temp,"%hd (fixed)\n", diag_configuration.power_level & ~0x8000);
	}
	else
	{
		sprintf(temp, "%hd (calculated)\n" ,cur_Tx_power);
	}
	caller_out("%25s%s", "TX Power : ", temp);

/* show the transmit status */
	caller_out("%25s%s\n", "TX : ", cur_xmit ? "ON" : "OFF");

/* show the current RSSI */
	make_rssi_string(temp);
	caller_out("%25s%s", "RSSI : ", temp);

/* show power product */
	caller_out("%25s%hu\n", "Pwr Product : ", 
         psCurrentCell ? psCurrentCell->ucPowerProd : 0);
	
/* show Cell ID */
	if (psCurrentCell)
	{
		caller_out("%25s%X%X (cell num %u)\n",
				    CELL_ID_STRING, 
                    psCurrentCell->uwSPNI, 
                    psCurrentCell->uwCellNum,
				    psCurrentCell->uwCellNum);
	}
	else
	{
		caller_out("%25sNONE\n", CELL_ID_STRING);
	}

/* show TEI */
	get_mes_current_tei(&lTEI);
	caller_out("%25s%lX\n", "Current TEI : ", lTEI);

/* Show registration status */
	caller_out("%25s%s\n", "Registered : ", 
         (CurrentlyRegistered() == TRUE) ? "Yes": "No");

	return DIAG_RESULT_OK;
}

struct s_mac_stats
{
	ULONG RevMacBlocks;
	ULONG RevBlockDecodeSuccess;
	ULONG RevBlockDecodeFail;
	ULONG FwdMacBlocks;
	ULONG FwdBlocksRejected;
	ULONG FwdBitErrors;
	ULONG FwdSymbolErrors;
	ULONG FwdBusyFlags;
	ULONG FwdTotalFlags;
	ULONG FwdSyncFound;
	ULONG FwdSyncLost;
	ULONG FwdMacOverRun;
} lCurMacStats, lPrevMacStats;
#define MAC_STAT_FMT "%-18s%10lu%10lu\n"
DIAG_RESULT
diag_cdpd_macstat(PARAMS *params, OUT_FUNC caller_out)
{

	extern unsigned long ulStatFwdSyncFound;
	extern unsigned long ulStatFwdSyncLost;

	struct s_mac_stats *pCur, *pPrev ;

    pCur = &lCurMacStats;
	pPrev = &lPrevMacStats;

	pCur->RevMacBlocks = ulStatRevMacBlocks;
	pCur->RevBlockDecodeSuccess = ulStatRevBlockDecodeSuccess;
	pCur->RevBlockDecodeFail = ulStatRevBlockDecodeFail;
	pCur->FwdMacBlocks = ulStatFwdMacBlocks;
	pCur->FwdBlocksRejected = ulStatFwdBlocksRejected;
	pCur->FwdBitErrors = ulStatFwdBitErrors;
	pCur->FwdSymbolErrors = ulStatFwdSymbolErrors;
	pCur->FwdBusyFlags = ulStatFwdBusyFlags;
	pCur->FwdTotalFlags = ulStatFwdTotalFlags;
	pCur->FwdSyncFound = ulStatFwdSyncFound;
	pCur->FwdSyncLost = ulStatFwdSyncLost;
	pCur->FwdMacOverRun = iRecOverRun;

/* set the current stats from the Airlink globals */

/* show the total and the delta since the last request */
/*	caller_out("%-25s%lu\n", "RevBlocksRejected", ulStatRevBlocksRejected);*/
/*	caller_out("%-25s%lu\n", "RevInterrupts", ulStatRevInterrupts);*/
	caller_out(MAC_STAT_FMT, "RevMacBlocks", ulStatRevMacBlocks,
          pCur->RevMacBlocks - pPrev->RevMacBlocks);
/*	caller_out("%-25s%lu\n", "RevMacFrames", ulStatRevMacFrames);*/
/*	caller_out("%-25s%lu\n", "RevMacOctets", ulStatRevMacOctets);*/
	caller_out(MAC_STAT_FMT,  "Decode Success", ulStatRevBlockDecodeSuccess,
          pCur->RevBlockDecodeSuccess - pPrev->RevBlockDecodeSuccess);
	caller_out(MAC_STAT_FMT,  "Decode Fail", ulStatRevBlockDecodeFail,
          pCur->RevBlockDecodeFail - pPrev->RevBlockDecodeFail);

	caller_out(MAC_STAT_FMT, "FwdMacBlocks", ulStatFwdMacBlocks,
          pCur->FwdMacBlocks - pPrev->FwdMacBlocks);
/*	caller_out("%-25s%lu\n", "FwdMacFrames", ulStatFwdMacFrames);*/
/*	caller_out("%-25s%lu\n", "FwdMacOctets", ulStatFwdMacOctets);*/
	caller_out(MAC_STAT_FMT, "FwdBlocksRejected", ulStatFwdBlocksRejected,
          pCur->FwdBlocksRejected - pPrev->FwdBlocksRejected);
	caller_out(MAC_STAT_FMT, "FwdBitErrors", ulStatFwdBitErrors,
          pCur->FwdBitErrors - pPrev->FwdBitErrors);
	caller_out(MAC_STAT_FMT, "FwdSymbolErrors", ulStatFwdSymbolErrors,
          pCur->FwdSymbolErrors - pPrev->FwdSymbolErrors);

/*	caller_out("%-25s%lu\n", "FwdInvalidFrames", ulStatFwdInvalidFrames);*/
/*	caller_out("%-25s%lu\n", "FwdAbortFrame", ulStatFwdAbortFrame);*/
/*	caller_out("%-25s%lu\n", "FwdMicroSlots", ulStatFwdMicroSlots);*/
	caller_out(MAC_STAT_FMT, "FwdBusyFlags", ulStatFwdBusyFlags,
          pCur->FwdBusyFlags - pPrev->FwdBusyFlags);
	caller_out(MAC_STAT_FMT, "FwdTotalFlags", ulStatFwdTotalFlags,
          pCur->FwdTotalFlags - pPrev->FwdTotalFlags);
	caller_out(MAC_STAT_FMT, "FwdSyncFound", ulStatFwdSyncFound,
          pCur->FwdSyncFound - pPrev->FwdSyncFound);
	caller_out(MAC_STAT_FMT, "FwdSyncLost", ulStatFwdSyncLost,
          pCur->FwdSyncLost - pPrev->FwdSyncLost);

	caller_out(MAC_STAT_FMT, "FwdMacOverRun", iRecOverRun, 
          pCur->FwdMacOverRun - pPrev->FwdMacOverRun);

/* copy current stats into previous stats */
	memcpy(pPrev, pCur, sizeof(struct s_mac_stats));
	return DIAG_RESULT_OK;
}

/*global table of all cell configuration data*/
extern CELL sCell[MAX_CELLS];

static void
display_cell_cfg(CELL *psCell, OUT_FUNC caller_out)
{
	UWORD *ptrChan;
	int j = 0;
	UWORD tChan;
	char tOutBuf[80] = "";
	char tempstr[8];
	int i;

	if ( psCell->ucValid )
	{
		caller_out("SPNI:%4X  CELL:%4X  Area:%X  Group: %X\n",
			psCell->uwSPNI,
			psCell->uwCellNum,
			psCell->ucAreaColor,
			psCell->ucCellColor);
		/*leave out SPI and WASI for now*/
		caller_out("RefChan:%hu  ErpDelta:%hd  RssiBias:%hd  Face:%hu\n",
			psCell->uwRefChan,
			psCell->cErpDelta,
			psCell->cRssiBias,
			psCell->ucFaceNeighbor);

		caller_out("PowerProd:%hu  MaxPwr:%hu  NumCsis:%hu\n",
			psCell->ucPowerProd,
			psCell->ucMaxPower,
			psCell->ucNumCsis);
		/*display the list of channels */
		ptrChan = (UWORD *)psCell->uwAllocChans;
		for (i=0; i< MAX_CHANS_IN_RRM_PKT; i++)
		{
			tChan = psCell->uwAllocChans[i] & CHANNEL_MASK;
			if ( !tChan )
			{
				break;
			}
			/* itoa would be better but it's not in the library */
			sprintf(tempstr, "%hu", tChan);


			strcat(tOutBuf, tempstr);
			strcat(tOutBuf, psCell->uwAllocChans[i] & 0x8000 ? "D ":" ");
			if ( !(++j % 15) )
			{
				caller_out("%s\n", tOutBuf);
				tOutBuf[0] = 0;
			}
			ptrChan++;
		}
		caller_out("%s\n", tOutBuf); /* display the leftovers */
	}
}

static void
diag_rrme_cellcfg_all(OUT_FUNC caller_out)
{
	CELL *psNextCell;

	for (psNextCell = &sCell[0];                      /* for all cells */
         psNextCell < &sCell[MAX_CELLS];
         psNextCell++)
  	{
    	if (psNextCell->ucValid)
		{
      		display_cell_cfg(psNextCell, caller_out);
    	}
  	}

}
static void
diag_rrme_cellcfg(OUT_FUNC caller_out)
{
	if (psCurrentCell)
		display_cell_cfg(psCurrentCell, caller_out);
	else
		caller_out("No current cell\n");
}
static void
diag_rrme_chanqual(OUT_FUNC caller_out)
{
	extern char cRssiHysteresis;
	extern unsigned char ucRefScanTime;
	extern unsigned char ucRssiScanDelta;
	extern unsigned char ucRssiAvgTime;
	extern unsigned char ucBlerThreshold;
	extern unsigned char ucBlerAvgTime;

	caller_out("RssiHysteresis %hd\n", (signed char)cRssiHysteresis);
	caller_out("RefScanTime %hu\n",    ucRefScanTime);
	caller_out("RssiScanDelta %hu\n",  ucRssiScanDelta);
	caller_out("RssiAvgTime %hu\n",    ucRssiAvgTime);
	caller_out("BlerThreshold %hu\n",  ucBlerThreshold);
	caller_out("BlerAvgTime %hu\n",    ucBlerAvgTime);
}

static void
diag_rrme_list(OUT_FUNC caller_out, int num_channels)
{
	extern SORT_LIST sRssiSort[];
	int i = 0;
	while ( sRssiSort[i].uwRfChan && i< num_channels)
	{
		caller_out("%2d %4hd %6hd %hd\n", i+1,
			sRssiSort[i].uwRfChan,
			-113 + sRssiSort[i].ucMeanRssi,
			sRssiSort[i].ucCellIndex);
		i++;
	}
}
static void
diag_rrme_chanacc(OUT_FUNC caller_out)
{
	caller_out("Not Available.\n");
}

/*-----------------11-04-96 11:58pm-----------------
 This function is called from AT$RRME.
 Parameters are:
	CELLCFG ALL	display cell config data
	CHANQUAL    display channel quality parameters
	CSID        display channel stream ID data
	LIST		display the sorted list
	CHANACC     display channel access parameters
--------------------------------------------------*/

DIAG_RESULT
diag_cdpd_rrmestat(PARAMS *p, OUT_FUNC caller_out)
{
	DIAG_RESULT rtn = DIAG_RESULT_OK;
	int iTemp;
	char *ptr;

	if ( p->param_count == 1 )
	{
		/*nothing requested*/
		rtn = DIAG_RESULT_ERROR;
	}
	else if ( strcmp(p->params[1], "CELLCFG" ) == 0)
	{
		if ( strcmp(p->params[2], "ALL") == 0 )
		{
			diag_rrme_cellcfg_all(caller_out);
		}
		else
		{
			diag_rrme_cellcfg(caller_out);
		}
	}
	else if ( strcmp(p->params[1], "CHANQUAL" ) == 0 )
	{
		diag_rrme_chanqual(caller_out);
	}
	else if ( strcmp(p->params[1], "LIST" ) == 0 )
	{
		iTemp = strtoul(p->params[2], &ptr, 0);
		if (ptr == p->params[2])
		{
		    iTemp = iNumOnSort;
		}
		if(iTemp > iNumOnSort)
		{
			iTemp = iNumOnSort;
		}
		diag_rrme_list(caller_out, iTemp);
	}
	else if ( strcmp(p->params[1], "CHANACC" ) == 0 )
	{
		diag_rrme_chanacc(caller_out);
	}
	else
	{
		rtn = DIAG_RESULT_ERROR;
	}

	return rtn;
}

DIAG_RESULT
diag_cdpd_mdlpstat(PARAMS *params, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}

DIAG_RESULT
diag_cdpd_mnrpstat(PARAMS *params, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}

/*
DIAG_RESULT
diag_cdpd_ping(PARAMS *params, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}

*/

DIAG_RESULT
diag_cdpd_lmeimsg(PARAMS *params, OUT_FUNC caller_out)
{
	return DIAG_RESULT_OK;
}


DIAG_RESULT
diag_cdpd_rssi(PARAMS *params, OUT_FUNC caller_out)
{
	char temp[30];
	make_rssi_string(temp);

	caller_out("%s\n", temp);
	caller_out("%hd (avg.)\n", -113 + ucMeanRssi);
	caller_out("Temp: %hd deg. C\n", cur_temperature);
 
	return DIAG_RESULT_OK;
}

/**********************************************************************
*
*  This function is used to start, stop, or resume the Airlink stack
*  task.  This may be necessary for a number of reasons, such as wanting
*  keep the RRM from attempting to change the channel.
***********************************************************************/
extern DIAG_RESULT send_dme_req(UINT8 req_type);

int BankimDebug;

DIAG_RESULT
diag_cdpd_airlink(PARAMS *p, OUT_FUNC caller_out)
{
	DIAG_RESULT rtn = DIAG_RESULT_OK;

	if (p->param_count == 2)
	{
		if (strcmp(p->params[1], "STOP") == 0)
		{
			rtn = send_dme_req(DME_STOP);
		}
		else if (strcmp(p->params[1], "RESUME") == 0)
		{
			rtn = send_dme_req(DME_RESUME);
		}
		else if (strcmp(p->params[1], "START") == 0)
		{
			rtn = send_dme_req(DME_START);
		}
		else if (strcmp(p->params[1], "RESTART") == 0)
		{
			rtn = send_dme_req(DME_RESTART);
		}
		else if (strcmp(p->params[1], "DEBUGON") == 0)
		{
			BankimDebug = 1;
		}
		else if (strcmp(p->params[1], "DEBUGOFF") == 0)
		{
			BankimDebug = 0;
		}
		else
		{
			rtn = DIAG_RESULT_BAD_PARAM;
		}
	}
	return rtn;
}

/******************************************************************/
DIAG_RESULT
send_dme_req(UINT8 req_type)
{
	airlink_msg_t *P;

	if((P = (airlink_msg_t *) get_buf(SysFreeQ32)) != NULL)
	{
		P->hdr.msg_type   = AIR_CMD_MSG_TYPE;
		P->hdr.src_taskid = PRINT_TASK_ID;

		P->subtype   = AIRLINK_DME_REQ;
		P->seq_no    = 1;
		P->rsp_queue = PRINT_QUEUE;
		P->data[0]   = req_type;

		if(enqueue(P, AIRLINK_TASK_ID, AIRLINK_QUEUE) == OS_NO_ERR)
		{
			return DIAG_RESULT_OK;
		}
	}
	return DIAG_RESULT_ERROR;
}

