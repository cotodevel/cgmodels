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

#ifdef __cplusplus
extern "C" {
#endif

extern u32 * getTGDSMBV3ARM7Bootloader();
extern int main(int argc, char **argv);
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool fillNDSLoaderContext(char * filename);
extern struct FileClassList * thisFileList;

//TGDS Soundstreaming API
extern int internalCodecType;
extern struct fd * _FileHandleVideo; 
extern struct fd * _FileHandleAudio;
extern void InitGL();

extern GLint DLSPHERE;
extern GLint DLEN2DTEX;
extern GLint DLDIS2DTEX;
extern GLint DLSOLIDCUBE05F;
extern void setupTGDSProjectOpenGLDisplayLists();
extern GLvoid ReSizeGLScene(GLsizei widthIn, GLsizei heightIn);

#ifdef __cplusplus
}
#endif

#endif
