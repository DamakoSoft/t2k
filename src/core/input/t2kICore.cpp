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

#include <t2kICore.h>

const uint8_t kNativeButtonA_GPIO=39;
const uint8_t kNativeButtonB_GPIO=38;
const uint8_t kNativeButtonC_GPIO=37;

const uint8_t kDamakoHW_UP_GPIO   =3;
const uint8_t kDamakoHW_DOWN_GPIO =1;
const uint8_t kDamakoHW_LEFT_GPIO =16;
const uint8_t kDamakoHW_RIGHT_GPIO=17;
const uint8_t kDamakoHW_A_GPIO    =2;
const uint8_t kDamakoHW_B_GPIO    =5;
const uint8_t kDamakoHW_SELECT_GPIO=25;
const uint8_t kDamakoHW_START_GPIO =26;

const int kKeyboardI2CAddr=0x08;
// const uint8_t kKeyboardInt=5;
const uint8_t kKeyboardInt=33;	// for Core2

const uint8_t kAllGameBoyFacesButtonReleased=0xFF;
enum GbfButtonMask {
	GBF_MASK_START	=0x80,
	GBF_MASK_SELECT	=0x40,
	GBF_MASK_B		=0x20,
	GBF_MASK_A		=0x10,
	GBF_MASK_RIGHT	=0x08,
	GBF_MASK_LEFT	=0x04,
	GBF_MASK_DOWN	=0x02,
	GBF_MASK_UP		=0x01,
};

static bool gLastStatusNativeA=false;	// true if pressed
static bool gLastStatusNativeB=false;
static bool gLastStatusNativeC=false;

// default input device is a GameBoy Faces.
static uint8_t gLastStatus=kAllGameBoyFacesButtonReleased;

static bool gCurrentStatusNativeA=false;	// true if pressed
static bool gCurrentStatusNativeB=false;
static bool gCurrentStatusNativeC=false;
static uint8_t gCurrentStatus=kAllGameBoyFacesButtonReleased;

static uint8_t gUseCustomHW=0x00;	// default is no custom HW.
const uint8_t kAllCustomHwButtonReleased=0xFF;
static uint8_t gLastStatusCustomHW   =kAllCustomHwButtonReleased;
static uint8_t gCurrentStatusCustomHW=kAllCustomHwButtonReleased;
static int gCustomHwGpioPin[8]={
	-1,	// start
	-1,	// select
	-1,	// A
	-1,	// B
	-1,	// right
	-1, // left
	-1,	// down
	-1,	// up
};
static bool gCustomHwInputType[8];	// false is inactive.

const uint8_t kAllCustomCKBButtonReleased=0xFF;
static const int kCardKeyboardI2CAddr=0x5F;//by dzonesasaki 201122
static uint8_t gCurrentStatusCardKey =  kAllCustomCKBButtonReleased;
static uint8_t gLastStatusCardKey =  kAllCustomCKBButtonReleased;
static uint8_t gUseCustomCKB =0;
static uint8_t gCustomCKBI2CVal[8]={255,255,255,255,255,255,255,255};
static bool gCustomCKVInputType[8];
// static uint8_t gui8KeyRampingRelease[8]={0,0,0,0,0,0,0,0};
const uint8_t kWeightCKBRamping[8]={128,128,4,4,4,4,4,4};

bool t2kICoreInit() {
	// DO NOT CALL BELOW CODE
	// Wire.begin(32,33);	// for Core2 Grove

	pinMode(kKeyboardInt,INPUT_PULLUP);
	return true;
}

T2K_StartMode t2kCheckStartMode(const int inNumOfTry) {
	// wait 1 sec to allow time to press the start / select button.
	Serial.println("-------------------------------------");
	Serial.print("detect start mode ");
	for(int i=0; i<20; i++) {
		Serial.print(".");
		t2kInputUpdate();
// Serial.printf("gCurrentStatus=%X\n",gCurrentStatus);
		delay(100);
	}
	Serial.println();

	// check the button.
	Serial.println("START CHECK");
	for(int i=0; i<inNumOfTry; i++) {
		t2kInputUpdate();
		Serial.printf("CHECK gCurrentStatus=%X\n",gCurrentStatus);
		if( t2kIsPressedStart() ) {
			Serial.println("DETECT START BUTTON");
			delay(100);
			return T2K_StartMode::kUseDefaultWiFi;
		} else if( t2kIsPressedSelect() ) {
			Serial.println("DETECT SELECT BUTTON");
			delay(100);
			return T2K_StartMode::kSelectWiFi;;
		}
		delay(100);
	}
	Serial.println("DETECT START GAME");
	return T2K_StartMode::kStartGame;;
}

bool t2kRegisterCutomHW(T2K_CustomHwType inType,int inGpioPinNo,bool inOnType) {
	int index=-1;
	switch(inType) {
		case T2K_CUSTOM_HW_START:	index=0;	break;
		case T2K_CUSTOM_HW_SELECT:	index=1;	break;
		case T2K_CUSTOM_HW_A:		index=2;	break;
		case T2K_CUSTOM_HW_B:		index=3;	break;
		case T2K_CUSTOM_HW_RIGHT:	index=4;	break;
		case T2K_CUSTOM_HW_LEFT:	index=5;	break;
		case T2K_CUSTOM_HW_DOWN:	index=6;	break;
		case T2K_CUSTOM_HW_UP:		index=7;	break;
		default:
			return false;
	}
	gUseCustomHW |=(uint8_t)inType;
	gCustomHwGpioPin[index]=inGpioPinNo;
	gCustomHwInputType[index]=inOnType;
	return true;
}

