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
#include "loader.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "keypadTGDS.h"
#include "GXPayload.h" //required to flush the GX<->DMA<->FIFO circuit on real hardware
#include "Texture_Cube_metal.h"
#include "Texture_Cube_wood.h"
#include "Texture_Cube_logo.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"
#include "16bpp.h" 
#include "imagepcx.h"
#include "Cube.h"
#include "loader.h"

//ARM7 VRAM core
#include "arm7vram.h"
#include "arm7vram_twl.h"

float rotateX = 0.0;
float rotateY = 0.0;
float camDist = 0.3*4;
char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

//OpenGL DL start
GLuint	box;				// Storage For The Box Display List
GLuint	top;				// Storage For The Top Display List
GLuint	xloop;				// Loop For X Axis
GLuint	yloop;				// Loop For Y Axis

GLfloat	xrot;				// Rotates Cube On The X Axis
GLfloat	yrot;				// Rotates Cube On The Y Axis

GLfloat boxcol[5][3]=
{
	{1.0f,0.0f,0.0f},{1.0f,0.5f,0.0f},{1.0f,1.0f,0.0f},{0.0f,1.0f,0.0f},{0.0f,1.0f,1.0f}
};

GLfloat topcol[5][3]=
{
	{.5f,0.0f,0.0f},{0.5f,0.25f,0.0f},{0.5f,0.5f,0.0f},{0.0f,0.5f,0.0f},{0.0f,0.5f,0.5f} 
};

float camMov = -1.0;
//OpenGL DL end

