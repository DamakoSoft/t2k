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

#include "t2kCommon.h"

#include "t2kSprite.h"

static uint8_t gGlobal16colorPalette[16]={
	RGB(0,0,0),	//  0: '_' transparent
	RGB(0,0,1),	//  1: 'b' dark blue
	RGB(3,0,0),	//  2: 'r' dark red
	RGB(3,0,1),	//  3: 'm' dark magenta
	RGB(0,3,0),	//  4: 'g' dark green
	RGB(0,3,1),	//  5: 'c' dark cyan
	RGB(3,3,0),	//  6: 'y' dark yellow
	RGB(3,3,2),	//  7: 'w' dark white (=gray)
	RGB(0,0,0),	//	8: 'k' black (=KURO)
	RGB(0,0,3),	//  9: 'B' blue
	RGB(7,0,0),	// 10: 'R' red
	RGB(7,0,3),	// 11: 'M' magenta
	RGB(0,7,0),	// 12: 'G' green
	RGB(0,7,3),	// 13: 'C' cyan
	RGB(7,7,0),	// 14: 'Y' yellow
	RGB(7,7,3),	// 15: 'W' white
};

bool t2kInitSprite(T2K_Sprite16colors *outSprite,
				   int inWidth,int inHeight,
				   const char *inPatternStrArray[],
				   int8_t inOriginX,int8_t inOriginY,
				   bool inUsePalette,uint8_t *inPaletteArea,
				   uint8_t *inBitmapArea) {
	outSprite->spriteInfo.spriteType=kST_16colors;
	outSprite->spriteInfo.width =inWidth;
	outSprite->spriteInfo.height=inHeight;
	outSprite->spriteInfo.numOfFrames=1;
	outSprite->spriteInfo.centerX=inOriginX;
	outSprite->spriteInfo.centerY=inOriginY;
	outSprite->spriteInfo.numOfColors=16;
	outSprite->spriteInfo.transparentColor=0;
	if( inUsePalette ) {
	   	if(inPaletteArea==NULL) {
			outSprite->palette=(uint8_t *)malloc(sizeof(uint8_t)*16);
			if(outSprite->palette==NULL) {
				ERROR("ERROR t2kInitSprite(T2K_Sprite16colors): "
					  "not enough memory for palette area.\n");
				return false;
			}
			outSprite->spriteInfo.palette_needToFree=true;
		} else {
			outSprite->palette=inPaletteArea;
			outSprite->spriteInfo.palette_needToFree=false;
		}
	} else {
		outSprite->palette=gGlobal16colorPalette;
		outSprite->spriteInfo.palette_needToFree=false;
	}
	if(inBitmapArea==NULL) {
		outSprite->bitmap=(uint8_t *)malloc(sizeof(uint8_t)*inWidth*inHeight);
		if(outSprite->bitmap==NULL) {
			ERROR("ERROR t2kInitSprite(T2K_Sprite16colors): "
				  "not enough memory for bitmap area.\n");
			return false;
		}
		outSprite->spriteInfo.palette_needToFree=true;
	}

	uint8_t *dest=outSprite->bitmap;
	const char *s=inPatternStrArray[0];
	uint8_t t;
	for(int y=0; y<inHeight; y++,dest+=inWidth,s=inPatternStrArray[y]) {
		for(int x=0; x<inWidth; x++) {
			switch(s[x]) {
				case '_': t= 0; break;
				case 'b': t= 1; break;
				case 'r': t= 2; break;
				case 'm': t= 3; break;
				case 'g': t= 4; break;
				case 'c': t= 5; break;
				case 'y': t= 6; break;
				case 'w': t= 7; break;
				case 'k': t= 8; break;
				case 'B': t= 9; break;
				case 'R': t=10; break;
				case 'M': t=11; break;
				case 'G': t=12; break;
				case 'C': t=13; break;
				case 'Y': t=14; break;
				case 'W': t=15; break;
				default:
					ERROR("ERROR t2kInitSprit(T2K_Sprite16colors): "
						  "invalid color command '%c' at (%d,%d).\n",s[x],x,y);
					return false;
			}
			dest[x]=t;
		}
	}
	return true;
}

// draw it so that the center of the sprite (centerX,centerY) is at the position
// of the screen (inX,inY).
bool t2kPutSprite(T2K_Sprite16colors *inSprite,int inX,int inY,
				  uint8_t *inCustomPalette) {
	if(inSprite->bitmap==NULL
	   || (inSprite->palette==NULL && inCustomPalette==NULL)) {
		// illegal sprite data.
		return false;
	}
	int left,top,right,bottom;
	if(t2kSprite_HasVisibleArea(&inSprite->spriteInfo,inX,inY,
								&left,&top,&right,&bottom)==false) {
		return true;	// no pixels to draw.
	}
	const int w=inSprite->spriteInfo.width;
	const int h=inSprite->spriteInfo.height;
	uint8_t *palette = inCustomPalette!=NULL ? inCustomPalette : inSprite->palette;
	const int transparentColorIndex=inSprite->spriteInfo.transparentColor;

	// NOTE: (u,v) in bitmap
	// u in [startU,endU)
	int startU = left<0 ? -left : 0;
	int endU   = right>kGRamWidth ? w-(right-kGRamWidth) : w;
	// v in [startV,endV)
	int startV = top<0 ? -top : 0;
	int endV   = bottom>kGRamHeight ? h-(bottom-kGRamHeight) : h;

	// NOTE: (s,t) in GRAM
	// s in [0,kGRamWidth)
	int startS = left<0 ? 0 : left;
	int endS   = kGRamWidth<right ? kGRamWidth : right;
	// t in [0,kGRamHeight)
	int startT = top<0 ? 0 : top;
	int endT   = kGRamHeight<bottom ? kGRamHeight :  bottom;

	uint8_t *srcScanline=inSprite->bitmap+startV*w+startU;
	uint8_t *dstScanline=t2kGetFramebuffer()+FBA(0,startT);
	for(int t=startT,v=startV; v<endV && t<endT;
			t++,v++,dstScanline+=kGRamWidth,srcScanline+=w) {
		for(int s=startS,u=startU; u<endU && s<endS; s++,u++) {
			uint8_t indexColor=srcScanline[u];
			if(indexColor>=16) {
				// illegal index color
				return false;
			}
			if(indexColor==transparentColorIndex) { continue; }
			dstScanline[s]=palette[indexColor];
		}
	}
	return true;
}

bool t2kSprite_HasVisibleArea(T2K_SpriteInfo *inSpriteInfo,int inX,int inY,
							  int *outLeft,int *outTop,
							  int *outRight,int *outBottom) {
	// sprite area is [left,right)x[top,bottom)
	int left=inX-inSpriteInfo->centerX;
	int top =inY-inSpriteInfo->centerY;
	int right =left+inSpriteInfo->width;
	int bottom=top +inSpriteInfo->height;
	if(right<=0 || kGRamWidth<=left || bottom<=0 || kGRamHeight<=top) {
		return false;
	}
	if(outLeft!=NULL) { *outLeft=left; }
	if(outTop !=NULL) { *outTop =top;  }
	if(outRight !=NULL) { *outRight =right;  }
	if(outBottom!=NULL) { *outBottom=bottom; }
	return true;
}

void t2kSetDefault16colorPalette(uint8_t *outPalette) {
	memcpy(outPalette,gGlobal16colorPalette,16);
}

