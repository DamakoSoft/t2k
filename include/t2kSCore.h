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

// for delta sigma
// see https://qiita.com/geachlab/items/0cd0cd58f666710e0607

#ifndef __T2K_SCORE_H__
#define __T2K_SCORE_H__

const int kNumOfChannels=4;
const int kAllChannels=-1;

bool t2kSCoreInit();
void t2kSCoreStart();
bool t2kSetMasterVolume(int8_t inChannel,uint8_t inVolume);
bool t2kTone(uint8_t inChannel,float inFreq,int16_t inDurationMSec,uint8_t inVolume);
bool t2kAddTone(uint8_t inChannel,float inFreq,int16_t inDurationMSec,uint8_t inVolume);
bool t2kStartToneSeq(uint8_t inChannel);
bool t2kClearToneSeq(uint8_t inChannel);

void t2kQuiet();


const float 						  T2K_TONE_C4=261.626,T2K_TONE_C4s=277.183;
const float T2K_TONE_D4f=T2K_TONE_C4s,T2K_TONE_D4=293.665,T2K_TONE_D4s=311.127;
const float T2K_TONE_E4f=T2K_TONE_D4s,T2K_TONE_E4=329.628;
const float 						  T2K_TONE_F4=349.228,T2K_TONE_F4s=369.994;
const float T2K_TONE_G4f=T2K_TONE_F4s,T2K_TONE_G4=391.995f,T2K_TONE_G4s=415.305;
const float	T2K_TONE_A4f=T2K_TONE_G4s,T2K_TONE_A4=440.000f,T2K_TONE_A4s=523.251;
const float T2K_TONE_B4f=T2K_TONE_A4s,T2K_TONE_B4=493.883f;
const float 						  T2K_TONE_C5=523.251f;

#endif

