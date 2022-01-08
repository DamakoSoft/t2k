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

#include <M5Core2.h>
#include <Wire.h>
#include <driver/i2s.h>

#include <Update.h>

#include <algorithm>

#include <t2k.h>

// #include <t2kDev.h>

const uint8_t buttonA_GPIO = 39;
const uint8_t buttonB_GPIO = 38;
const uint8_t buttonC_GPIO = 37;
uint32_t buttonA_count = 0;

const char *gShoot="T200L32O5 CE";

// sample BGM
// composed by Utarin, MML encoded by KojiSaito.
const char *gSampleBGM="@M120 @V99 @K-3"
					   "< $"
					   "c12r24e12r24   c12r24e12r24   c24r48c24r48e12r24   c12r24e12r24"
					   ">b12r24<d12r24 >b12r24<d12r24 >b24r48b24r48<d12r24 >b12r24<d12r24"
					   ">a12r24<c12r24 >a12r24<c12r24 >a24r48a24r48<c12r24 >a12r24<c12r24"
					   ">g12r24b12r24  g12r24b=12r24  g24r48g24r48b=12r24  g12r24b=12r24"
					   "<"
					   "c12r24e12r24   c12r24e12r24   c24r48c24r48e12r24   c12r24e12r24"
					   ">b12r24<d12r24 >b12r24<d12r24 >b24r48b24r48<d12r24 >b12r24<d12r24"
					   ">a12r24<c12r24 >a12r24<c12r24 >a24r48a24r48<c12r24 >a12r24<c12r24"
					   ">g12r24b12r24  g12r24b=12r24  g24r48g24r48b=12r24  g12r24b=12r24"
					   "<"
					   "e16r16e16r16 e8r16e16 r16e16r16e16 r16e16r16e16"
					   "d16r16d16r16 d8r16d16 r16d16r16d16 r16d16r16d16"
					   "f16r16f16r16 f8r16f16 r16f16r16f16 r16f16r16f16"
					   "e16r16e16r16 e8r16d16 d8r16c16     c16r16d16r16"
					   " "
					   "e16r16e16r16 e8r16e16 r16e16r16e16 r16e16r16e16"
					   "d16r16d16r16 d8r16d16 r16d16r16d16 r16d16r16d16"
					   "f16r16f16r16 f8r16f16 r16f16r16f16 r16f16r16f16"
					   "e16r16e16r16 e8r16d16 d8r16c16     c16r16d16r16";

static const char *gBallSpritePattern[]={
	"__RRRR__",
	"_RRRRRR_",
	"RRWRRRRR",
	"RRRRRRRR",
	"RRRRRRRr",
	"RRRRRRrr",
	"_RRRRrr_",
	"__rrrr__",
};
static T2K_Sprite16colors gBallSprite;
static uint8_t gBallSpritePalette[7][16];

static const char *gMyShipPattern[]={
	"____W____",
	"___WWW___",
	"__WWWWW__",
	"__WWCWW__",
	"R_WCCCW_R",
	"W_WCWCW_W",
	"WYWWWWWYW",
	"WWWY_YWWW",
	"C_W___W_C",
};
static T2K_Sprite16colors gMyShipSprite;

static const char *gAlienPattern[]={
	"__GGGGg_",
	"_GggggGg",
	"GgY_Y__G",
	"Gg_____G",
	"GGg___GG",
	"_GGGGGGg",
	"__Gg_Gg_",
	"__Gg__Gg",
};
static T2K_Sprite16colors gAlienSprite;


const int kNumOfStars=100;
static int gStarXY[kNumOfStars];

static void topMenu();
static void blinkTopMenu();
static void drawTopMenu(int inSelectedItem,int inCounterForBlink);

static void mmlTest();
static void displaySceneIdError();

static void spriteTest();
static void initBall(int inTargetIndex);

static void fadeOut();

enum SceneID {
	kTOP_SCENE_ID			=  0,

	kMML_TEST_SCENE_ID		= 10,
	kSPRITE_TEST_SCENE_ID	= 20,

	kBLINK_TOP_MENU_SCENE_ID=200,
	kFADE_OUT_SCENE_ID		=201,