//true: pen touch
//false: no tsc activity
bool get_pen_delta( int *dx, int *dy ){
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

int isOpenGLDisplayList = 0;
//Back to loader, based on Whitelisted DLDI names
static char curLoaderNameFromDldiString[MAX_TGDSFILENAME_LENGTH+1];
static inline char * canGoBackToLoader(){
	char * dldiName = dldi_tryingInterface();
	if(dldiName != NULL){
		if(strcmp(dldiName, "R4iDSN") == 0){	//R4iGold loader
			strcpy(curLoaderNameFromDldiString, "0:/_DS_MENU.dat_ori");	//this allows to return to original payload code, and autoboot to TGDS-multiboot 
			return (char*)&curLoaderNameFromDldiString[0];
		}
		if(strcmp(dldiName, "Ninjapass X9 (SD Card)") == 0){	//Ninjapass X9SD
			strcpy(curLoaderNameFromDldiString, "0:/loader.nds");
			return (char*)&curLoaderNameFromDldiString[0];
		}
	}
	return NULL;
}

void menuShow(){
	clrscr();
	printf("                              ");
	printf(" TexturedBlenderModelNDSRender example:");
	printf("                              ");
	printf("Button (Start): File browser ");
	printf("    Button (A) Load TGDS/devkitARM NDS Binary. ");
	printf("                              ");
	printf("Available heap memory: %d", getMaxRam());
	
	printf("D-PAD Left/Right: toggle OpenGL DisplayList / >%d", TGDSPrintfColor_Yellow);
	printf("                     Blender 3D Model >%d", TGDSPrintfColor_Yellow);
	printf("Mode: ");
	
	if(isOpenGLDisplayList == 0){
		printf("     [OpenGL DisplayList: Metal]  >%d", TGDSPrintfColor_Cyan);	
	}
	else if(isOpenGLDisplayList == 1){
		printf("     [OpenGL DisplayList: ToolchainGenericDS]  >%d", TGDSPrintfColor_Cyan);	
	}
	else{
		printf("     [Blender 3D Model: Wooden]  >%d", TGDSPrintfColor_Yellow);
	}
}

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or AD
bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

int main(int argc, char **argv)   {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	//Save Stage 1: IWRAM ARM7 payload: NTR/TWL (0x03800000)
	memcpy((void *)TGDS_MB_V3_ARM7_STAGE1_ADDR, (const void *)0x02380000, (int)(96*1024));	//
	coherent_user_range_by_size((uint32)TGDS_MB_V3_ARM7_STAGE1_ADDR, (int)(96*1024)); //		also for TWL binaries 
	
	//Execute Stage 2: VRAM ARM7 payload: NTR/TWL (0x06000000)
	u32 * payload = NULL;
	if(__dsimode == false){
		payload = (u32*)&arm7vram[0];	
	}
	else{
		payload = (u32*)&arm7vram_twl[0];
	}
	executeARM7Payload((u32)0x02380000, 96*1024, payload);
	
	bool isTGDSCustomConsole = false;	//set default console or custom console
	GUI_init(isTGDSCustomConsole);

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();
	
	isTGDSCustomConsole = true;	//set default console or custom console
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

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
	char tmpName[256];
	char ext[256];
	if(__dsimode == true){
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
			(argc < 2) 
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
			int newArgc = 2;
			if (argc > 2) {
				//Arg0:	Chainload caller: TGDS-MB
				//Arg1:	This NDS Binary reloaded through ChainLoad
				//Arg2: This NDS Binary reloaded through ChainLoad's Argument0
				strcpy(arg0, (const char *)argv[2]);
				newArgc++;
			}
			
			char thisArgv[3][MAX_TGDSFILENAME_LENGTH];
			memset(thisArgv, 0, sizeof(thisArgv));
			strcpy(&thisArgv[0][0], curChosenBrowseFile);	//Arg0:	Chainload caller: TGDS-MB
			strcpy(&thisArgv[1][0], thisTGDSProject);	//Arg1:	NDS Binary reloaded through ChainLoad
			strcpy(&thisArgv[2][0], (char*)arg0);	//Arg2: NDS Binary reloaded through ChainLoad's ARG0
			addARGV(newArgc, (char*)&thisArgv);				
			if(TGDSMultibootRunNDSPayload(curChosenBrowseFile) == false){ //should never reach here, nor even return true. Should fail it returns false
				
			}
		}
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	REG_IME = 0;
	set0xFFFF0000FastMPUSettings();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	setupDisabledExceptionHandler();
	menuShow();
	InitGL();
	
	while (1){
		
		scanKeys();
		if(keysDown() & KEY_LEFT){
			isOpenGLDisplayList--;
			if(isOpenGLDisplayList < 0){
				isOpenGLDisplayList=0;
			}
			while(keysHeld() & KEY_LEFT){
				scanKeys();
				swiDelay(1);
			}
			menuShow();
		}
		if(keysDown() & KEY_RIGHT){
			isOpenGLDisplayList++;
			if(isOpenGLDisplayList > 2){
				isOpenGLDisplayList = 2;
			}
			while(keysHeld() & KEY_RIGHT){
				scanKeys();
				swiDelay(1);
			}
			menuShow();
		}
		
		
		//any floating point gl call is being converted to fixed prior to being implemented
		gluPerspective(30, 256.0 / 192.0, 0.1, 70);
		gluLookAt(	0.0, 0.0, camDist,		//camera possition 
					0.0, 0.0, 0.0,		//look at
					0.0, 1.0, 0.0);		//up
		glPushMatrix();

		//move it away from the camera
		glTranslate3f32(0, 0, floattof32(-1));
				
		glRotateX(rotateX);
		glRotateY(rotateY);
		
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();	
		glMatrixMode(GL_MODELVIEW);
		
		glMaterialShinnyness();
		updateGXLights(); //Update GX 3D light scene!
		
		if(isOpenGLDisplayList == 0){
			glBindTexture( 0, textureSizePixelCoords[Texture_MetalCubeID].textureIndex);
		}
		else if(isOpenGLDisplayList == 1){
			glBindTexture( 0, textureSizePixelCoords[Texture_LogoCubeID].textureIndex);
		}
		else {
			glBindTexture( 0, textureSizePixelCoords[Texture_WoodenCubeID].textureIndex);
		}

		GLfloat light_ambient[]  = { 1.0f, 1.0f, 1.0f, 1.0f };			
		//https://www.glprogramming.com/red/chapter05.html
		//The GL_DIFFUSE parameter probably most closely correlates with what you naturally think of as "the color of a light." 
		//It defines the RGBA color of the diffuse light that a particular light source adds to a scene. By default, GL_DIFFUSE is (1.0, 1.0, 1.0, 1.0) for GL_LIGHT0, 
		//which produces a bright, white light as shown in the left side of "Plate 13" in Appendix I. 
		//The default value for any other light (GL_LIGHT1, ... , GL_LIGHT7) is (0.0, 0.0, 0.0, 0.0).
		GLfloat light_diffuse[]  = { 31.0f, 31.0f, 31.0f, 0.0f };

		GLfloat light_specular[] = { 1.0f, 1.0f, xrot, 1.0f };
		GLfloat light_position[] = { 0.0f, -1.0f, 0.0f, 0.0f };
		glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient); //GX unused
		glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); //GX unused
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		
		{
			#ifdef ARM9
			GLfloat mat_ambient[]    = { 8.0f, 8.0f, 8.0f, 0.0f }; //NDS
			GLfloat mat_diffuse[]    = { 16.0f, 16.0f, 16.0f, 0.0f }; //NDS
			GLfloat mat_specular[]   = { 8.0f, 8.0f, 8.0f, 0.0f }; //NDS
			GLfloat mat_emission[]   = { 5.0f, 5.0f, 5.0f, 0.0f }; //NDS
			GLfloat high_shininess[] = { 128.0f }; //NDS
			#endif

			#ifdef WIN32
			GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f }; //WIN32
			GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f }; //WIN32
			GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //WIN32
			GLfloat mat_emission[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //WIN32
			GLfloat high_shininess[] = { 100.0f }; //WIN32
			#endif
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat_ambient); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION,  mat_emission);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, high_shininess);
		}
		GLfloat args[4];
		args[0] = (GLfloat)RGB15(31,31,31);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, (const GLfloat*)&args);	
		
		if(isOpenGLDisplayList == 2){
			glCallListGX((u32*)&Cube);
		}
		else{
			int pen_delta[2];
			bool isTSCActive = get_pen_delta( &pen_delta[0], &pen_delta[1] );
			if( isTSCActive == false ){
				printfCoords(0, 16, " No TSC Activity ----");
				rotateX = 0.0;
				rotateY = 0.0;
			}
			else{
				printfCoords(0, 16, "TSC Activity ----");
				rotateX = pen_delta[0];
				rotateY = pen_delta[1];
				if(yrot > 0){
					yrot-=rotateY;
				}
				else{
					yrot+=rotateY;
				}
				
				if(xrot > 0){
					xrot-=rotateX;
				}
				else{
					xrot+=rotateX;
				}
			}
			if (keysDown() & KEY_UP)
			{
				camMov-=2.8f;
			}
			if (keysDown() & KEY_DOWN)
			{
				camMov+=2.8f;
			}
			
			for (yloop=1;yloop<6;yloop++){
				for (xloop=0;xloop<yloop;xloop++){
					// Reset The View
					glLoadIdentity();
					glTranslatef(1.4f+(((float)xloop)*2.8f)-(((float)yloop)*1.4f),((6.0f-((float)yloop))*2.4f)-7.3f,-20.0f + camMov);
					
					glRotatef(45.0f-(2.0f*yloop)+xrot,1.0f,0.0f,0.0f);
					glRotatef(45.0f+yrot,0.0f,1.0f,0.0f);

					//Run the precompiled standard Open GL 1.1 Display List here
					glColor3fv(boxcol[yloop-1]);
					glCallList(box);
					glColor3fv(topcol[yloop-1]);
					glCallList(top);
				}
			}
		}
		
		glPopMatrix(1);
		rotateX += 0.3;
		rotateY += 0.3;
		
		//swap framebuffers
		#ifdef WIN32
		glutSwapBuffers();
		#endif

		#ifdef ARM9
		glFlush();
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQVBlankWait();
		#endif
	}
}

