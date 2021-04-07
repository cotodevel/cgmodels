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

#ifndef __main7_h__
#define __main7_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "spitscTGDS.h"
#include "soundTGDS.h"
#ifdef __cplusplus
extern "C" {
#endif

extern int main(int argc, char **argv);
extern void Wifi_initJuglak7();
extern unsigned char MacAddr[6];
extern int Wifi_TxCheck();
extern void Wifi_MACCopy(u16 * dest, u32 MAC_Base, u32 MAC_Offset, u32 length);
extern void Wifi_TxRawJuglak7(u16 * data, int datalen);
extern void Wifi_SetChannel7Juglak(int channel);

extern struct XYTscPos first, tempPos;
extern int vcount;
extern bool GDBStarted;

extern unsigned char *tx_queue[64],*rx_queue[64];
extern unsigned int *rx_sizes,*tx_sizes;
extern unsigned int *RIPC;
extern void RIPC_Cmd(bool interupt);

#ifdef __cplusplus
}
#endif


#define WIFI_REG(ofs) (*((volatile u16 *)(0x04800000+(ofs))))

// make these accessable by both cores
#define rx_count RIPC[16]
#define rx_base RIPC[18]
#define tx_base RIPC[19]

#define WIFI_READY RIPC[20]

#endif