	kINVALID_SCENE_ID		=255,
};
static T2K_Scene gSceneTable[]={
	{ kTOP_SCENE_ID,			topMenu 	 },
	{ kMML_TEST_SCENE_ID,		mmlTest 	 },
	{ kSPRITE_TEST_SCENE_ID,	spriteTest	},

	{ kBLINK_TOP_MENU_SCENE_ID,	blinkTopMenu },
	{ kFADE_OUT_SCENE_ID,		fadeOut 	 },
	
	{ kINVALID_SCENE_ID,		NULL		 },	// sentinel
};

// static T2K_StartMode gStartMode;

void setup() {
	t2kInit();

	// init input core module
	t2kICoreInit();
	delay(500); Serial.println("--> t2kICore INIT DONE");

#if 0
	gStartMode=t2kCheckStartMode();
	if(gStartMode!=T2K_StartMode::kStartGame) {
		M5.begin(true,true,false,false);
		t2kStartFtp(gStartMode);
		return;
	}
#endif

	if( t2kGCoreInit() ) { Serial.println("t2kGCoreInit Done.\n"); }
	t2kGCoreStart();
	t2kSetBrightness(128);	// 0 to 255
	t2kFill(~0);

	t2kSCoreInit();
	t2kSCoreStart();

	Serial.println("=== SOUND INIT DONE");
	Serial.printf("internal Free Heap %d\n",ESP.getFreeHeap());

	t2kFontInit();
	Serial.println("=== t2kFont INIT DONE");

	bool result=t2kMmlInit();
	Serial.printf("=== t2kMML INIT DONE %d\n",result);

	result=t2kSceneInit(displaySceneIdError);
	Serial.printf("=== t2kScene INIT DONE %d\n",result);

	// t2kCollisionInit();

	Serial.println("===== START =====");
	Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
	Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
	Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
	Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());

	if(t2kCheckMML(gSampleBGM)==false) { Serial.printf("MML ERROR BGM\n"); }
	if(t2kCheckMML(gShoot)==false) { Serial.printf("MML ERROR Paro\n"); }
	Serial.printf("all MML check done.\n");

	initBallSpritePalette();
	if(t2kInitSprite(&gBallSprite,8,8,gBallSpritePattern,
					 0,0,true,gBallSpritePalette[0])==false) {
		Serial.printf("BALL SPRITE ERROR\n");
	}

	if(t2kInitSprite(&gMyShipSprite,9,9,gMyShipPattern,4,4)==false) {
		Serial.printf("MyShip SPRITE ERROR\n");
	}

	if(t2kInitSprite(&gAlienSprite,8,8,gAlienPattern)==false) {
		Serial.printf("Alien SPRITE ERROR\n");
	}

	t2kAddScenes(gSceneTable);
	initBall(0);

	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_UP,CKB_VAL_UP,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_DOWN,CKB_VAL_DOWN,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_LEFT,CKB_VAL_LEFT,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_RIGHT,CKB_VAL_RIGHT,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_A,CKB_VAL_X_LOW,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_B,CKB_VAL_Z_LOW,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_START,CKB_VAL_RET,false);
	t2kRegisterCutomCKB(T2K_CUSTOM_CKB_SELECT,CKB_VAL_L_LOW,false);

	t2kSetNextSceneID(kTOP_SCENE_ID);
	t2kLed(true); // turn on green LED.
	t2kSetMasterVolume(kAllChannels,50);

	for(int i=0; i<kNumOfStars; i++) {
		gStarXY[i]=(random(kGRamWidth)<<8) | random(kGRamHeight);
	}
}

int gVolume=128;
bool gSoundON=true;
bool gPlayingBGM=false;

uint32_t gFrameCount=0;
int gTestCH=0;

void loop() {
#if 0
	if(gStartMode!=T2K_StartMode::kStartGame) {
		t2kFtpUpdate();
		return;
	}
#endif

	gFrameCount++;
	t2kUpdate();
}