void InitGL(){
#ifdef _MSC_VER
	// initialise glut
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
	glutInitWindowSize(width, height);
    glutInitWindowPosition(100, 100);
	glutCreateWindow("TGDS Project through OpenGL (GLUT)");
	glutDisplayFunc(drawScene);
	glutReshapeFunc(ReSizeGLScene);
	glutKeyboardFunc(keyboard);
    glutSpecialFunc(keyboardSpecial);
	setVSync(true);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    float pos_light[4] = { 5.0f, 5.0f, 10.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, pos_light);
    glEnable(GL_LIGHT0);
	glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

#endif

#ifdef ARM9
	int TGDSOpenGLDisplayListWorkBufferSize = (256*1024);
	glInit(TGDSOpenGLDisplayListWorkBufferSize); //NDSDLUtils: Initializes a new videoGL context
	glClearDepth(0x7FFF);		// Depth Buffer Setup
	// Enable Texture Mapping + light #0 per scene
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_ANTIALIAS);
	glEnable(GL_BLEND);
	glDisable(GL_LIGHT0|GL_LIGHT1|GL_LIGHT2|GL_LIGHT3);
	glEnable(GL_LIGHT0);
	setTGDSARM9PrintfCallback((printfARM9LibUtils_fn)&TGDSDefaultPrintf2DConsole); //Redirect to default TGDS printf Console implementation
	REG_IE |= IRQ_VBLANK;
	glClearColor(0,35,195);		// blue green background colour
	setOrientation(ORIENTATION_0, true);
	
	//set mode 0, enable BG0 and set it to 3D
	SETDISPCNT_MAIN(MODE_0_3D);
	
	/* TGDS 1.65 OpenGL 1.1 Initialization */
	ReSizeGLScene(255, 191);
	
	//glGenTextures(1, &textureSizePixelCoords[Texture_CubeID].textureIndex);
	//glBindTexture(0, textureSizePixelCoords[Texture_CubeID].textureIndex);
	//glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, (u8*)_6bppBitmap);		
	
	//Multiple 64x64 textures as PCX/BMP
	//Load 2 textures and map each one to a texture slot
	u32 arrayOfTextures[3];
	arrayOfTextures[0] = (u32)&Texture_Cube_metal;
	arrayOfTextures[1] = (u32)&Texture_Cube_wood;
	arrayOfTextures[2] = (u32)&Texture_Cube_logo;
	int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (sizeof(arrayOfTextures)/sizeof(u32)) );
	int i = 0;
	for(i = 0; i < texturesInSlot; i++){
		printf("Tex. index: %d: Tex. name[%d]", i, getTextureNameFromIndex(i));
	}
	glCallListGX((u32*)&GXPayload); //Run this payload once to force cache flushes on DMA GXFIFO
