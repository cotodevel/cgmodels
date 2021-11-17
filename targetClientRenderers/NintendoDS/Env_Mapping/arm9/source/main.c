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

#include "Suzanne.h"
#include "cafe.h"

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

void menuShow(){
	clrscr();
	printf("                              ");
	printf(" Env Mapping example ");
	printf(" Use the D-PAD to move the rendered polygon ");
	
	printf("Available heap memory: %d", getMaxRam());
	printf("Button (Select): this menu. ");
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

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int TGDSProjectReturnFromLinkedModule()  {
	return -1;
}

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif

#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: true
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	//xmalloc init removes args, so save them
	int i = 0;
	for(i = 0; i < argc; i++){
		argvs[i] = argv[i];
	}

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc, TGDSDLDI_ARM7_ADDRESS));
	sint32 fwlanguage = (sint32)getLanguage();
	
	//argv destroyed here because of xmalloc init, thus restore them
	for(i = 0; i < argc; i++){
		argv[i] = argvs[i];
	}

	int ret=FS_init();
	if (ret == 0)
	{
		printf("FS Init ok.");
	}
	else{
		printf("FS Init error: %d", ret);
	}
	
	asm("mcr	p15, 0, r0, c7, c10, 4");
	flush_icache_all();
	flush_dcache_all();
	/*			TGDS 1.6 Standard ARM9 Init code end	*/
	
	//gl start
	float camDist = 0.3*4;
	int rotateX = 0;
	int rotateY = 0;
	int i;
	int cafe_texid;
	{
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewPort(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset();
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		glGenTextures( 1, &cafe_texid );
		glBindTexture( 0, cafe_texid );
		glTexImage2D( 0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|TEXGEN_NORMAL, (u8*)cafe );
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(30, 255.0 / 192.0, 0.1, 30);
		
	}
	//gl end
	
	menuShow();
	
	while(1) {
		
		//TEXGEN_NORMAL helpfully pops our normals into this matrix and uses the result as texcoords
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		GLvector tex_scale = { 64<<16, -64<<16, 1<<16 };
		glScalev( &tex_scale );		//scale normals up from (-1,1) range into texcoords
		glRotateXi(rotateX);		//rotate texture-matrix to match the camera
		glRotateYi(rotateY);
		
		glMatrixMode(GL_POSITION);
		glLoadIdentity();
		
		gluLookAt(	0.0, 0.0, camDist,		//camera possition 
				0.0, 0.0, 0.0,		//look at
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
		
		//int pen_delta[2];
		//get_pen_delta( &pen_delta[0], &pen_delta[1] );
		//rotateY -= pen_delta[0];
		//rotateX -= pen_delta[1];
		
		glBindTexture( 0, cafe_texid );
		glCallList((u32*)&Suzanne);
		
		glFlush();
		if(keys & KEY_START) break;
	}
	
	/*
	while (1){
		scanKeys();
		if (keysDown() & KEY_SELECT){
			menuShow();
			scanKeys();
			while(keysDown() & KEY_SELECT){
				scanKeys();
			}
		}
		
		if (keysDown() & KEY_START){
			menuShow();
			scanKeys();
			while(keysDown() & KEY_START){
				scanKeys();
			}
		}
	}
	*/
}