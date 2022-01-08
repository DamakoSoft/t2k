# t2k - Tatsuko Driver by DamakoSoft
Tatsuko Driver (as called t2k) is NOT a device driver which drives some device.
t2k is a software library designed to drive game development.

t2k is a software library available in M5Stack Core2
that supports sprites, font, screen display, music playback, and gamepad input, etc.
The software design is focused to be used for game development.

## DamakoSoft - What is Damako?
The Damako is a kind of rice chunk used 
in the local cuisine of Akita Prefecture in Japan.
DamakoSoft is made up of members with ties to Akita, hence we give the name.

![damako](https://user-images.githubusercontent.com/22868285/148642280-065e35a2-e40b-48e1-883f-1f5525eb5892.jpeg)

A example of Damako

## Tatsuko Driver - What is Tatsuko?
The name "Tatsuko" in TatsuKo Driver comes from the legend princess Tatsuko,
who appears in old tales from Akita prefecture in Japan.
For more information, please refer to this page of Senboku City:
[https://www.city.semboku.akita.jp/en/sightseeing/spot/04\_tatsukodensetsu.html](https://www.city.semboku.akita.jp/en/sightseeing/spot/04_tatsukodensetsu.html)

# Contents

* How to use
* Components overview
* API overview
* DamakoSoft staff

# How to use
If you want to see how to use it quickly, see src/t2kDemo.ino.

The current t2k uses PlatformIO,
and you can use it by replacing src/t2kDemo.ino.

Typical usage is as follows:

```
#include <M5Core2.h>
//    :
// Include the necessary files
//    :

#include <t2k.h>

// global variables, prototype declaration, etc.

void setup() {
    t2kInit();
    t2kICoreInit();

    //    :
    // Various initial settings
    //    :

    t2kGCoreInit();
    t2kGCoreStart();
    t2kSetBrightness(128);

    t2kSCoreInit();
    t2kSCoreStart();

    t2kFontInit();

    t2kMmlInit();

    // The displaySceneIdError function is implemented as a function
	// to display a system error.
    t2kSceneInit(displaySceneIdError);

    // The scene information ‘sceneTable' should be defined somewhere
    // in advance.
    t2kAddScenes(sceneTable);
	// Give the scene identification value at game startup as an argument.
    t2kSetNextSceneID(startSceneID);

	// initialization process, if necessary
}

void loop() {
	//    :
    // Implement here, the process to be performed every frame.
	//    :

    t2kUpdate();
}
```

# Components overview
t2k is a software library consisting of two groups:
the core modules and the base modules.
The more fundamental functions are provided as the core modules,
and the base modules are software components that are built on the core modules.

## Core modules
<dl>
<dt>t2kGCore - t2k Graphics Core module :</dt>
<dd>
Flicker-free screen update system
(double buffering, and high-speed memory data transfer by DMA).

* Resolution: 160x120
* Color: 256 colors (Red: 3 bits, Green: 3 bits, Blue: 2 bits) 
* Coordinate system: top left is (0,0), bottom right is (159,119)
</dd>
<dt>t2kSCore - t2k Sound Core module :</dt>
<dd>
Asynchronous tone sound generation system
(support for multi-channel, and single tone or sound sequences)
</dd>
<dt>t2kICore - t2k Input Core module :</dt>
<dd>
Module for monitoring the status of gamepad buttons
(compatible with M5Stack's official GameBoy FACE)
</dd>
</dl>

## Base modules
<dl>
<dt>t2kSprite - t2k Sprite module :</dt>
<dd>
Sprite definition and display
(support for color palettes)
</dd>
<dt>t2kFont - t2k Font module :</dt>
<dd>
Alphabet character and number etc display module
(font data included)
</dd>
<dt>t2kGraphics - t2k Graphics module :</dt>
<dd>
Geometric shape drawing modules such as straight line and circumference drawing
(high-speed drawing using DDA.)
</dd>
<dt>t2kMML - t2k Music playback module :</dt>
<dd>
Playback of music written in Music Macro Language =  MML
(MML parser and checker included)
</dd>
<dt>t2kScene - t2k Scene manage module :</dt>
<dd>
Registering and switching scenes
(up to 255 scenes can be supported)
</dd>
</dl>

# API overview
## t2kCommon

* void t2kInit()
* void t2kLcdBrightness(uint8\_t inBrightness)  // 255 is max
* void t2kLed(bool inIsOn)

## t2kGCore

* bool t2kGCoreInit()
* void t2kGCoreStart()  // start flipPump task
* void t2kFlip()
* void t2kFill(uint8\_t inRGB332)
* void t2kPSet(int inX,int inY,uint8\_t inRGB332)  // PSet = point set
* uint8\_t t2kGetFramebuffer()
* void t2kCopyFromFrontBuffer()
* void t2kSetBrightness(uint8\_t inBrightness)  // 255 is max brightness

## t2kSCore

* bool t2kSCoreInit()
* void t2kSCoreStart()  // start tonePump task
* bool t2kSetMasterVolume(int8\_t inChannel,uint8\_t inVolume)
* bool t2kTone(uint8\_t inChannel,float inFreq,int16\_t inDurationMSec,uint8\_t inVolume)
* bool t2kAddTone(uint8\_t inChannel,float inFreq,int16\_t inDurationMSec,uint8\_t inVolume)

## t2kICore

* bool t2kICoreInit()
* void t2kInputUpdate()  // called once for each game loop
* bool t2kIsPressed{Start | Select | A | B | Right | Left | Up | Down}()
* bool t2kNowPressed{Start | Select | A | B | Right | Left | Up | Down}()

## t2kSprite

* bool t2kInitSprite( arguments for sprite definision )
* void t2kPutSprite(T2K\_Sprite16colors,int inX,int inY,uint8\_t inCustomPalette=NULL)

## t2kFont

* bool t2kFontInit(const Font \*inFont=kComputerfontFace)
* void t2kPutChar(int inX,int inY,uint8\_8 inRGB332,char inChar)
* void t2kPutStr(int inX,int inY,uint8\_t inRGB332,const char \*inString)
* void t2kPrintf(int inX,int inY,uint8\_t inRGB332,const char \*inFormat,...) 
* void t2kPrintf(int inY,uint8\_t inRGB332,const char \*inFormat,...) // centering version
* void t2kDrawFontPattern(int inX,int inY,uint8\_t inRGB332,const uint8\_t \*inPattern)

## t2kGraphcis

* void t2kDrawLine(int inX1,int inY1,int inX2,int inY2,uint8\_t inRGB332)
* void t2kDrawRect(int inX,int inY,int inWidth,int inHeight,uint8\_t inRGB332)
* void t2kFillRect(int inX,int inY,int inWidth,int inHeight,uint8\_t inRGB332)
* void t2kDrawCircle(int inX,int inY,int inR,uint8\_t inRGB332)

## t2kMML

* bool t2kMmlInit()
* void t2kUpdateMML()  // called once for each game loop
* bool t2kCheckMML(const char \*inMmlString)
* bool t2kPlayMML(uint8\_t inChannel,const char \*inMmlString)
* bool t2kStopMML(uint8\_t inChannel)
* void t2kStopMMLs()

## t2kScene

* bool t2kSceneInit(T2K\_SceneFunc inDefaultSceneFunc)
* void t2kSceneUpdate()  // called once for each game loop
* void t2kAddScenes(T2K\_Scene \*inSceneInfoArray)
* bool t2kAddScene(uint8\_t inSceneID,T2K\_SceneFunc inSceneFunc)
* void t2kSetDefaultSceneFunc(T2K\_SceneFunc inSceneFunc)
* void t2kSetNextSceneID(uint8\_t inNextSceneID)
* uint8\_t t2kGetCurrentSceneID()

# DamakoSoft staff
The official Twitter account of DamakoSoft is @DamakoSoft.
Please follow us if you like.

The members of DamakoSoft are as follows (Twitter accounts are shown):

* DAizo sasaki - @dzonesasaki
* yoshiMAsa sugawara - @raveman179
* KOji saito - @KojiSaito

