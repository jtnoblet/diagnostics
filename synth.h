#ifndef SYNTH_H
#define SYNTH_H

#ifdef SYNTH_C
#define EXTERN
#else
#define EXTERN extern
#endif

extern unsigned char XTALvalue; 

#define W1_LEN        20
#define W2_LEN        15
#define W3_LEN        18
#define W4_LEN        15
#define W5_LEN        19
#define W6_LEN         6

#define HIGH_FREQ     14850000
#define LOW_FREQ      14400000

#define NEW_VCO 

/*#define SYNTH_FREQ    HIGH_FREQ*/

/* LOW_FREQ */

 #ifdef NEW_VCO
  #define WORD2H_1440        (UINT32)((UINT32)0x1E02 << (32-W2_LEN))
 #else 
  #define WORD2H_1440        (UINT32)((UINT32)0x0F02 << (32-W2_LEN))
 #endif
 #define WORD4H_1440        (UINT32)((UINT32)0x0F04 << (32-W4_LEN))



/* HIGH_FREQ */

 #ifdef NEW_VCO
  #define WORD2H_1485        (UINT32)((UINT32)0x1EF2 << (32-W2_LEN))
 #else
  #define WORD2H_1485        (UINT32)((UINT32)0x0F7A << (32-W2_LEN))
 #endif 
 #define WORD4H_1485        (UINT32)((UINT32)0x0F7C << (32-W4_LEN))



#define WORD3H        (UINT32)((UINT32)0x2E6B << (32-W3_LEN))
#define WORD5H        (UINT32)((UINT32)0x30FD << (32-W5_LEN))
#define WORD6H_NORM   (UINT32)((UINT32)0x1E   << (32-W6_LEN))
#define WORD6H_SPDU   (UINT32)((UINT32)0x3E   << (32-W6_LEN))

#define	S_Base  				0x0800E00
#define	SERIAL_BASE			0x0800D00

/*
 * from Ruby II User's manual Table 7.4.3-1 SERIAL_CONTROL
 */
#define	SERIAL_C0			0x04
#define	SERIAL_C1			0x0C
#define	SERIAL_C2			0x14
/*
 * from Ruby II User's manual Table 7.4.4-1 SERIAL_DATA_REGISTER
 */
#define	SERIAL_REG_C0		0x00
#define	SERIAL_REG_C1		0x08
#define	SERIAL_REG_C2		0x10
/*
 * from Ruby II User's manual Table 7.4-5 SERIAL_CLK_CONTROL
 */
#define	SERIAL_CLK_CONTROL	0x18
/*
 * from Ruby II User's manual Table 7.4-6 SERIAL_STATUS_REGISTER
 */
#define	SERIAL_STS			0x1C
/*
 * How LilGuy is using the serial controller
 */
#define	SERIAL_CLK	SERIAL_C1		/*C1 setup register for clock */
#define	SERIAL_WIN	SERIAL_REG_C1	/*C1 clock window register */
									/* NOTE: Must end in same Cx as SERIAL_CLK */

#define	SERIAL_DAT	SERIAL_C2		/*C2 setup register for data */
#define	SERIAL_I_O	SERIAL_REG_C2	/*C2 data I/O register */
									/* NOTE: Must end in same Cx as SERIAL_DAT */

#define	SERIAL_ENB	SERIAL_C0		/*C0 setup register for enable */
#define	SERIAL_GATE	SERIAL_REG_C0	/*C0 enable I/O register */
									/* NOTE: Must end in same Cx as SERIAL_ENB */
#define	ONE			0x800			/* 0x800 for serial 0x001 for PIO */
#define	ZERO		0

#define ANT1        0x2
#define ANT2        0x0

#define RCV_ENABLE    0x04
#define RCV_DISABLE   0

#define TX_ON       1
#define TX_OFF      0

extern void set_freq_norm(UINT32);
extern void set_freq_speedup(UINT32);
extern UINT8 ldet(void);
extern void init_PIO(void);
extern void antsel(UINT8);
extern void rcv_en(UINT8);
extern void tx_en(UINT8);
extern void rfdac(void);
extern UINT8 read_rssi(void);
long	chan2freq(int);

extern void pet_watchdog(void);

#endif