static int gMyShipX=80;
static void mmlTest() {
	t2kFill(kBlack);
	t2kSetMasterVolume(3,128);
	uint8_t *gram=t2kGetFramebuffer();

	const int kMyShipMoveDx=4;
	if(t2kIsPressedLeft() && kMyShipMoveDx<gMyShipX) { gMyShipX-=kMyShipMoveDx; }
	if(t2kIsPressedRight() && gMyShipX<150-kMyShipMoveDx) { gMyShipX+=kMyShipMoveDx; }

	// draw stars
	for(int i=0; i<kNumOfStars; i++) {
		int t=i%3+1;
		gStarXY[i]+=t;
		int x=gStarXY[i]>>8;
		int y=gStarXY[i]&0xFF;
		if(x>=kGRamWidth || y>=kGRamHeight) {
			gStarXY[i]=random(kGRamWidth)<<8;
			gStarXY[i]+=gFrameCount%4;
			continue;
		}
		gram[FBA(x,y)]=RGB(t*2,t*2,t);
	}

	// draw alien
	{
		float t1=radians(gFrameCount%180*2);
		float t2=radians(gFrameCount%60*6);
		const int alienOrbitR1=30;
		int cx=(int)(alienOrbitR1*cos(t1))+kGRamWidth/2;
		int cy=(int)(alienOrbitR1*sin(t1))+kGRamHeight/2;
		const int alienOrbitR2=20;
		int alienX=(int)(alienOrbitR2*cos(t2))+cx;
		int alienY=(int)(alienOrbitR2*sin(t2))+cy;
		t2kPutSprite(&gAlienSprite,alienX,alienY);
	}

	// draw volume meter
	{
		int volumeBarH=gVolume/4;
		t2kPrintf(4,96-volumeBarH,kWhite,"BGM%3d",gVolume);
		if( gPlayingBGM ) {
			t2kFillRect(4,108-volumeBarH,5,volumeBarH,RGB(0,4,3));
		} else {
			t2kDrawRect(4,108-volumeBarH,5,volumeBarH,RGB(0,4,3));
		}
	}

	t2kPutSprite(&gMyShipSprite,gMyShipX,100);

	if( t2kIsPressedUp() ) {
		if(gVolume<255) { gVolume+=5; }
		gVolume=min(gVolume,255);
	}
	if( t2kIsPressedDown() ) {
		if(gVolume>0) { gVolume-=5; }
		gVolume=max(gVolume,0);
	}
	t2kSetMasterVolume(0,gVolume);

	if( t2kNowPressedA() ) {
		if( gPlayingBGM ) {
			t2kStopMML(0);
			gPlayingBGM=false;
		} else {
			t2kPlayMML(0,gSampleBGM);
			gPlayingBGM=true;
		}
	}
	if( t2kNowPressedSelect() ) {
		gTestCH=(gTestCH+1)%4;
	}

	if( t2kNowPressedB() ) {
		t2kFill(kRed);
		t2kPlayMML(3,gShoot);
	}

	// moving small green box
	{
		const uint8_t color[8]={
			kBlack,kBlue,kRed,kMagenta,kGreen,kCyan,kYellow,kWhite
		};
		uint8_t rainbow=color[gFrameCount%8];
		static int sx=0;
		sx=(sx+5)%180;
		t2kFillRect(sx-20,14, 18,2,rainbow);
	}

	t2kPrintf(4,kWhite,"SCPRE: %07d",gFrameCount);
	t2kPrintf(110,kWhite,"SELECT+START = >TOP");

	if(t2kIsPressedStart() && t2kIsPressedSelect()) {
		t2kSetNextSceneID(kFADE_OUT_SCENE_ID);
	}
}

