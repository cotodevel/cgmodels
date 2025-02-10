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

#ifndef __main9_h__
#define __main9_h__

#include "typedefsTGDS.h"
#include "dsregs.h"
#include "limitsTGDS.h"
#include "dldi.h"
#include "utilsTGDS.h"
#include "videoGL.h"
#include "TGDS_threads.h"

#define Texture_MetalCubeID ((int)0)
#define Texture_WoodenCubeID ((int)1)
#define Texture_LogoCubeID ((int)2)

#ifdef __cplusplus
extern "C" {
#endif

extern u32 * getTGDSMBV3ARM7Bootloader();
//TGDS Soundstreaming API
extern int internalCodecType;
extern struct fd * _FileHandleVideo; 
extern struct fd * _FileHandleAudio;
extern bool stopSoundStreamUser();
extern void closeSoundUser();
extern int internalCodecType;

extern int main(int argc, char **argv);
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool fillNDSLoaderContext(char * filename);
extern struct FileClassList * thisFileList;

extern float rotateX ;
extern float rotateY ;
extern float camDist ;

extern void InitGL();

extern GLint DLSPHERE;
extern GLint DLEN2DTEX;
extern GLint DLDIS2DTEX;
extern GLint DLSOLIDCUBE05F;
extern void setupTGDSProjectOpenGLDisplayLists();
extern GLvoid ReSizeGLScene(GLsizei widthIn, GLsizei heightIn);

extern int isOpenGLDisplayList;
extern bool get_pen_delta( int *dx, int *dy );

extern GLuint	box;				// Storage For The Box Display List
extern GLuint	top;				// Storage For The Top Display List
extern GLuint	xloop;				// Loop For X Axis
extern GLuint	yloop;				// Loop For Y Axis

extern GLfloat	xrot;				// Rotates Cube On The X Axis
extern GLfloat	yrot;				// Rotates Cube On The Y Axis

extern GLfloat boxcol[5][3];
extern GLfloat topcol[5][3];
extern float camMov;

extern struct task_Context * internalTGDSThreads;
extern void onThreadOverflowUserCode(u32 * args);

extern u32 getGDBStubUserProcess();
#ifdef __cplusplus
}
#endif

#endif
