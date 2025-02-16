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
#include <math.h>
#include "posixHandleTGDS.h"
#include "TGDSMemoryAllocator.h"
#include "videoGL.h"
#include "videoTGDS.h"
#include "Suzanne.h"
#include "cafe.h"
#include "loader.h"

//ARM7 VRAM core
#include "arm7vram.h"
#include "arm7vram_twl.h"

u32 * getTGDSMBV3ARM7Bootloader(){
	if(__dsimode == false){
		return (u32*)&arm7vram[0];	
	}
	else{
		return (u32*)&arm7vram_twl[0];
	}
}

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

#if (defined(__GNUC__) && !defined(__clang__))
__attribute__((optimize("Os")))
#endif
#if (!defined(__GNUC__) && defined(__clang__))
__attribute__ ((optnone))
#endif
int main(int argc, char **argv) {
	
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
	
	bool isTGDSCustomConsole = true;	//set default console or custom console: true
	GUI_init(isTGDSCustomConsole);
	GUI_clear();

	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(isCustomTGDSMalloc));
	sint32 fwlanguage = (sint32)getLanguage();

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
	
	REG_IME = 0;
	set0xFFFF0000FastMPUSettings();
	//TGDS-Projects -> legacy NTR TSC compatibility
	if(__dsimode == true){
		TWLSetTouchscreenTWLMode();
	}
	REG_IME = 1;
	
	setupDisabledExceptionHandler();
	
	//gl start
	InitGL();
	float camDist = 0.3*12;
	int rotateX = 0;
	int rotateY = 0;
	int cafe_texid;
	{
		setOrientation(ORIENTATION_0, true);
		
		//set mode 0, enable BG0 and set it to 3D
		SETDISPCNT_MAIN(MODE_0_3D);
		
		//this should work the same as the normal gl call
		glViewport(0,0,255,191);
		
		glClearColor(0,0,0);
		glClearDepth(0x7FFF);
		
		glReset();
		glEnable(GL_ANTIALIAS);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		
		glGenTextures( 1, &cafe_texid );
		glBindTexture( 0, cafe_texid);
		glTexImage2D( 0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T|TEXGEN_NORMAL, (u8*)cafe );
		
		//any floating point gl call is being converted to fixed prior to being implemented
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(30, 255.0 / 191.0, 0.1, 30);
		
	}
	//gl end
	
	menuShow();
	
	while(1) {
		
		//TEXGEN_NORMAL helpfully pops our normals into this matrix and uses the result as texcoords
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		
		GLvector tex_scale = { 64<<16, -64<<16, 1<<16 };
		glScalev( &tex_scale);		//scale normals up from (-1,1) range into texcoords
		
		glRotateXi(rotateX);		//rotate texture-matrix to match the camera
		glRotateYi(rotateY);
		
		glMatrixMode(GL_POSITION);
		glLoadIdentity();
		
		gluLookAt(	0.5, 0.5, camDist,		//camera possition 
				0.0, 0.0, 0.0,		//look at
				0.0, 1.0, 0.0		//up
		);
		
		glTranslatef32(0, 0, 0);
		glRotateXi(rotateX);
		glRotateYi(rotateY);
		GLfloat mat_emission[]   = { 31, 31, 31, 0 };
		glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, mat_emission);
		updateGXLights(); //Update GX 3D light scene!
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
		
		glBindTexture( 0, cafe_texid);
		glCallListGX((u32*)&Suzanne);
		
		glFlush();
		if(keys & KEY_START) break;
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
	
	DLSPHERE=glGenLists(1);
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