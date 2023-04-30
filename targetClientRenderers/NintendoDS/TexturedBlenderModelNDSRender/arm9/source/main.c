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
#include "spitscTGDS.h"

#include "Cube.h"	//Meshes Generated from Blender 2.49b
#include "Cellphone.h"
#include "Texture_Cellphone.h"	//Textures of it
#include "Texture_Cube.h"	
#include "Texture_Cube_Exported.h"	//exported from BMP24 into native Texture format

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
void menuShow(){
	clrscr();
	printf("                              ");
	printf(" TexturedBlenderModelNDSRender example ");
	printf(" Use the D-PAD to move the rendered polygon ");
	printf(" Polygon rendered and textured from Blender natively. ");
	
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

void ReSizeGLScene(int width, int height){		// Resize And Initialize The GL Window
	if (height==0)										// Prevent A Divide By Zero By
	{
		height=1;										// Making Height Equal One
	}

	glViewport(0, 0, width, height);						// Reset The Current Viewport
	
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(float)width/(float)height,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
}

//true: pen touch
//false: no tsc activity
static bool get_pen_delta( int *dx, int *dy ){
	static int prev_pen[2] = { 0x7FFFFFFF, 0x7FFFFFFF };
	
	// TSC Test.
	struct touchPosition touch;
	XYReadScrPosUser(&touch);
	
	if( (touch.px == 0) && (touch.py == 0) ){
		prev_pen[0] = prev_pen[1] = 0x7FFFFFFF;
		*dx = *dy = 0;
		return false;
	}
	else{
		if( prev_pen[0] != 0x7FFFFFFF ){
			*dx = (prev_pen[0] - touch.px);
			*dy = (prev_pen[1] - touch.py);
		}
		prev_pen[0] = touch.px;
		prev_pen[1] = touch.py;
	}
	return true;
}

#define NAN 0x80000000
#define BIAS 127
#define K 8
#define N 23
typedef unsigned float_bits;
/* Compute (int) f.
 * If conversion causes overflow or f is NaN, return 0x80000000
 */
static inline int float_f2i(float_bits f) {
  unsigned s = f >> (K + N);
  unsigned exp = f >> N & 0xFF;
  unsigned frac = f & 0x7FFFFF;
  
  /* Denormalized values round to 0 */
  if (exp == 0)
    return 0;
  /* f is NaN */
  if (exp == 0xFF)
    return NAN;
  /* Normalized values */
  int x;
  int E = exp - BIAS;
  /* Normalized value less than 0, return 0 */
  if (E < 0)
    return 0;
  /* Overflow condition */
  if (E > 30)
    return NAN;
  x = 1 << E;
  if (E < N)
    x |= frac >> (N - E);
  else
    x |= frac << (E - N);
  /* Negative values */
  if (s == 1)
    x = ~x + 1;
  return x;  
}

float boxMove = 0.0;

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
	
	menuShow();
	
	//gl start
		InitGL();
		float camDist = 0.3*4;
		float rotateX = 0.0;
		float rotateY = 0.0;
		
		float distX = 0.0;
		float distY = 0.0;
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewport(0,0,255,191);	
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset();
		
		//Direct Tex (PCX 128x128)
		LoadGLTextures((u32)&Texture_Cube);
		
		//Multiple 64x64 textures as BPM
		/*
		//Load 2 textures and map each one to a texture slot
		u32 arrayOfTextures[2];
		arrayOfTextures[0] = (u32)&Texture_Cube_Exported;
		arrayOfTextures[1] = (u32)&Texture_Cellphone;
		int textureArrayNDS[2]; //0 : Cube tex / 1 : Cellphone tex
		int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (int*)&textureArrayNDS, 2);
		for(i = 0; i < texturesInSlot; i++){
			printf("tex: %d:textID[%d]", i, textureArrayNDS[i]);
		}
		*/
		
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(35, 256.0 / 192.0, 0.1, 40);	
	//gl end
	//ReSizeGLScene(240, 160);
	
	int objects = 0;
	float lookat = 6.0;
	int xloop=0,yloop=0;
	int xrot=0,yrot=0;
	
	while(1) {		
		int pen_delta[2];
		bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
		distY -= (pen_delta[0]);
		distX -= (pen_delta[1] );
		
		lookat = -0.65;
		glPushMatrix();	
		glMatrixMode(GL_POSITION);
		
		//NDS GX server does not know this but OpenGL states:
		//Steps in the Initialization Process
			//Name the display list : GLuint glGenLists(GLsizei range)
			//Create the display list:
			
			//The Functions:
			//void glNewList(GLuint name, GLenum mode) specifies the start of list with index name
			//void glEndList() specifies the end
				//Modes:
				//GL_COMPILE or GL_COMPILE_AND_EXECUTE
				
			//Example: Create the display list
			  //glNewList(teapot, GL_COMPILE);
			  //glutSolidTeapot(0.2);
			  //glEndList();
		
		//Steps in the Rendering Process
			//Transform, set material properties, etc (Use the display list)
			//Finally, Execute the display list

		
		// Use the display list
		for (yloop=1;yloop<6;yloop++){
			if(yloop != 4){
				glBindTexture( 0, 0); //glBindTexture( 0, textureArrayNDS[1]);
				glLoadIdentity();							// Reset The View		
				gluLookAt(	-0.1, -0.1, lookat,		//camera possition 
				1.0, -distX, -distY,		//look at
				0.0, 1.0, 0.0		//up
				);
				
				GLfloat mat_emission[]   = { 31, 31, 31, 0 };
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);
				updateGXLights(); //Update GX 3D light scene!
				
				glTranslatef32(0, 0, 0.0);
				glRotateX(-90.0);	//Because OBJ Axis is 90� inverted...
				glRotateY(45.0);
				
				// Execute the display list
				glCallListGX((u32*)&Cellphone);
			}
			//render a Cube on the 4th object
			else{
				glBindTexture( 0, 0);	//glBindTexture( 0, textureArrayNDS[0]);
				glLoadIdentity();							// Reset The View		
				gluLookAt(	-0.9, -0.9, (0.9) + lookat,		//camera possition 
				-3.0, distX, distY,		//look at
				14.0, 7.0, -14.0	//up
				);
				
				GLfloat mat_emission[]   = { 31, 31, 31, 0 };
				glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);
				updateGXLights(); //Update GX 3D light scene!
				
				glTranslatef32(60.0, 60.0, 60.0);
				glRotateX(-boxMove);	//Because OBJ Axis is 90� inverted...
				glRotateY(boxMove);
				
				// Execute the display list
				glCallListGX((u32*)&Cube);
			}
			lookat+=0.3;
		}
		
		glFlush();
		glPopMatrix(1);
	}
}

int InitGL()										// All Setup For OpenGL Goes Here
{
	glInit(); //NDSDLUtils: Initializes a new videoGL context	
	glClearColor(255,255,255);		// White Background
	glClearDepth(0x7FFF);		// Depth Buffer Setup
	glEnable(GL_ANTIALIAS);
	glEnable(GL_TEXTURE_2D); // Enable Texture Mapping 
	glEnable(GL_BLEND);
}