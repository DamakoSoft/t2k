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

#ifndef __T2K_COMMON_H__
#define __T2K_COMMON_H__

#ifndef TEST_ON_PC
	#include <M5Core2.h>
	#include <Arduino.h>
	#include <Wire.h>
	#include <SPI.h>
#endif

#ifdef ENABLE_DEBUG_MESSAGE
    #define DEBUG_LN(s) Serial.println(s)
    #define DEBUG(fmt,...) Serial.printf(fmt,__VA_ARGS__)
#else
    #define DEBUG_LN(s)
    #define DEBUG(fmt,...)
#endif

#ifdef TEST_ON_PC
	#define ERROR printf
	const int portMAX_DELAY=-1;
	#include "M5Stack.h"	// dummy
#else
	#define ERROR Serial.printf
#endif

enum T2K_HW_Type {
	kT2K_HW_Unknown,
	kT2K_HW_BASIC,
	kT2K_HW_CORE2,
};

void t2kInit();

T2K_HW_Type t2kGetHwType();
const char *t2kGetHwName();

void t2kLcdBrightness(uint8_t inBrightness);
void t2kLed(bool inIsOn);

#endif

