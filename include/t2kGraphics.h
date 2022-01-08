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

#ifndef __T2K_GRAPHICS_H__
#define __T2K_GRAPHICS_H__

void t2kDrawLine(int inX1,int inY1,int inX2,int inY2,uint8_t inRGB332);

void t2kDrawRect(int inX,int inY,int inWidth,int inHeight,uint8_t inRGB332);
void t2kFillRect(int inX,int inY,int inWidth,int inHeight,uint8_t inRGB332);

void t2kDrawCircle(int inX,int inY,int inR,uint8_t inRGB332);

#endif

