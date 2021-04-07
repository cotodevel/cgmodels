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

typedef struct PCXHeader
{
   char         manufacturer;   //should be 0
   char         version;        //should be 5
   char         encoding;       //should be 1
   char         bitsPerPixel; //should be 8
   short int    xmin,ymin;      //coordinates for top left,bottom right
   short int    xmax,ymax;
   short int    hres;           //resolution
   short int    vres;
   char         palette16[48];  //16 color palette if 16 color image
   char         reserved;       //ignore
   char         colorPlanes;   //ignore
   short int    bytesPerLine;
   short int    paletteYype;   //should be 2
   char         filler[58];     //ignore
}__attribute__ ((packed)) PCXHeader, *pPCXHeader;

//!	\brief holds a red green blue triplet
typedef struct RGB_24
{
	unsigned char r;	//!< 8 bits for the red value.
	unsigned char g;	//!< 8 bits for the green value.
	unsigned char b;	//!< 8 bits for the blue value.
} __attribute__ ((packed)) RGB_24;

//!	A generic image structure.
typedef struct sImage
{
	short height; 				/*!< \brief The height of the image in pixels */
	short width; 				/*!< \brief The width of the image in pixels */
	int bpp;					/*!< \brief Bits per pixel (should be 4 8 16 or 24) */
	unsigned short* palette;	/*!< \brief A pointer to the palette data */

	//! A union of data pointers to the pixel data.
	union
	{
		u8* data8;		//!< pointer to 8 bit data.
		u16* data16;	//!< pointer to 16 bit data.
		u32* data32;	//!< pointer to 32 bit data.
	} image;

} sImage, *psImage;


#ifdef __cplusplus
extern "C" {
#endif

extern int textureID;
extern void imageDestroy(sImage* img);
extern void image8to16(sImage* img);
extern int LoadGLTextures();
extern int loadPCX(const unsigned char* pcx, sImage* image);
extern char args[8][MAX_TGDSFILENAME_LENGTH];
extern char *argvs[8];
extern int TGDSProjectReturnFromLinkedModule();

extern int main(int argc, char **argv);
extern char curChosenBrowseFile[MAX_TGDSFILENAME_LENGTH+1];
extern bool fillNDSLoaderContext(char * filename);
extern struct FileClassList * thisFileList;

#ifdef __cplusplus
}
#endif

#endif