bool t2kRemoveCustomHW(T2K_CustomHwType inType) {
	int index=-1;
	switch(inType) {
		case T2K_CUSTOM_HW_START:	index=0;	break;
		case T2K_CUSTOM_HW_SELECT:	index=1;	break;
		case T2K_CUSTOM_HW_A:		index=2;	break;
		case T2K_CUSTOM_HW_B:		index=3;	break;
		case T2K_CUSTOM_HW_RIGHT:	index=4;	break;
		case T2K_CUSTOM_HW_LEFT:	index=5;	break;
		case T2K_CUSTOM_HW_DOWN:	index=6;	break;
		case T2K_CUSTOM_HW_UP:		index=7;	break;
		default:
			return false;
	}
	gUseCustomHW &= ~((uint8_t)inType);
	gCustomHwGpioPin[index]=-1;
	return true;
}

bool t2kRegisterCutomCKB(T2K_CustomCkbType inType,int inI2CVal,bool inOnType) {
	int8_t index=-1;
	switch(inType) {
		case T2K_CUSTOM_CKB_START:	index=0;	break;
		case T2K_CUSTOM_CKB_SELECT:	index=1;	break;
		case T2K_CUSTOM_CKB_A:		index=2;	break;
		case T2K_CUSTOM_CKB_B:		index=3;	break;
		case T2K_CUSTOM_CKB_RIGHT:	index=4;	break;
		case T2K_CUSTOM_CKB_LEFT:	index=5;	break;
		case T2K_CUSTOM_CKB_DOWN:	index=6;	break;
		case T2K_CUSTOM_CKB_UP:		index=7;	break;
		default:
			return false;
	}
	gUseCustomCKB |=(uint8_t)inType;
	gCustomCKBI2CVal[index]=inI2CVal;
	gCustomCKVInputType[index]=inOnType;
	return true;
}

void t2kInputUpdate() {
	gLastStatus=gCurrentStatus;

	if(digitalRead(kKeyboardInt)==0) {
    	Wire1.requestFrom(kKeyboardI2CAddr,1);	// request 1 byte from keyboard
		if( Wire1.available() ) {
			gCurrentStatus=Wire1.read();		// receive a byte as character
			// Serial.printf("READ STATUS %X",gCurrentStatus);
		}
	}
}

bool t2kIsPressedNativeButtonA() { return gCurrentStatusNativeA; }
bool t2kIsPressedNativeButtonB() { return gCurrentStatusNativeB; }
bool t2kIsPressedNativeButtonC() { return gCurrentStatusNativeC; }

bool t2kNowPressedNativeButtonA() {
	return gCurrentStatusNativeA==true && gLastStatusNativeA==false;
}
bool t2kNowPressedNativeButtonB() {
	return gCurrentStatusNativeB==true && gLastStatusNativeB==false;
}
bool t2kNowPressedNativeButtonC() {
	return gCurrentStatusNativeC==true && gLastStatusNativeC==false;
}

static bool checkPressedButton(uint8_t inMask) {
	return (gCurrentStatus & inMask)==0
//		|| (gUseCustomHW!=0 && (gCurrentStatusCustomHW & inMask)==0)
//		|| (gUseCustomCKB!=0 && (gCurrentStatusCardKey & inMask)==0)
		;
}

bool t2kIsPressedStart()  { return checkPressedButton(GBF_MASK_START); }
bool t2kIsPressedSelect() { return checkPressedButton(GBF_MASK_SELECT); }
bool t2kIsPressedA() { return checkPressedButton(GBF_MASK_A); }
bool t2kIsPressedB() { return checkPressedButton(GBF_MASK_B); }
bool t2kIsPressedRight() { return checkPressedButton(GBF_MASK_RIGHT); }
bool t2kIsPressedLeft()  { return checkPressedButton(GBF_MASK_LEFT); }
bool t2kIsPressedUp()    { return checkPressedButton(GBF_MASK_UP); }
bool t2kIsPressedDown()  { return checkPressedButton(GBF_MASK_DOWN); }

static bool nowPressed(uint8_t inMask) {
	return ((gCurrentStatus & inMask)==0 && (gLastStatus & inMask)!=0)
		|| (gUseCustomHW!=0
			&& (gCurrentStatusCustomHW & inMask)==0
				&& (gLastStatusCustomHW & inMask)!=0)
		|| (gUseCustomCKB!=0
			&& (gCurrentStatusCardKey & inMask)==0
				&& (gLastStatusCardKey & inMask)!=0)				
		;
}

bool t2kNowPressedStart()  { return nowPressed(GBF_MASK_START);  }
bool t2kNowPressedSelect() { return nowPressed(GBF_MASK_SELECT); }
bool t2kNowPressedA() { return nowPressed(GBF_MASK_A); }
bool t2kNowPressedB() { return nowPressed(GBF_MASK_B); }
bool t2kNowPressedRight() { return nowPressed(GBF_MASK_RIGHT); }
bool t2kNowPressedLeft()  { return nowPressed(GBF_MASK_LEFT);  }
bool t2kNowPressedUp()    { return nowPressed(GBF_MASK_UP);    }
bool t2kNowPressedDown()  { return nowPressed(GBF_MASK_DOWN);  }

bool t2kNowPressedAnyButton() {
	return t2kNowPressedStart() || t2kNowPressedSelect()
		|| t2kNowPressedA()     || t2kNowPressedB()
		|| t2kNowPressedLeft()  || t2kNowPressedRight()
		|| t2kNowPressedUp()    || t2kNowPressedDown();
}

