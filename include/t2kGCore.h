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

// This program is based on M5Stack_LCD_DMA by MHageGH and modified for game
// development. You can get the original MHageGH's M5Stack_LCD_DMA here.
// https://github.com/MhageGH/M5Stack_LCD_DMA
// We would like to thank Mr. MHageGH for making such a wonderful program
// available to the public. Thank you very much!

#ifndef __T2K_GCORE_H__
#define __T2K_GCORE_H__

const int kGRamWidth=160;
const int kGRamHeight=120;

// Frame Buffer Address
// 	x must be in [0,160)	note: t in [a,b) means a<=t<b
//	y must be in [0,120)
#define FBA(x,y) ((kGRamWidth*(y))+(x))

#define isValidXY(x,y) ((x)>=0 && (x)<kGRamWidth && 0<=(y) && (y)<kGRamHeight)
#define isInvalidXY(x,y) ((x)<0 || kGRamWidth<=(x) || (y)<0 || kGRamHeight<=(y))

// Make RGB332 value
// 	r must be in [0,7]		note: t in [a,b] means a<=t<=b
//	g must be in [0,7]
//	b must be in [0,3]
#define RGB(r,g,b) ( (((r)&0x07)<<5) | (((g)&0x07)<<2) | ((b) &0x03) )

enum {
	// basic 8 colors
	kBlack=RGB(0,0,0), kBlue=RGB(0,0,3), kRed	=RGB(7,0,0), kMagenta=RGB(7,0,3),
	kGreen=RGB(0,7,0), kCyan=RGB(0,7,3), kYellow=RGB(7,7,0), kWhite	 =RGB(7,7,3),
};

bool t2kGCoreInit();
void t2kGCoreStart();
uint8_t *t2kGetFramebuffer();
void t2kFlip();
void t2kFill(uint8_t inColorRGB332);
void t2kCopyFromFrontBuffer();
void t2kPSet(int inX,int inY,uint8_t inColorRGB332);
void t2kSetBrightness(uint8_t inBrightness);	// 255 is brightest.

#endif

