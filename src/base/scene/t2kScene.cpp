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

#include <t2kScene.h>

static T2K_SceneFunc gSceneFunc[256];
static T2K_SceneFunc gDefaultFunc=NULL;

static uint8_t gCurrentSceneID;
static uint8_t gNextSceneID;

bool t2kSceneInit(T2K_SceneFunc inDefaultSceneFunc) {
	for(int i=0; i<256; i++) { gSceneFunc[i]=NULL; }
	if(inDefaultSceneFunc==NULL) { return false; }
	gNextSceneID=0;
	gDefaultFunc=inDefaultSceneFunc;
	return true;
}

void t2kSceneUpdate() {
	gCurrentSceneID=gNextSceneID;
	T2K_SceneFunc func=gSceneFunc[gCurrentSceneID];
	if(func==NULL) {
		if(gDefaultFunc!=NULL) { gDefaultFunc(); }
	} else {
		func();
	}
}

void t2kAddScene(uint8_t inSceneID,T2K_SceneFunc inSceneFunc) {
	gSceneFunc[inSceneID]=inSceneFunc;
}

bool t2kAddScenes(T2K_Scene *inSceneInfoArray) {
	if(inSceneInfoArray==NULL) { return false; }
	for(int i=0; inSceneInfoArray[i].sceneFunc!=NULL; i++) {
		int id=inSceneInfoArray[i].sceneID;
		if(id<0 || 255<id) { return false; }
		gSceneFunc[id]=inSceneInfoArray[i].sceneFunc;
	}
	return true;
}

void t2kSetDefaultSceneFunc(T2K_SceneFunc inSceneFunc) {
	gDefaultFunc=inSceneFunc;
}

uint8_t t2kGetCurrentSceneID() {
	return gCurrentSceneID;
}

void t2kSetNextSceneID(uint8_t inNextSceneID) {
	gNextSceneID=inNextSceneID;
}