static int gSelectedTopMenuItem=0;
static int gBlinkCount=0;
static int gNextSceneIdFromTopMenu=-1;
static bool gInitBrightness=false;
static void topMenu() {
	if(gInitBrightness==false) {
		t2kSetBrightness(255);
		gInitBrightness=true;
	}

	const int numOfMenuItems=3;
	drawTopMenu(gSelectedTopMenuItem,-1);

	if( t2kNowPressedUp() ) {
		gSelectedTopMenuItem=(gSelectedTopMenuItem+numOfMenuItems-1)%numOfMenuItems;
		t2kPlayMML(0,"@V4 L32 N880");
	}
	if(t2kNowPressedSelect() || t2kNowPressedDown() ) {
		gSelectedTopMenuItem=(gSelectedTopMenuItem+1)%numOfMenuItems;
		t2kPlayMML(0,"@V4 L32 N800");
	}

	if(t2kNowPressedStart() || t2kNowPressedB() ) {
		switch(gSelectedTopMenuItem) {
			case 0:	gNextSceneIdFromTopMenu=kMML_TEST_SCENE_ID;		break;
			case 1: gNextSceneIdFromTopMenu=kSPRITE_TEST_SCENE_ID;	break;
			case 2: gNextSceneIdFromTopMenu=kINVALID_SCENE_ID;		break;
			default:
				gInitBrightness=false;
				gNextSceneIdFromTopMenu=kINVALID_SCENE_ID;
		}
		gBlinkCount=0;
		t2kSetNextSceneID(kBLINK_TOP_MENU_SCENE_ID);
	}
}
static int gCircleCenterX=random(kGRamWidth);
static int gCircleCenterY=random(kGRamHeight);
static void drawTopMenu(int inSelectedItem,int inCounterForBlink) {
	t2kFill(kBlack);

	// circle animation
	const int circleR=gFrameCount%40*5;
	if(circleR==0) {
		gCircleCenterX=random(kGRamWidth);
		gCircleCenterY=random(kGRamHeight);
	}
	t2kDrawCircle(gCircleCenterX,gCircleCenterY,circleR,RGB(4,5,3));

	// rotate a line
	const float pi=3.141592f;
	const float t1=(gFrameCount%360)/180.0f*pi;
	const int r=120,cx=28,cy=45;
	int x1=(int)(r*cos(t1)+cx),   y1=(int)(r*sin(t1)+cy);
	int x2=(int)(r*cos(t1+pi)+cx),y2=(int)(r*sin(t1+pi)+cy);
	t2kDrawLine(x1,y1,x2,y2,RGB(1,4,3));
	const float t2=t1+pi/2;
	int x3=(int)(r*cos(t2)+cx),   y3=(int)(r*sin(t2)+cy);
	int x4=(int)(r*cos(t2+pi)+cx),y4=(int)(r*sin(t2+pi)+cy);
	t2kDrawLine(x3,y3,x4,y4,RGB(1,4,3));

	t2kPrintf(50,kWhite,"DEMO TESTER");

	const char *item[3]={
		"PLAY MML",
		"DRAW SPRITES",
		"ERROR SCREEN",
	};
	for(int i=0; i<3; i++) {
		if(inCounterForBlink>0 && i==inSelectedItem && inCounterForBlink%4>=2) {
			continue;
		} else {
			t2kPrintf(47,70+i*10,kWhite,item[i]);
		}
		if(inCounterForBlink>0 && inCounterForBlink%4==0) {
			t2kPlayMML(0,"L96 O6 CEG");
		}
	}
	t2kDrawFontPattern(35,70+inSelectedItem*10,kWhite,T2K_TrianglePatternRight);
}
static void blinkTopMenu() {
	if(++gBlinkCount>21) {
		// blink end
		t2kSetNextSceneID(gNextSceneIdFromTopMenu);
		switch(gNextSceneIdFromTopMenu) {
			case kMML_TEST_SCENE_ID:
				gPlayingBGM=true;
				t2kPlayMML(0,gSampleBGM);
				break;
			case kSPRITE_TEST_SCENE_ID:
				t2kPlayMML(0,gSampleBGM);
				break;
		}
	} else {
		drawTopMenu(gSelectedTopMenuItem,gBlinkCount);
	}
}

enum {
	kDarkBlue=RGB(0,0,1),
	kDarkRed =RGB(3,0,0),
	kDarkMagenta=RGB(3,0,1),
	kDarkGreen  =RGB(0,3,0),
	kDarkCyan	=RGB(0,3,1),
	kDarkYellow =RGB(3,3,0),
	kGray    =RGB(3,3,2),
};
static uint8_t kDarkBallColor[7]={
	kDarkBlue,kDarkRed,kDarkMagenta,kDarkGreen,kDarkCyan,kGray,
};
static uint8_t kBrightBallColor[7]={
	kBlue,kRed,kMagenta,kGreen,kCyan,kYellow,kWhite,
};
static void initBallSpritePalette() {
	for(int i=0; i<7; i++) {
		t2kSetDefault16colorPalette(gBallSpritePalette[i]);
		gBallSpritePalette[i][ 2]=kDarkBallColor[i];
		gBallSpritePalette[i][10]=kBrightBallColor[i];
	}
	gBallSpritePalette[6][15]=kYellow;
}

