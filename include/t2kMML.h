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

// The range of musical transposition is from -7 to +7.
// We learned about the transposition from the following sites:
// http://www.ones-will.com/blog/dtm/music-theory/9574/
// If we are wrong, please contact us via Twitter account @KojiSaito or @DamakoSoft .

#ifndef __T2K_MML_H__
#define __T2K_MML_H__

bool t2kMmlInit();
bool t2kCheckMML(const char *inMmlString);
bool t2kPlayMML(uint8_t inChannel,const char *inMmlString);
bool t2kStopMML(uint8_t inChannel);
void t2kStopMMLs();
void t2kUpdateMML();

#endif

