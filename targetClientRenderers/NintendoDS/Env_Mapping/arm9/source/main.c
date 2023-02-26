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
#include "spitscTGDS.h"
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

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

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
	
	/////////////////////////////////////////////////////////Reload TGDS Proj///////////////////////////////////////////////////////////
	#if !defined(ISEMULATOR)
	char tmpName[256];
	char ext[256];	
	char TGDSProj[256];
	char curChosenBrowseFile[256];
	strcpy(TGDSProj,"0:/");
	strcat(TGDSProj, "ToolchainGenericDS-multiboot");
	if(__dsimode == true){
		strcat(TGDSProj, ".srl");
	}
	else{
		strcat(TGDSProj, ".nds");
	}
	//Force ARM7 reload once 
	if( 
		(argc < 3) 
		&& 
		(strncmp(argv[1], TGDSProj, strlen(TGDSProj)) != 0) 	
	){
		REG_IME = 0;
		MPUSet();
		REG_IME = 1;
		char startPath[MAX_TGDSFILENAME_LENGTH+1];
		strcpy(startPath,"/");
		strcpy(curChosenBrowseFile, TGDSProj);
		
		char thisTGDSProject[MAX_TGDSFILENAME_LENGTH+1];
		strcpy(thisTGDSProject, "0:/");
		strcat(thisTGDSProject, TGDSPROJECTNAME);
		if(__dsimode == true){
			strcat(thisTGDSProject, ".srl");
		}
		else{
			strcat(thisTGDSProject, ".nds");
		}
		
		//Boot .NDS file! (homebrew only)
		strcpy(tmpName, curChosenBrowseFile);
		separateExtension(tmpName, ext);
		strlwr(ext);
		
		//pass incoming launcher's ARGV0
		char arg0[256];
		int newArgc = 3;
		if (argc > 2) {
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			printf(" ---- test");
			
			//arg 0: original NDS caller
			//arg 1: this NDS binary
			//arg 2: this NDS binary's ARG0: filepath
			strcpy(arg0, (const char *)argv[2]);
			newArgc++;
		}
		//or else stub out an incoming arg0 for relaunched TGDS binary
		else {
			strcpy(arg0, (const char *)"0:/incomingCommand.bin");
			newArgc++;
		}
		//debug end
		
		char thisArgv[4][MAX_TGDSFILENAME_LENGTH];
		memset(thisArgv, 0, sizeof(thisArgv));
		strcpy(&thisArgv[0][0], thisTGDSProject);	//Arg0:	This Binary loaded
		strcpy(&thisArgv[1][0], curChosenBrowseFile);	//Arg1:	Chainload caller: TGDS-MB
		strcpy(&thisArgv[2][0], thisTGDSProject);	//Arg2:	NDS Binary reloaded through ChainLoad
		strcpy(&thisArgv[3][0], (char*)&arg0[0]);//Arg3: NDS Binary reloaded through ChainLoad's ARG0
		addARGV(newArgc, (char*)&thisArgv);				
		if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
			
		}
	}
	#endif
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	
	//gl start
	float camDist = 0.3*8;
	int rotateX = 0;
	int rotateY = 0;
	int cafe_texid;
	{
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewport(0,0,255,191,USERSPACE_TGDS_OGL_DL_POINTER);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset(USERSPACE_TGDS_OGL_DL_POINTER);
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		glGenTextures( 1, &cafe_texid );
		glBindTexture( 0, cafe_texid, USERSPACE_TGDS_OGL_DL_POINTER);
		glTexImage2D( 0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|TEXGEN_NORMAL, (u8*)cafe );
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION, USERSPACE_TGDS_OGL_DL_POINTER);
		glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);
		gluPerspective(30, 255.0 / 191.0, 0.1, 30, USERSPACE_TGDS_OGL_DL_POINTER);
		
	}
	//gl end
	
	menuShow();
	
	while(1) {
		
		//TEXGEN_NORMAL helpfully pops our normals into this matrix and uses the result as texcoords
		glMatrixMode(GL_TEXTURE,USERSPACE_TGDS_OGL_DL_POINTER);
		glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);
		GLvector tex_scale = { 64<<16, -64<<16, 1<<16 };
		glScalev( &tex_scale,USERSPACE_TGDS_OGL_DL_POINTER);		//scale normals up from (-1,1) range into texcoords
		glRotateXi(rotateX,USERSPACE_TGDS_OGL_DL_POINTER);		//rotate texture-matrix to match the camera
		glRotateYi(rotateY,USERSPACE_TGDS_OGL_DL_POINTER);
		
		glMatrixMode(GL_POSITION,USERSPACE_TGDS_OGL_DL_POINTER);
		glLoadIdentity(USERSPACE_TGDS_OGL_DL_POINTER);
		
		gluLookAt(	0.0, 0.0, camDist,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0,		//up
				USERSPACE_TGDS_OGL_DL_POINTER
		);
		
		glTranslatef32(0, 0, 0.0, USERSPACE_TGDS_OGL_DL_POINTER);
		glRotateXi(rotateX, USERSPACE_TGDS_OGL_DL_POINTER);
		glRotateYi(rotateY, USERSPACE_TGDS_OGL_DL_POINTER);

		glMaterialf(GL_EMISSION, RGB15(31,31,31), USERSPACE_TGDS_OGL_DL_POINTER);

		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_BACK, USERSPACE_TGDS_OGL_DL_POINTER);

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
		
		glBindTexture( 0, cafe_texid, USERSPACE_TGDS_OGL_DL_POINTER);
		glCallListGX((u32*)&Suzanne);
		
		glFlush(USERSPACE_TGDS_OGL_DL_POINTER);
		if(keys & KEY_START) break;
	}
}