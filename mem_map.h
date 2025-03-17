/**********************************************************************
*
*	C Header:		mem_map.h
*	Instance:		1
*	Description:	
*	%created_by:	jpotter %
*	%date_created:	Thu Oct 30 11:14:07 1997 %
*
**********************************************************************/
#ifndef _1_mem_map_h_H
#define _1_mem_map_h_H

#ifndef lint
static char    *_1_mem_map_h = "@(#) %filespec: mem_map.h-2 %  (%full_filespec: mem_map.h-2:incl:1 %)";
#endif

#define FLASH_BASE_ADDR    0x01000000
#define FLASH_SIZE         0x00100000
#define FLASH_END_ADDR     (FLASH_BASE_ADDR + FLASH_SIZE)

#define SRAM_BASE_ADDR     0x01400000
#define SRAM_SIZE          0x00100000
#define SRAM_END_ADDR      (SRAM_BASE_ADDR + SRAM_SIZE)




#endif
