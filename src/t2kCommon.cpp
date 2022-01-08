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
#include <Wire.h>

static T2K_HW_Type gHwType=kT2K_HW_Unknown;

static T2K_HW_Type detectHW();
static uint8_t read8bit(uint8_t inAddr);

T2K_HW_Type t2kGetHwType() {
	if(gHwType==kT2K_HW_Unknown) {
		gHwType=detectHW();
	}
	return gHwType;
}

const char *t2kGetHwName() {
	switch(gHwType) {
		case kT2K_HW_BASIC: return "M5Stack Basic";
		case kT2K_HW_CORE2: return "M5Stack Core2";
		default:
			return "Unknown HW";
	}
}

static const uint8_t kAXP192_addr=0x30;
static bool gIsInit=false;

static AXP192 gAxp;
static M5Touch gTouch;

static M5Display gLcd;

// same as https://github.com/m5stack/M5Core2/blob/master/src/AXP192.cpp
void t2kInit() {
	if( gIsInit ) { return; }

	gAxp.begin(kMBusModeOutput);
	gLcd.begin();
	gTouch.begin();
	gIsInit=true;

	Serial.begin(115200);
	delay(50);
	Serial.println("");
	Serial.printf("--> t2kInit: Arduino main program is running on core %d\n",
				  xPortGetCoreID());
}

void t2kLed(bool inIsOn) {
	gAxp.SetLed(inIsOn);
}

void t2kLcdBrightness(uint8_t inBrightness) {
	gAxp.SetLcdVoltage(2500+int((inBrightness/255.0f)*800));
}

// for internal use
void t2kLcdReset(bool inState) {
	gAxp.SetLCDRSet(inState);
}

static T2K_HW_Type detectHW() {
    Wire1.begin(21,22);	// SDA=GPIO 21, SCL=GPIO 22
    Wire1.setClock(400000);

   	uint8_t check=read8bit(kAXP192_addr);
// Serial.printf("check=%d\n",check);
	if(check==255) {
		return kT2K_HW_BASIC;
	} else { // if(check==2) {
		return kT2K_HW_CORE2;
	}
	return  kT2K_HW_Unknown;
}

static uint8_t read8bit(uint8_t inAddr) {
    Wire1.beginTransmission(0x34);
    Wire1.write(inAddr);
    Wire1.endTransmission();
    Wire1.requestFrom(0x34,1);
    return Wire1.read();
}