#endif

	glEnable(GL_COLOR_MATERIAL);	//allow to mix both glColor3f + light sources when lighting is enabled (glVertex + glNormal3f)
	glDisable(GL_CULL_FACE); 
	glCullFace (GL_NONE);
	setupTGDSProjectOpenGLDisplayLists();
}

GLint DLEN2DTEX = -1;
GLint DLDIS2DTEX = -1;
GLint DLSOLIDCUBE05F = -1;

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
void setupTGDSProjectOpenGLDisplayLists(){
	// Generate Different Lists
	// 1: enable_2D_texture()
	DLEN2DTEX=(GLint)glGenLists(1);							
	glNewList(DLEN2DTEX,GL_COMPILE);						
	{
		GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f }; 
		GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f }; 
		GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f }; 
		GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f }; 

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	   
		glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); 
        glLightfv(GL_LIGHT0, GL_POSITION, light_position); 
		
		{
			#ifdef WIN32
			GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f }; //WIN32
			GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f }; //WIN32
			GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //WIN32
			GLfloat high_shininess[] = { 0.0f }; //WIN32
			#endif
			#ifdef ARM9
			GLfloat mat_ambient[]    = { 60.0f, 60.0f, 60.0f, 60.0f }; //NDS
			GLfloat mat_diffuse[]    = { 1.0f, 1.0f, 1.0f, 1.0f }; //NDS
			GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //NDS
			GLfloat high_shininess[] = { 128.0f }; //NDS
			#endif
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat_ambient);
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_diffuse); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, high_shininess);
		}
	}
	
	glEndList();
	DLDIS2DTEX=glGenLists(1);

	// 2: disable_2D_texture()
	glNewList(DLDIS2DTEX,GL_COMPILE);
	{
		GLfloat light_ambient[]  = { 0.0f, 0.0f, 0.0f, 1.0f };
        GLfloat light_diffuse[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        GLfloat light_position[] = { 2.0f, 5.0f, 5.0f, 0.0f };

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		
		glLightfv(GL_LIGHT0, GL_AMBIENT,  light_ambient); 
        glLightfv(GL_LIGHT0, GL_DIFFUSE,  light_diffuse); 
        glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular); 
        glLightfv(GL_LIGHT0, GL_POSITION, light_position); 

		
		{
			#ifdef ARM9
			GLfloat mat_ambient[]    = { 60.0f, 60.0f, 60.0f, 60.0f }; //NDS
			GLfloat mat_diffuse[]    = { 1.0f, 1.0f, 1.0f, 1.0f }; //NDS
			GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //NDS
			GLfloat high_shininess[] = { 128.0f }; //NDS
			#endif

			#ifdef WIN32
			GLfloat mat_ambient[]    = { 0.7f, 0.7f, 0.7f, 1.0f }; //WIN32
			GLfloat mat_diffuse[]    = { 0.8f, 0.8f, 0.8f, 1.0f }; //WIN32
			GLfloat mat_specular[]   = { 1.0f, 1.0f, 1.0f, 1.0f }; //WIN32
			GLfloat high_shininess[] = { 100.0f }; //WIN32
			#endif
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   mat_ambient); 
			glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_diffuse);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  mat_specular);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, high_shininess);
		}
	}
	glEndList();
	DLSOLIDCUBE05F=glGenLists(1);

	// 3: draw solid cube: 0.5f()
	glNewList(DLSOLIDCUBE05F, GL_COMPILE);
	{
		float size = 0.5f;
		GLfloat n[6][3] =
		{
			{-1.0f, 0.0f, 0.0f},
			{0.0f, 1.0f, 0.0f},
			{1.0f, 0.0f, 0.0f},
			{0.0f, -1.0f, 0.0f},
			{0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, -1.0f}
		};
		GLint faces[6][4] =
		{
			{0, 1, 2, 3},
			{3, 2, 6, 7},
			{7, 6, 5, 4},
			{4, 5, 1, 0},
			{5, 6, 2, 1},
			{7, 4, 0, 3}
		};
		GLfloat v[8][3];
		GLint i;

		v[0][0] = v[1][0] = v[2][0] = v[3][0] = -size / 2;
		v[4][0] = v[5][0] = v[6][0] = v[7][0] = size / 2;
		v[0][1] = v[1][1] = v[4][1] = v[5][1] = -size / 2;
		v[2][1] = v[3][1] = v[6][1] = v[7][1] = size / 2;
		v[0][2] = v[3][2] = v[4][2] = v[7][2] = -size / 2;
		v[1][2] = v[2][2] = v[5][2] = v[6][2] = size / 2;

		for (i = 5; i >= 0; i--)
		{
			glBegin(GL_QUADS);
				glNormal3fv(&n[i][0]);
				glTexCoord2f(0, 0);
				glVertex3fv(&v[faces[i][0]][0]);
				glTexCoord2f(1, 0);
				glVertex3fv(&v[faces[i][1]][0]);
				glTexCoord2f(1, 1);
				glVertex3fv(&v[faces[i][2]][0]);
				glTexCoord2f(0, 1);
				glVertex3fv(&v[faces[i][3]][0]);
			glEnd();
		}
	}
	glEndList();
	
	DLSPHERE = (GLint)glGenLists(1);
	//drawSphere(); -> NDS GX Implementation
	glNewList(DLSPHERE, GL_COMPILE); //recompile a light-based sphere as OpenGL DisplayList for rendering on upper screen later
	{
		float r=1; 
		int lats=8; 
		int longs=8;
		int i, j;
		for (i = 0; i <= lats; i++) {
			float lat0 = M_PI * (-0.5 + (float)(i - 1) / lats);
			float z0 = sin((float)lat0);
			float zr0 = cos((float)lat0);

			float lat1 = M_PI * (-0.5 + (float)i / lats);
			float z1 = sin((float)lat1);
			float zr1 = cos((float)lat1);
			glBegin(GL_TRIANGLE_STRIP);
			for (j = 0; j <= longs; j++) {
				float lng = 2 * M_PI * (float)(j - 1) / longs;
				float x = cos(lng);
				float y = sin(lng);
				glNormal3f(x * zr0, y * zr0, z0);
				glVertex3f(r * x * zr0, r * y * zr0, r * z0);
				glNormal3f(x * zr1, y * zr1, z1);
				glVertex3f(r * x * zr1, r * y * zr1, r * z1);
			}
			glEnd();
		}
	}
	glEndList();
	
	
	////////////////////////////OpenGL Display List init
	box=glGenLists(2);	// Generate 2 Different Lists
	glNewList(box,GL_COMPILE);							// Start With The Box List
		glBegin(GL_QUADS);
			// Bottom Face
			glNormal3f(0.5f,-1.0f, 0.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			// Front Face
			glNormal3f( 1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			// Back Face
			glNormal3f( 1.0f, 1.0f,-1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			// Right face
			glNormal3f( 1.0f, 0.0f, 0.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f( 1.0f, -1.0f,  1.0f);
			// Left Face
			glNormal3f(-1.0f, 0.0f, 1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f, -1.0f, -1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.0f, -1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();
	top=box+1;											// Storage For "Top" Is "Box" Plus One
	glNewList(top,GL_COMPILE); // Now The "Top" Display List
		glBegin(GL_QUADS);
			// Top Face
			glNormal3f( 1.0f, 1.0f, 1.0f);
			glTexCoord2f(0.0f, 1.0f); glVertex3f(-1.0f,  1.0f, -1.0f);
			glTexCoord2f(0.0f, 0.0f); glVertex3f(-1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 0.0f); glVertex3f( 1.0f,  1.0f,  1.0f);
			glTexCoord2f(1.0f, 1.0f); glVertex3f( 1.0f,  1.0f, -1.0f);
		glEnd();
	glEndList();
}

#ifdef ARM9
#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("O0")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__((optnone))
#endif
#endif
GLvoid ReSizeGLScene(GLsizei widthIn, GLsizei heightIn)		// resizes the window (GLUT & TGDS GL)
{
	#ifdef WIN32
	if (heightIn==0)										// Prevent A Divide By Zero By
	{
		heightIn=1;										// Making Height Equal One
	}
	glViewport(0,0,widthIn,heightIn);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)widthIn/(GLfloat)heightIn,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	#endif

	#ifdef ARM9
	if (heightIn==0)										// Prevent A Divide By Zero By
	{
		heightIn=1;										// Making Height Equal One
	}

	glViewport(0,0,widthIn,heightIn);						// Reset The Current Viewport

	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();									// Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f,(GLfloat)widthIn/(GLfloat)heightIn,0.1f,100.0f);

	glMatrixMode(GL_MODELVIEW);							// Select The Modelview Matrix
	glLoadIdentity();									// Reset The Modelview Matrix
	#endif
}