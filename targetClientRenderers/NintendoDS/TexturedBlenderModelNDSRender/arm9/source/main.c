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

#include "Cube.h"	//Mesh Generated from Blender 2.49b
#include "Texture_Cube.h"	//Textures of it

int textureID=0;
//---------------------------------------------------------------------------------
void imageDestroy(sImage* img) {
//---------------------------------------------------------------------------------
	if(img->image.data8) TGDSARM9Free (img->image.data8);
	if(img->palette && img->bpp == 8) TGDSARM9Free (img->palette);
}

//---------------------------------------------------------------------------------
void image8to16(sImage* img) {
//---------------------------------------------------------------------------------
	int i;
	u16* temp = (u16*)TGDSARM9Malloc(img->height*img->width*2);

	for(i = 0; i < img->height * img->width; i++)
		temp[i] = img->palette[img->image.data8[i]] | (1<<15);

	TGDSARM9Free (img->image.data8);
	TGDSARM9Free (img->palette);

	img->palette = NULL;

	img->bpp = 16;
	img->image.data16 = temp;
}

//---------------------------------------------------------------------------------
int loadPCX(const unsigned char* pcx, sImage* image) {
//---------------------------------------------------------------------------------
	//struct rgb {unsigned char b,g,r;};
	RGB_24* pal;
	
	PCXHeader* hdr = (PCXHeader*) pcx;

	pcx += sizeof(PCXHeader);
	
	unsigned char c;
	int size;
	int count;
	int run;
	int i;
	int iy;
	int width, height;
	int scansize = hdr->bytesPerLine;
	unsigned char *scanline;


	width = image->width  = hdr->xmax - hdr->xmin + 1 ;
	height = image->height = hdr->ymax - hdr->ymin + 1;

	size = image->width * image->height;

	if(hdr->bitsPerPixel != 8)
		return 0;
	
	scanline = image->image.data8 = (unsigned char*)TGDSARM9Malloc(size);
	image->palette = (unsigned short*)TGDSARM9Malloc(256 * 2);

	count = 0;

	for(iy = 0; iy < height; iy++) {
		count = 0;
		while(count < scansize)
		{
			c = *pcx++;
			
			if(c < 192) {
				scanline[count++] = c;
			} else {
				run = c - 192;
			
				c = *pcx++;
				
				for(i = 0; i < run && count < scansize; i++)
					scanline[count++] = c;
			}
		}
		scanline += width;
	}

	//check for the palette marker.
	//I have seen PCX files without this, but the docs don't seem ambiguous--it must be here.
	//Anyway, the support among other apps is poor, so we're going to reject it.
	if(*pcx != 0x0C)
	{
		TGDSARM9Free(image->image.data8);
		image->image.data8 = 0;
		TGDSARM9Free(image->palette);
		image->palette = 0;
		return 0;
	}

	pcx++;

	pal = (RGB_24*)(pcx);

	image->bpp = 8;

	for(i = 0; i < 256; i++)
	{
		u8 r = (pal[i].r + 4 > 255) ? 255 : (pal[i].r + 4);
		u8 g = (pal[i].g + 4 > 255) ? 255 : (pal[i].g + 4);
		u8 b = (pal[i].b + 4 > 255) ? 255 : (pal[i].b + 4);
		image->palette[i] = RGB15(r >> 3 , g >> 3 , b >> 3) ;
	}
	return 1;
}

int LoadGLTextures()									// Load PCX files And Convert To Textures
{
	sImage pcx;

	//load our texture
	loadPCX((u8*)Texture_Cube, &pcx);
	image8to16(&pcx);

	//DS supports no filtering of anykind so no need for more than one texture
	glGenTextures(1, &textureID);
	glBindTexture(0, textureID);
	glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD, pcx.image.data8);

	imageDestroy(&pcx);

	return 0;
}

char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];

void menuShow(){
	clrscr();
	printf("                              ");
	printf(" Env Mapping example ");
	printf(" Use the D-PAD to move the rendered polygon ");
	
	printf("Available heap memory: %d", getMaxRam());
	printf("Button (Select): this menu. ");
	printarm7DebugBuffer();
}

bool stopSoundStreamUser(){
	
}

void closeSoundUser(){
	//Stubbed. Gets called when closing an audiostream of a custom audio decoder
}

//ToolchainGenericDS-LinkedModule User implementation: Called if TGDS-LinkedModule fails to reload ARM9.bin from DLDI.
char args[8][MAX_TGDSFILENAME_LENGTH];
char *argvs[8];
int TGDSProjectReturnFromLinkedModule() __attribute__ ((optnone)) {
	return -1;
}

int main(int argc, char **argv) __attribute__ ((optnone)) {
	
	/*			TGDS 1.6 Standard ARM9 Init code start	*/
	bool isTGDSCustomConsole = true;	//set default console or custom console: true
	GUI_init(isTGDSCustomConsole);
	GUI_clear();
	
	bool isCustomTGDSMalloc = true;
	setTGDSMemoryAllocator(getProjectSpecificMemoryAllocatorSetup(TGDS_ARM7_MALLOCSTART, TGDS_ARM7_MALLOCSIZE, isCustomTGDSMalloc));
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
	float camDist = 0.3*4;
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
		
		LoadGLTextures();
		
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
		
		glBindTexture( 0, textureID );
		glCallList((u32*)&Cube);
		glFlush();
		glPopMatrix(1);
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