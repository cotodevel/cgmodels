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

//---------------------------------------------------------------------------------
typedef struct sIPCSharedTGDSSpecific{
//---------------------------------------------------------------------------------
	uint32 frameCounter7;	//VBLANK counter7
	uint32 frameCounter9;	//VBLANK counter9
	char filename[256];
}  IPCSharedTGDSSpecific	__attribute__((aligned (4)));

//TGDS Memory Layout ARM7/ARM9 Cores
#define TGDS_ARM7_MALLOCSTART (u32)(0x06018000)
#define TGDS_ARM7_MALLOCSIZE (int)(16*1024)
#define TGDSDLDI_ARM7_ADDRESS (u32)(TGDS_ARM7_MALLOCSTART + TGDS_ARM7_MALLOCSIZE)	//0x0601C000
#define TGDS_ARM7_AUDIOBUFFER_STREAM (u32)(0x03800000)

#define REQ_GBD_ARM7 (u32)(0xffff1988)

#define FIFO_PLAYSOUNDSTREAM_FILE (u32)(0xFFFFABCB)
#define FIFO_STOPSOUNDSTREAM_FILE (u32)(0xFFFFABCC)
#define FIFO_PLAYSOUNDEFFECT_FILE (u32)(0xFFFFABCD)
#define workBufferSoundEffect0 (s16*)((int)0x06000000 + (96*1024) - (4096*4))
#define NO_VIDEO_PLAYBACK	1

#ifdef ARM9
static inline void initGDBSession(){
	SendFIFOWords((u32)REQ_GBD_ARM7, 0xFF);
}
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

extern struct sIPCSharedTGDSSpecific* getsIPCSharedTGDSSpecific();

//NOT weak symbols : the implementation of these is project-defined (here)
extern void HandleFifoNotEmptyWeakRef(u32 cmd1, uint32 cmd2);
extern void HandleFifoEmptyWeakRef(uint32 cmd1,uint32 cmd2);

extern void EWRAMPrioToARM7();
extern void EWRAMPrioToARM9();

#ifdef __cplusplus
}
#endif