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
#include "loader.h"
#include "dmaTGDS.h"
#include "nds_cp15_misc.h"
#include "fileBrowse.h"
#include <stdio.h>
#include "biosTGDS.h"
#include "global_settings.h"
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"

#include "Texture_Cube.h"
#include "Texture_Cube_Exported.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "math.h"
#include "16bpp.h" 
#include "imagepcx.h"

float rotateX = 0.0;
float rotateY = 0.0;
float camDist = 0.3*4;
//3D Cube end

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

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
	printf("(Select): back to Loader. >%d", TGDSPrintfColor_Green);
	printf("Available heap memory: %d", getMaxRam());
	printf("Select: this menu");
}

//new: this bootcode will run from VRAM, once the NDSBinary context has been created and handled by the reload bootcode.
//generates a table of sectors out of a given file. It has the ARM7 binary and ARM9 binary
bool ReloadNDSBinaryFromContext(char * filename)  {
	
	return false;
}

int internalCodecType = SRC_NONE;//Internal because WAV raw decompressed buffers are used if Uncompressed WAV or AD
bool stopSoundStreamUser(){
	return false;
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];

int texID;

int main(int argc, char **argv)   {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console
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
	}
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	REG_IME = 0;
	MPUSet();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	
	menuShow();
	
	//gl start
	{
		InitGL();
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewport(0,0,255,191);	
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glGenTextures(1, &texID);
		glBindTexture(0, texID);
		glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, (u8*)_6bppBitmap);		
		
		//Direct Tex (PCX 128x128, using internal TGDS texture names buffer)
		//LoadGLSingleTextureAuto((u32)&Texture_Cube, (int *)&texID, texID);
		
		//Multiple 64x64 textures as BPM
		
		//Load 2 textures and map each one to a texture slot
		/*
		u32 arrayOfTextures[1];
		arrayOfTextures[0] = (u32)&Texture_Cube;
		int textureArrayNDS[2]; //0 : Cube tex / 1 : Cellphone tex
		int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (int*)&textureArrayNDS, 1);
		for(i = 0; i < texturesInSlot; i++){
			printf("tex: %d:textID[%d]", i, textureArrayNDS[i]);
		}
		*/

		glReset();
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ANTIALIAS);
		glEnable(GL_BLEND);
	}
	//gl end
	
	char * arg0 = (0x02280000 - (MAX_TGDSFILENAME_LENGTH+1));
	ReloadNDSBinaryFromContext(arg0);
	
	while (1){
		handleARM9SVC();	/* Do not remove, handles TGDS services */
		IRQWait(0, IRQ_VBLANK);
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
	
	glClearColor(255,255,255);		// White Background
	glClearDepth(0x7FFF);		// Depth Buffer Setup
	glEnable(GL_ANTIALIAS|GL_TEXTURE_2D|GL_BLEND); // Enable Texture Mapping + light #0 enabled per scene

	glDisable(GL_LIGHT0|GL_LIGHT1|GL_LIGHT2|GL_LIGHT3);

	setTGDSARM9PrintfCallback((printfARM9LibUtils_fn)&TGDSDefaultPrintf2DConsole); //Redirect to default TGDS printf Console implementation
	menuShow();
	
	REG_IE |= IRQ_VBLANK;
	glReset(); //Depend on GX stack to render scene
	glClearColor(0,35,195);		// blue green background colour

	/* TGDS 1.65 OpenGL 1.1 Initialization */
	ReSizeGLScene(255, 191);
	glMaterialShinnyness();

	//#1: Load a texture and map each one to a texture slot
	/*
	u32 arrayOfTextures[7];
	arrayOfTextures[0] = (u32)&apple; //0: apple.bmp  
	arrayOfTextures[1] = (u32)&boxbitmap; //1: boxbitmap.bmp  
	arrayOfTextures[2] = (u32)&brick; //2: brick.bmp  
	arrayOfTextures[3] = (u32)&grass; //3: grass.bmp
	arrayOfTextures[4] = (u32)&menu; //4: menu.bmp
	arrayOfTextures[5] = (u32)&snakegfx; //5: snakegfx.bmp
	arrayOfTextures[6] = (u32)&Texture_Cube; //6: Texture_Cube.bmp
	int texturesInSlot = LoadLotsOfGLTextures((u32*)&arrayOfTextures, (int*)&texturesSnakeGL, 7); //Implements both glBindTexture and glTexImage2D 
	int i = 0;
	for(i = 0; i < texturesInSlot; i++){
		printf("Texture loaded: %d:textID[%d] Size: %d", i, texturesSnakeGL[i], getTextureBaseFromTextureSlot(activeTexture));
	}
	*/
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
	#ifdef WIN32 //todo: does this work on NDS?
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