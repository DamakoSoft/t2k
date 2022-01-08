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

#ifndef __T2K_SCENE_H__
#define __T2K_SCENE_H__

typedef void (*T2K_SceneFunc)();

struct T2K_Scene {
	int sceneID;
	T2K_SceneFunc sceneFunc;
};

bool t2kSceneInit(T2K_SceneFunc inDefaultSceneFunc);
void t2kSceneUpdate();

void t2kAddScene(uint8_t inSceneID,T2K_SceneFunc inSceneFunc);
bool t2kAddScenes(T2K_Scene *inSceneInfoArray);	// sceneID=-1 means end mark.
void t2kSetDefaultSceneFunc(T2K_SceneFunc inSceneFunc);

void t2kSetNextSceneID(uint8_t inNextSceneID);
uint8_t t2kGetCurrentSceneID();

#endif

