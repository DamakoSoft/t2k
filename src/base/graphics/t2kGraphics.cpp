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

#include <M5Core2.h>

#include <t2kGCore.h>

#include <t2kGraphics.h>

#define sign(x) ((x)<0 ? -1 : (x)>0 ? +1 : 0 )

void t2kDrawLine(int inX1,int inY1,int inX2,int inY2,uint8_t inRGB332) {
	if((inX1<0 && inX2<0) || (kGRamWidth<=inX1 && kGRamWidth<=inX2)
	  || (inY1<0 && inY2<0) || (kGRamHeight<=inY1 && kGRamHeight<=inY2)) {
		return;
	}
	const int dx=inX2-inX1;
	const int dy=inY2-inY1;
	int x=inX1;
	int y=inY1;
	uint8_t *gram=t2kGetFramebuffer();
	const int absDx=abs(dx);
	const int absDy=abs(dy);
	const int signDx=sign(dx);
	const int signDy=sign(dy);
	int e,i;
	if(absDx>absDy) {
		const float deltaE=2*absDy;
		const float adjustE=2*absDx;
		for(i=0,e=-absDx; i<=absDx; i++) {
			if( isValidXY(x,y) ) { gram[FBA(x,y)]=inRGB332; }
			x+=signDx; e+=deltaE;
			if(e>=0) { y+=signDy; e-=adjustE; }
		}
	} else {
		const float deltaE=2*absDx;
		const float adjustE=2*absDy;
		for(i=0,e=-absDy; i<=absDy; i++) {
			if( isValidXY(x,y) ) { gram[FBA(x,y)]=inRGB332; }
			y+=signDy; e+=deltaE;
			if(e>=0) { x+=signDx; e-=adjustE; }
		}
	}
}

void t2kFillRect(int inX,int inY,int inWidth,int inHeight,uint8_t inRGB332) {
	if(kGRamWidth<=inX || kGRamHeight<=inY) { return; }
	const int right =min(inX+inWidth, kGRamWidth);
	const int bottom=min(inY+inHeight,kGRamHeight);
	if(right<0 || bottom<0) { return; }
	const int left=max(inX,0);
	const int top =max(inY,0);
	uint8_t *gram=t2kGetFramebuffer();
	for(int y=top; y<bottom; y++) {
		uint8_t *p=gram+FBA(left,y);
		for(int x=left; x<right; x++,p++) {
			*p=inRGB332;
		}
	}
}

void t2kDrawRect(int inX,int inY,int inWidth,int inHeight,uint8_t inRGB332) {
	if(kGRamWidth<=inX || kGRamHeight<=inY) { return; }
	const int right =min(inX+inWidth, kGRamWidth);
	const int bottom=min(inY+inHeight,kGRamHeight);
	if(right<0 || bottom<0) { return; }
	const int left=max(inX,0);
	const int top =max(inY,0);
	uint8_t *gram=t2kGetFramebuffer();
	uint8_t *p=gram+FBA(left,top);
	uint8_t *q=gram+FBA(left,bottom-1);
	for(int x=left; x<right; x++,p++,q++) { *p=*q=inRGB332; }
	for(int y=top+1; y<bottom-1; y++) {
		p=gram+FBA(left,y);
		*p=inRGB332;
		*(p+inWidth-1)=inRGB332;
	}
}

void t2kDrawCircle(int inX,int inY,int inR,uint8_t inRGB332) {
	if(inR<=0) { t2kPSet(inX,inY,inRGB332); return; }
	uint8_t *gram=t2kGetFramebuffer();
	float x=inR,y=0,e=1.0f/inR;
	do {
		x-=e*y; y+=e*x;
		int ix=(int)(x)+inX,iy=inY-(int)(y);
		if( isValidXY(ix,iy) ) { gram[FBA(ix,iy)]=inRGB332; }
	} while(fabs(x-inR)>.5f || fabs(y)>.5f);
}

