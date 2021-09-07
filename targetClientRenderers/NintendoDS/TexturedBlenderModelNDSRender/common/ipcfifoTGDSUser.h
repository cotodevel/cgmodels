/*

			Copyright (C) 2017  Coto
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

//TGDS required version: IPC Version: 1.3

//IPC FIFO Description: 
//		struct sIPCSharedTGDS * TGDSIPC = TGDSIPCStartAddress; 														// Access to TGDS internal IPC FIFO structure. 		(ipcfifoTGDS.h)
//		struct sIPCSharedTGDSSpecific * TGDSUSERIPC = (struct sIPCSharedTGDSSpecific *)TGDSIPCUserStartAddress;		// Access to TGDS Project (User) IPC FIFO structure	(ipcfifoTGDSUser.h)

//inherits what is defined in: ipcfifoTGDS.h
#ifndef __ipcfifoTGDSUser_h__
#define __ipcfifoTGDSUser_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "ipcfifoTGDS.h"

#define SBUFCOUNT 16
#define SBUFSIZE  44100

#define SBUFSECS 4
#define SFREQ 22050
#define SBYTES 2

#define SSAMPLES 1152
#define SBSIZE SSAMPLES*2

#define MP2BUFSIZE	1152*16
#define MP2CHUNKSIZE 64*1024

typedef struct sTransferRegion3 {
  //uint32 heartbeat;          // counts frames
 
  unsigned char CMD;
 // int test;
  //int cursound;
  //unsigned int test2;
  //unsigned short *sounddata;
  //unsigned short *sounddata2;
  //unsigned short *sdx1,*sdx2;

  int lastwritten;
  
  unsigned int sticks;
  unsigned int lticks;
  unsigned int cticks;
  bool hastick;
  
  unsigned char *mp2pnt;
  unsigned char *mp2pnt2;
  unsigned int mp2size;
  bool mp2chunk;
  bool mp2end;
  bool mp2needchunk;
  
  unsigned short *mp2bufl;
  unsigned short *mp2bufr;
  bool stopmp2;
  
  unsigned short *mp3bufl;
  unsigned short *mp3bufr;
  unsigned int mp3freq;
  unsigned int mp3outbufsize;
  
  long signed int t1,t2,t3,t4;
  
} TransferRegion3, * pTransferRegion3;

//---------------------------------------------------------------------------------
typedef struct sIPCSharedTGDSSpecific{
//---------------------------------------------------------------------------------
	uint32 frameCounter7;	//VBLANK counter7
	uint32 frameCounter9;	//VBLANK counter9
}  IPCSharedTGDSSpecific	__attribute__((aligned (4)));

//TGDS Memory Layout ARM7/ARM9 Cores
#define TGDS_ARM7_MALLOCSTART (u32)(0x06000000)
#define TGDS_ARM7_MALLOCSIZE (int)(112*1024)
#define TGDSDLDI_ARM7_ADDRESS (u32)(TGDS_ARM7_MALLOCSTART + TGDS_ARM7_MALLOCSIZE)

#define IPC3 ((TransferRegion3 volatile *)(((u32)TGDSIPCStartAddress + TGDSIPCSize)+sizeof(struct sIPCSharedTGDSSpecific)))
#define LRIPC IPC3

#define strpcmControl_NOP (0)
#define strpcmControl_Play (1)
#define strpcmControl_Stop (2)

#define LCDPC_NOP (0)
#define LCDPC_OFF_BOTH (1)
#define LCDPC_ON_BOTTOM (2)
#define LCDPC_ON_TOP_LEDON (3)
#define LCDPC_ON_TOP_LEDBLINK (4)
#define LCDPC_ON_BOTH (5)
#define LCDPC_SOFT_POWEROFF (0xff)

#define REQ_GBD_ARM7 (u32)(0xffff1988)
#define REQ_TX_FRAME (u32)(0xffff1A11)

#ifdef ARM9
static inline void initGDBSession(){
	SendFIFOWords((u32)REQ_GBD_ARM7);
}
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific();

//NOT weak symbols : the implementation of these is project-defined (here)
extern void HandleFifoNotEmptyWeakRef(volatile u32 cmd1);
extern void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2);

extern void EWRAMPrioToARM7();
extern void EWRAMPrioToARM9();

#ifdef __cplusplus
}
#endif