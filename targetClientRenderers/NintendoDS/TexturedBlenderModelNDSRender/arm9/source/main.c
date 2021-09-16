/*
			Copyright (C) 2017  Coto
This program is TGDSARM9Free software; you can redistribute it and/or modify
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

/*---------------------------------------------------------------------------------

 	Copyright (C) 2005
		Jason Rogers (dovoto)
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.

---------------------------------------------------------------------------------*/

#include "main.h"
#include "dsregs.h"
#include "dsregs_asm.h"
#include "typedefsTGDS.h"
#include "gui_console_connector.h"
#include "dswnifi_lib.h"
#include "TGDSLogoLZSSCompressed.h"
#include "ipcfifoTGDSUser.h"
#include "fatfslayerTGDS.h"
#include "cartHeader.h"
#include "ff.h"
#include "dldi.h"
#include "xmem.h"
#include "dmaTGDS.h"
#include "eventsTGDS.h"
#include "timerTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include "biosTGDS.h"
#include "keypadTGDS.h"
#include "global_settings.h"
#include <stdio.h>
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "VideoGL.h"
#include "videoTGDS.h"
#include "imagepcx.h"

#include "Cube.h"	//Mesh Generated from Blender 2.49b
#include "Texture_Cube.h"	//Textures of it

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
void menuShow(){
	clrscr();
	printf("                              ");
	printf(" TexturedBlenderModelNDSRender example ");
	printf(" Use the D-PAD to move the rendered polygon ");
	printf(" Polygon rendered and textured from Blender natively. ");
	
	printf("Available heap memory: %d", getMaxRam());
	printf("Button (Select): this menu. ");
	printarm7DebugBuffer();
}

//TGDS Soundstreaming API
int internalCodecType = SRC_NONE; //Returns current sound stream format: WAV, ADPCM or NONE
struct fd * _FileHandleVideo = NULL; 
struct fd * _FileHandleAudio = NULL;

bool stopSoundStreamUser(){
	
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];
int TGDSProjectReturnFromLinkedModule() {
	return -1;
}

int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: true
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else if(ret == -1)
	{
		printf("FS Init error.");
	}
	switch_dswnifi_mode(dswifi_idlemode);
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//gl start
	float camDist = 0.1*4;
	float rotateX = 0.0;
	float rotateY = 0.0;
	int i;
	{
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewPort(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset();
		
		LoadGLTextures((u8*)&Texture_Cube);
		
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(35, 256.0 / 192.0, 0.1, 40);
		
	}
	//gl end
	
	menuShow();
	
	while(1) {
		
		glPushMatrix();
		
		glMatrixMode(GL_POSITION);
		glLoadIdentity();
		
		gluLookAt(	0.0, 0.0, camDist,		//camera possition 
				0.0, -1.0, 4.0,		//look at
				0.0, 1.0, 0.0);		//up
		
		glTranslatef32(0, 0, 0.0);
		glRotateXi(rotateX);
		glRotateYi(rotateY);

		glMaterialf(GL_EMISSION, RGB15(31,31,31));

		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK );

		scanKeys();
		u32 keys = keysHeld();

		if( keys & KEY_UP ) rotateX += 3;
		if( keys & KEY_DOWN ) rotateX -= 3;
		if( keys & KEY_LEFT ) rotateY += 3;
		if( keys & KEY_RIGHT ) rotateY -= 3;
		
		if( keys & KEY_A ) camDist -= 0.1;
		if( keys & KEY_B ) camDist += 0.1;
		
		glBindTexture( 0, textureID );
		glRotateX(-90.0);	//Because OBJ Axis is 90º inverted...
		glRotateY(45.0);	
		glCallList((u32*)&Cube);
		glFlush();
		glPopMatrix(1);
	}
	
}