// t2k - Tatsuko Driver is a software library designed to drive game development.
// Copyright (C) Damako Soft since 2020, all rights reserved.
// current version is ver. 0.1.
//
// Damako Soft staff:
// 	Da: Daizo Sasaki
// 	Ma: yoshiMasa Sugawara
// 	Ko: Koji Saito
//
// If you are interested in t2k, please follow our Twitter account @DamakoSoft 
//
// These software come with absolutory no warranty and are released under the
// MIT License.  see https://opensource.org/licenses/MIT

#ifndef __T2K_SPRITE_H__
#define __T2K_SPRITE_H__

#include <t2kGCore.h>

enum SpriteType {
	kST_Invalid =0,
	kST_16colors=1,
};

struct T2K_SpriteInfo {
	uint8_t spriteType;
	uint8_t width,height;
	uint8_t numOfFrames;		// 1 means still image.
	int8_t centerX,centerY;		// offset of lfet and top in bitmap.
	int8_t numOfColors;			// 16 or 256
	int16_t transparentColor;	// -1 is no transparent color
	bool bitmap_needToFree;		// true if bitmap is created by malloc.
	bool palette_needToFree;	// true if palette is created by malloc.
};

struct T2K_Sprite16colors {
	T2K_SpriteInfo spriteInfo;	// piggy back
	uint8_t *bitmap;
	uint8_t *palette;
};

// pattern str info
//	notation:
//		 0: '_' transparent	(0 means color index)
//		 1: 'b' dark blue
// 		 2: 'r' dark red
//  	 3: 'm' dark magenta
//  	 4: 'g' dark green
//  	 5: 'c' dark cyan
//  	 6: 'y' dark yellow
//  	 7: 'w' dark white (=gray)
//		 8: 'k' black (=KURO)
//  	 9: 'B' blue
// 		10: 'R' red
// 		11: 'M' magenta
// 		12: 'G' green
// 		13: 'C' cyan
// 		14: 'Y' yellow
// 		15: 'W' white
//
// example:
// 		static const char *gBallSpritePattern={
//	    	"__RRRR__",
//		    "_RRRRRR_",
//		    "RRWRRRRR",
//		    "RRRRRRRR",
//		    "RRRRRRRr",
//		    "RRRRRRrr",
//		    "_RRRRrr_",
//		    "__rrrr__",
//		};

bool t2kInitSprite(T2K_Sprite16colors *outSprite,int inWidth,int inHeight,
				   const char *inPatternStrArray[],
				   int8_t inOriginX=0,int8_t inOriginY=0,
				   bool inUsePalette=false,uint8_t *inPaletteArea=NULL,
				   uint8_t *inBitmapArea=NULL);
bool t2kPutSprite(T2K_Sprite16colors *inSprite,int inX,int inY,
				  uint8_t *inCustomPalette=NULL);

void t2kSetDefault16colorPalette(uint8_t *outPalette);

bool t2kSprite_HasVisibleArea(T2K_SpriteInfo *inSpriteInfo,int inX,int inY,
							  int *outLeft,int *outTop,
							  int *outRight,int *outBottom);
#endif

