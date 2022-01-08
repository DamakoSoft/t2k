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

#ifndef __T2K_H__
#define __T2K_H__

// The t2k program suite is based on the knowledge of many predecessors.
// If these predecessors had not made their findings public, this library would
// never have been completed.
#include <t2kCommon.h>

// t2kGCore is the foundation of our graphics library. It is based on MHageGH's
// M5Stack_LCD_DMA library. You can get the original MHageGH's M5Stack_LCD_DMA
// here:
//     https://github.com/MhageGH/M5Stack_LCD_DMA
// We would like to thank Mr. MHageGH for making such a wonderful program
// available to the public.
#include <t2kGCore.h>

// t2kSCore is the foundation of our sound (tone generator) library.
#include <t2kSCore.h>

// t2kICore is the foundation of our input (for GameBoy Face) library.
#include <t2kICore.h>

#ifndef FONT_OFF
	#include <t2kFont.h>
#endif

#ifndef SPRITE_OFF
	#include <t2kSprite.h>

	//#ifndef COLLISION_OFF
	//	#include <t2kCollision.h>
	//#endif
#endif

#ifndef GRAPHICS_OFF
	#include <t2kGraphics.h>
#endif

#ifndef MML_OFF
	#include <t2kMML.h>
#endif

#ifndef SCENE_OFF
	#include <t2kScene.h>
#endif

inline void t2kUpdate() {
#ifndef MML_OFF
	t2kUpdateMML();
#endif

	t2kInputUpdate();

#ifndef SCENE_OFF
	t2kSceneUpdate();
#endif

	t2kFlip();
}

#endif // __T2K_H__

