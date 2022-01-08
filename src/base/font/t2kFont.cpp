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

#include <t2kCommon.h>

#include <stdarg.h>

#include <t2kGCore.h>
#include <t2kFont.h>

static const Font *gFont=NULL;
static uint8_t gSquarePattern[8]={
	0xFF,0x81,0x81,0x81, 0x81,0x81,0x81,0xFF
};

bool t2kFontInit(const Font *inFont) {
	gFont=inFont;
	return true;
}

void t2kPutChar(int inX,int inY,uint8_t inRGB332,char inChar) {
	int charIndex = inChar<' ' || 0x7F<inChar ? 0x7F-' ' : inChar-' ';
	const uint8_t *pattern = gFont==NULL ? gSquarePattern : gFont[charIndex].pattern;
	uint8_t *gram=t2kGetFramebuffer();
	int ye=inY+8;
	int xe=inX+8;
	for(int y=inY,i=0; y<ye; y++,i++) {
		if(y<0 || 120<=y) { continue; }
		uint8_t mask=0x80;
		uint8_t dots=pattern[i];
		for(int x=inX; x<xe; x++,mask=mask>>1) {
			if(x<0 || 160<=x) { continue; }
			if((dots & mask)==0) { continue; }
			gram[FBA(x,y)]=inRGB332;
		}
	}
}

void t2kPutStr(int inX,int inY,uint8_t inRGB322,const char *inString) {
	int x=inX;
	for(int i=0; inString[i]!='\0'; i++,x+=8) {
		t2kPutChar(x,inY,inRGB322,inString[i]);
	}
}

static char gPrintfBuffer[256];
void t2kPrintf(int inX,int inY,uint8_t inRGB322,const char *inFormat,...) {
	va_list arg;
    va_start(arg,inFormat);
    vsprintf(gPrintfBuffer,inFormat,arg);
    va_end(arg);
	t2kPutStr(inX,inY,inRGB322,gPrintfBuffer);
}

void t2kPrintf(int inY,uint8_t inRGB322,const char *inFormat,...) {
	va_list arg;
    va_start(arg,inFormat);
    vsprintf(gPrintfBuffer,inFormat,arg);
    va_end(arg);

	int len=strlen(gPrintfBuffer);
	int x=(kGRamWidth-len*8)/2;
	t2kPutStr(x,inY,inRGB322,gPrintfBuffer);
}

void t2kDrawFontPattern(int inX,int inY,uint8_t inRGB332,const uint8_t *inPattern) {
	uint8_t *gram=t2kGetFramebuffer();
	int ye=inY+8;
	int xe=inX+8;
	for(int y=inY,i=0; y<ye; y++,i++) {
		if(y<0 || 120<=y) { continue; }
		uint8_t mask=0x80;
		uint8_t dots=inPattern[i];
		for(int x=inX; x<xe; x++,mask=mask>>1) {
			if(x<0 || 160<=x) { continue; }
			if((dots & mask)==0) { continue; }
			gram[FBA(x,y)]=inRGB332;
		}
	}
}

