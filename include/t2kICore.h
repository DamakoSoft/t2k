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

#ifndef __T2K_ICORE_H__
#define __T2K_ICORE_H__

enum T2K_CustomHwType {
	T2K_CUSTOM_HW_START =0x80,
	T2K_CUSTOM_HW_SELECT=0x40,
	T2K_CUSTOM_HW_A     =0x20,
	T2K_CUSTOM_HW_B     =0x10,
	T2K_CUSTOM_HW_RIGHT =0x08,
	T2K_CUSTOM_HW_LEFT  =0x04,
	T2K_CUSTOM_HW_DOWN  =0x02,
	T2K_CUSTOM_HW_UP    =0x01,
};

enum T2K_CustomCkbType {
	T2K_CUSTOM_CKB_START =0x80,
	T2K_CUSTOM_CKB_SELECT=0x40,
	T2K_CUSTOM_CKB_A     =0x20,
	T2K_CUSTOM_CKB_B     =0x10,
	T2K_CUSTOM_CKB_RIGHT =0x08,
	T2K_CUSTOM_CKB_LEFT  =0x04,
	T2K_CUSTOM_CKB_DOWN  =0x02,
	T2K_CUSTOM_CKB_UP    =0x01,
};

#define	CKB_VAL_RET	 	0x0D
#define	CKB_VAL_L_LOW	0x6C
#define	CKB_VAL_X_LOW	0x78
#define	CKB_VAL_Z_LOW	0x7A
#define	CKB_VAL_RIGHT	0xB7
#define	CKB_VAL_LEFT	0xB4
#define	CKB_VAL_DOWN	0xB6
#define	CKB_VAL_UP		0xB5

enum class T2K_StartMode {
	kStartGame,
	kUseDefaultWiFi,
	kSelectWiFi,	
};

const bool kOnIsHigh=true;
const bool kOnIsLow =false;

bool t2kICoreInit();
void t2kInputUpdate();
T2K_StartMode t2kCheckStartMode(const int inNumOfTry=3);

bool t2kRegisterCutomCKB(T2K_CustomCkbType inType,int inI2CVal,bool inOnType) ;

bool t2kRegisterCutomHW(T2K_CustomHwType inType,int inGpioPinNo,bool inOnType);
bool t2kRemoveCustomHW(T2K_CustomHwType inType);

bool t2kIsPressedNativeButtonA();
bool t2kIsPressedNativeButtonB();
bool t2kIsPressedNativeButtonC();

bool t2kNowPressedNativeButtonA();
bool t2kNowPressedNativeButtonB();
bool t2kNowPressedNativeButtonC();

bool t2kIsPressedStart();
bool t2kIsPressedSelect();
bool t2kIsPressedA();
bool t2kIsPressedB();
bool t2kIsPressedRight();
bool t2kIsPressedLeft();
bool t2kIsPressedUp();
bool t2kIsPressedDown();

bool t2kNowPressedStart();
bool t2kNowPressedSelect();
bool t2kNowPressedA();
bool t2kNowPressedB();
bool t2kNowPressedRight();
bool t2kNowPressedLeft();
bool t2kNowPressedUp();
bool t2kNowPressedDown();

bool t2kNowPressedAnyButton();

#endif