// ------------------------------------------------------------------
//	sprite test
// ------------------------------------------------------------------
struct Ball {
	float x,y;
	float dx,dy;
	int color;
};
static int gNumOfSprite=1;
const int MAX_BALLS=256;
static Ball gBall[MAX_BALLS];
static void initBall(int inTargetIndex) {
	gBall[inTargetIndex].x=random(0,159);
	gBall[inTargetIndex].y=random(0,119);

	float dx,dy;
	do {
		dx=random(-20,+20)/10.0f*(4+inTargetIndex/20.0f);
		dy=random(-20,+20)/10.0f*(4+inTargetIndex/20.0f);;
	} while(abs(dx)<1|| abs(dy)<1);
	gBall[inTargetIndex].dx=dx;
	gBall[inTargetIndex].dy=dy;
}
static void spriteTest() {
	t2kSetMasterVolume(kAllChannels,128);
	t2kFill(kBlack);
	if(t2kIsPressedUp() && gNumOfSprite<MAX_BALLS) {
		initBall(gNumOfSprite++);				
		t2kPlayMML(1,gShoot);
	}
	if(t2kIsPressedDown() && gNumOfSprite>1) {
		gNumOfSprite--;
		t2kPlayMML(1,gShoot);
	}
	for(int i=0; i<gNumOfSprite; i++) {
		gBall[i].x+=gBall[i].dx;
		gBall[i].y+=gBall[i].dy;
		if(gBall[i].x<0) { gBall[i].x=0; gBall[i].dx*=-1; }
		if(gBall[i].x+8>=kGRamWidth) { gBall[i].x=kGRamWidth-8-1; gBall[i].dx*=-1; }
		if(gBall[i].y<0) { gBall[i].y=0; gBall[i].dy*=-1; }
		if(gBall[i].y+8>=kGRamHeight) { gBall[i].y=kGRamHeight-8-1; gBall[i].dy*=-1; }
		t2kPutSprite(&gBallSprite,gBall[i].x,gBall[i].y,gBallSpritePalette[i%7]);
	}
	uint8_t color = gNumOfSprite<MAX_BALLS ? kWhite : kCyan;
	t2kPrintf(60,color,"Num of Sprites %03d",gNumOfSprite);
	t2kPrintf(4,110,kWhite,"SELECT+START = >TOP");

	if(t2kIsPressedStart() && t2kIsPressedSelect()) {
		t2kSetNextSceneID(kFADE_OUT_SCENE_ID);
	}
}


// ------------------------------------------------------------------
//	fade out
// ------------------------------------------------------------------
const int kFadeOutCountInitialValue=25;
static uint8_t gFadeOutCount=kFadeOutCountInitialValue;
static void fadeOut() {
	t2kCopyFromFrontBuffer();
	t2kSetBrightness(gFadeOutCount*10);
	gFadeOutCount--;
	if(gFadeOutCount<3) { t2kFill(kBlack); }
	if(gFadeOutCount==0) {
		t2kSetNextSceneID(kTOP_SCENE_ID);
		gFadeOutCount=kFadeOutCountInitialValue;
		gInitBrightness=false;
		t2kStopMMLs();
		t2kQuiet();
	}
}

static void displaySceneIdError() {
	t2kFill(kRed);
	t2kPrintf(10,40,kWhite,"SCENE ID ERROR");
	t2kPrintf(10,50,kWhite,"scene id=%d",t2kGetCurrentSceneID());

	t2kPrintf(10,70,kWhite,"PUSH ANY BUTTON");
	t2kPrintf(10,80,kWhite,"TO RETURN TOP MENU");

	int n=gFrameCount%11;
	for(int i=0; i<n; i++) {
		t2kPutChar(10+i*8,100,kWhite,'>');
	}
	t2kPrintf(100,100,kWhite,"%06d",gFrameCount);

	if( t2kNowPressedAnyButton() ) {
		t2kSetNextSceneID(kTOP_SCENE_ID);
	}
}

