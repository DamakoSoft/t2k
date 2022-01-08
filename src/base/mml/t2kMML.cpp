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

// see MML command  http://www.slis.tsukuba.ac.jp/~hiraga.yuzuru.gf/mml/MML.txt

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include <t2kCommon.h>
#include <t2kSCore.h>

const int kBufferingMSec=100;

// high word: numerator
// low  word: denominator
typedef uint32_t Rational;

struct MmlState {
	float tempo;
	float initialTempo;
	Rational musicBeat;
	int musicalTransposition;
	int currentOctaveIndex;
	Rational defaultLength;
	int baseStrength;
	Rational lengthSubTotal;
	float unsendRestDurationMSec;
};
struct MmlInfo {
	bool isAlive;
	const char *mmlStr;
	int mmlStrLength;
	int nextMmlCharIndex;
	int repeatStartIndex;
	bool nowPlaying;
	bool readyToPlay;
	MmlState mmlState;
};

static MmlInfo gMmlInfo[kNumOfChannels];

static float gFreqTable[89];	// gFreqTable[88]=0 <- for rest.
static char gFreqNameStr[89][4];

// ============================== Rational ==============================
#ifdef TEST_ON_PC
static void printRational(Rational inRational) {
	int16_t a=(int16_t)(inRational>>16);
	int16_t b=(int16_t)(inRational & 0xFFFF);
	printf("%d/%d\n",a,b);
}
#endif

static uint32_t MakeRational(int16_t inNumerator,int16_t inDenominator) {
	return ((uint16_t)inNumerator)<<16 | (uint16_t)inDenominator;
}
//static bool isValid(Rational inR) {
//	int16_t denominator=(int16_t)(inR & 0xFFFF);
//	return denominator!=0;
//}
static int16_t gcd(int16_t inA,int16_t inB) {
	if(inA==0) {
		return inB;
	} else if(inB==0) {
		return inA;
	}
	int16_t r;
	int16_t x=inA,y=inB;
	while((r=x%y)!=0) { x=y; y=r; }
	return y;
}
static Rational add(Rational inR1,Rational inR2) {
	// inR1=a/b
	int16_t a=(int16_t)(inR1>>16);
	int16_t b=(int16_t)(inR1 & 0xFFFF);

	// inR2=c/d
	int16_t c=(int16_t)(inR2>>16);
	int16_t d=(int16_t)(inR2 & 0xFFFF);

	//  a     c     ad+bc     x
	// --- + --- = ------- = ---
	//  b     d       bd      y
	int16_t x,y;
	if(b==d) {
		x=a+c;
		y=b;
	} else {
		x=a*d+b*c;
		y=b*d;
	}
	int16_t t=gcd(x,y);
	return MakeRational(x/t,y/t);
}
static Rational sub(Rational inR1,Rational inR2) {
	// inR1=a/b
	int16_t a=(int16_t)(inR1>>16);
	int16_t b=(int16_t)(inR1 & 0xFFFF);

	// inR2=c/d
	int16_t c=(int16_t)(inR2>>16);
	int16_t d=(int16_t)(inR2 & 0xFFFF);

	//  a     c     ad-bc     x
	// --- - --- = ------- = ---
	//  b     d       bd      y
	int16_t x,y;
	if(b==d) {
		x=a-c;
		y=b;
	} else {
		x=a*d-b*c;
		y=b*d;
	}
	int16_t t=gcd(x,y);
	return MakeRational(x/t,y/t);
}
static Rational mul(Rational inR1,Rational inR2) {
	// inR1=a/b
	int16_t a=(int16_t)(inR1>>16);
	int16_t b=(int16_t)(inR1 & 0xFFFF);

	// inR2=c/d
	int16_t c=(int16_t)(inR2>>16);
	int16_t d=(int16_t)(inR2 & 0xFFFF);

	//  a     c     ac     x
	// --- * --- = ---- = ---
	//  b     d     bd     y
	int16_t x=a*c;
	int16_t y=b*d;
	int16_t t=gcd(x,y);
	return MakeRational(x/t,y/t);
}
static float getRationalValue(Rational inRational) {
	int16_t a=(int16_t)(inRational>>16);
	int16_t b=(int16_t)(inRational & 0xFFFF);
	if(b==0) { return NAN; }
	return (float)a/b;
}

// ============================== MML ==============================
static void initMML(MmlInfo *outMmlInfo,const char *inMmlStr,int inMmlLength);
static void initMmlState(MmlState *ioMmlState);
static bool registerMML(int inChannel);
static bool isFinishMML(MmlInfo *inMML);
static bool checkMML(const char *inMmlString,int inMmlLength);
static bool parseMmlCommand(MmlInfo *ioMML,
							bool *outHasCommand,
							bool *outIsOutputToneInfo,
							float *outFreqHz,
							int16_t *outDurationMSec,float *outRingTimeScale,
							uint8_t *outVolume,
							bool inIsSupportRepeat=true);
static int checkNoteCommand(const char *inMmlString,int inStartPos,int inMmlLength,
							int *outShift,bool *outHasNatural,
						    Rational *outNoteLength,
							float *outRingTime,
							int *outStrength,
							Rational inDefaultLength,int inBaseStrength);
static int checkMusicalTransposition(const char *inMmlStr,int inStartPos,int inMmlLen,
									 int *outMusicalTransposition);
static int checkBaseStrength(const char *inMmlStr,int inStartPos,int inMmlLen,
							 int *outBaseStrength);
static int getNoteOffset(char c,int inMusicalTransposition);
static int checkNoteLength(const char *inMmlString,int inStartPos,int inMmlLength,
						   Rational *outNoteLength,
						   Rational inDefaultNoteLength);
static int checkNoteLengthTerm(const char *inMmlString,int inStartPos,int inMmlLength,
						   	   Rational *outNoteLength,Rational inDefaultNoteLength);
static int checkNoteLengthNumber(const char *inMmlString,int inStartPos,int inMmlLength,
								 Rational *outNoteLengthNumberValue,
								 Rational inDefaultNoteLength);
static int checkNoteLengthModifierFactor(const char *inMmlString,int inStartPos,
										 int inMmlLength,
										 Rational *outNoteLengthFactor);
static int checkNoteStrength(const char *inMmlString,int inStartPos,int inMmlLength,
							 int *outStrength,int inBaseStrength);

static int checkOctaveCommand(const char *inMmlString,int inStartPos,int inMmlLength,
							  int *outCurrentOctaveIndex);
static int checkTempoValue(const char *inMmlString,int inStartPos,int inMmlLength,
						   float *outTempoValue);

static int checkRational(const char *inMmlString,int inStartPos,int inMmlLength,
						 Rational *outRational);
static int checkNumber(const char *inMmlString,int inStartPos,float *outNumber);
static int checkInteger(const char *inMmlString,int inStartPos,int *outIntValue);
static bool isWhiteSpace(const char inChar);
static bool isNoteCommand(const char inChar);
static bool isMmlCommand(const char inChar);
static int skipWhiteSpace(const char *inMmlString,int inStartPos,int inMmlLength);
static void printMmlErrorInfo(const char *inMmlStr,int inIndex,int inMmlLen);

// t2kMML's MML grammer
// ----------------------------------------------------------------------------
// mml : white-space* (mml-command white-space*)+ white-space* ;
// white-space : [ \t\n\r] ;
// mml-command : note-command
// 			   | tempo-command
// note-command : ([A-G]|[a-g]) (white-space* [+-])?
// 					(white-space* note-length)? (white-space* note-strength)? ;
// note-length : note-length-term ([+-] note-length-term)* ;
// note-length-term : note-length-number
// 						(note-length-modifier)? (note-ring-time-modifier)? ;
// note-length-number : '' <- empty (means default length)
// 					  | (1|2|4|8|16|32|64|3|6|12|24|48|96)
// 					  ;
// note-length-modifier : (white-space* note-length-modifier-term)+ ;
// note-length-modifier-term : ('.' | "..")
// 							 | '_'+
// 							 | '/'+
// 							 ;
// note-ring-time-modifier : '*' ( INTEGER | FLOAT ) ;
// note-strength : ':' white-space* [+-]? white-space* INTEGER (white-space* [\'])?
// 				 | \'
// 				 ;

bool t2kMmlInit() {
	gFreqTable[88]=0;	// for rest
	const float s=pow(2,1/12.0f);
	float f=27.5f;
	gFreqTable[0]=f;
	f*=s;
	static const char *noteName[]={
	 //  0      1      2      3      4      5    
		"A  ", "A# ", "B  ", "C  ", "C# ", "D  ",
		"D# ", "E  ", "F  ", "F# ", "G  ", "G# ",
	 //  6      7      8      9      10     11
	};
	strcpy(gFreqNameStr[0],noteName[0]);
	gFreqNameStr[0][1]='0';
	for(int i=1; i<88; i++,f*=s) {
		if(i%12==0) {
			f=55*(1<<((i/12)-1));
		}
		gFreqTable[i]=f;
		strcpy(gFreqNameStr[i],noteName[i%12]);
		int t = gFreqNameStr[i][1]==' ' ? 1 : 2;
		gFreqNameStr[i][t]='0'+(i+9)/12;
	}

	gFreqNameStr[88][0]='R';
	gFreqNameStr[88][1]='\0';

	for(int i=0; i<kNumOfChannels; i++) { gMmlInfo[i].isAlive=false; }

#ifdef TEST_ON_PC
	for(int i=0; i<89; i++) {
		printf("i=%d freq=%f name=%s\n",i,gFreqTable[i],gFreqNameStr[i]);
	}
#endif

	return true;
}

bool t2kCheckMML(const char *inMmlString) {
	if(inMmlString==NULL) {
		ERROR("ERROR t2kSetMML: MML is NULL.\n");	
		return false;
	}
	int mmlLen=strlen(inMmlString);
	if(mmlLen==0) {
		ERROR("ERROR t2kSetMML: MML is empty.");	
		return false;
	}
	if(checkMML(inMmlString,mmlLen)==false) {
		ERROR("ERROR t2kSetMML: invalid MML\n");	
		return false;
	}
	return true;
}

bool t2kPlayMML(uint8_t inChannel,const char *inMmlString) {
	if(inChannel>=kNumOfChannels) {
		ERROR("ERROR t2kPlayMML: invalid channel=%d\n",inChannel);
		ERROR("                  channel should be in [0,%d).\n",kNumOfChannels);
		return false;
	}
	MmlInfo *mml=gMmlInfo+inChannel;
	int mmlLen=strlen(inMmlString);
	initMML(mml,inMmlString,mmlLen);
//Serial.printf("musical transposition=%d\n",mml->mmlState.musicalTransposition);

	t2kClearToneSeq(inChannel);
	registerMML(inChannel);

	mml->nowPlaying=true;
	mml->readyToPlay=true;
	t2kStartToneSeq(inChannel);
	return true;
}

bool t2kStopMML(uint8_t inChannel) {
	if(inChannel>=kNumOfChannels) {
		ERROR("ERROR t2kPlayMML: invalid channel=%d\n",inChannel);
		ERROR("                  channel should be in [0,%d).\n",kNumOfChannels);
		return false;
	}
	gMmlInfo[inChannel].isAlive=false;
	t2kClearToneSeq(inChannel);
	return true;
}

void t2kStopMMLs() {
	for(uint8_t i=0; i<kNumOfChannels; i++) {
		t2kStopMML(i);
	}
}

void t2kUpdateMML() {
	bool needUpdateAgain;
	uint16_t totalDurationMSec[kNumOfChannels];
	bool scoreQueueIsFull[kNumOfChannels];
	for(int i=0; i<kNumOfChannels; i++) {
		totalDurationMSec[i]=0;
		scoreQueueIsFull[i]=false;
	}
	do {
		needUpdateAgain=false;
		for(int ch=0; ch<kNumOfChannels; ch++) {
			MmlInfo *mml=gMmlInfo+ch;
			if(mml->isAlive==false || mml->nowPlaying==false) { continue; }
			if( isFinishMML(mml)) {
				if(mml->repeatStartIndex<0) {
					mml->nowPlaying=false;
					mml->isAlive=false;
					continue;
				} else {
					mml->nextMmlCharIndex=mml->repeatStartIndex;
					// Serial.printf("MML: repeat (restart pos=%d)\n",mml->repeatStartIndex);
				}
			}
			if(mml->mmlState.unsendRestDurationMSec>0) {
				if( t2kAddTone(ch,0,mml->mmlState.unsendRestDurationMSec,0) ) {
					mml->mmlState.unsendRestDurationMSec=-1;
					totalDurationMSec[ch]+=mml->mmlState.unsendRestDurationMSec;
					needUpdateAgain=true;
				}
				continue;
			}

			int indexBackup=mml->nextMmlCharIndex;
			float freqHz;
			int16_t durationMSec;
			float ringTimeScale;
			uint8_t volume;
			bool hasCommand;
			bool isOutputToneInfo;
			if(parseMmlCommand(mml,&hasCommand,
							   &isOutputToneInfo,
							   &freqHz,&durationMSec,&ringTimeScale,&volume)==false) {
				mml->isAlive=false;
				continue;
			}	
			if(ringTimeScale>1) { ringTimeScale=1; }
			if(ringTimeScale<1) {
				mml->mmlState.unsendRestDurationMSec=durationMSec*(1-ringTimeScale);			
			}
			if( isOutputToneInfo ) {
//Serial.printf("updateMML: ch=%d freq=%f duration=%d volume=%d\n",ch,freqHz,durationMSec,volume);
				if(t2kAddTone(ch,freqHz,durationMSec,volume)==false) {
					mml->nextMmlCharIndex=indexBackup;
					mml->mmlState.unsendRestDurationMSec=-1; // cansel rest.
					scoreQueueIsFull[ch]=true;
				} else {
					needUpdateAgain=true;
					totalDurationMSec[ch]+=durationMSec;
				}
			}
		}
		if( needUpdateAgain ) {
			bool enough=true;
			for(int i=0; i<kNumOfChannels; i++) {
				// check buffering 60 msec
				if(gMmlInfo[i].isAlive && totalDurationMSec[i]<kBufferingMSec) {
					enough=false;
					break;
				}
			}
			if( enough ) { break; }

			enough=true;
			for(int i=0; i<kNumOfChannels; i++) {
				if(gMmlInfo[i].isAlive && scoreQueueIsFull[i]==false) {
					enough=false;
				}
			}
			if( enough ) { break; }
		}
	} while( needUpdateAgain );
}

static void initMML(MmlInfo *outMmlInfo,const char *inMmlStr,int inMmlLength) {
	outMmlInfo->isAlive=true;
	outMmlInfo->mmlStr=inMmlStr;
	outMmlInfo->mmlStrLength=inMmlLength;
	outMmlInfo->nextMmlCharIndex=0;
	outMmlInfo->repeatStartIndex=-1;
	outMmlInfo->nowPlaying=false;
	outMmlInfo->readyToPlay=false;
	initMmlState(&outMmlInfo->mmlState);
}
static void initMmlState(MmlState *ioMmlState) {
	ioMmlState->tempo=120;
	ioMmlState->initialTempo=-1;
	ioMmlState->musicBeat=MakeRational(4,4);
	ioMmlState->musicalTransposition=0;
	ioMmlState->currentOctaveIndex=39;	// C4
	ioMmlState->defaultLength=MakeRational(1,4);
	ioMmlState->baseStrength=90;
	ioMmlState->lengthSubTotal=MakeRational(0,1);
	ioMmlState->unsendRestDurationMSec=0;
}
static bool registerMML(int inChannel) {
	float freqHz;
	int16_t durationMSec;
	float ringTimeScale;
	uint8_t volume;
	bool hasCommand;
	bool isOutputToneInfo;
	bool isAnyCommands=false;
	MmlInfo *mml=gMmlInfo+inChannel;

	int16_t totalDurationMSec=0;

again:
	while(isFinishMML(mml)==false) {
		if(mml->mmlState.unsendRestDurationMSec>0) {
			if( t2kAddTone(inChannel,0,mml->mmlState.unsendRestDurationMSec,0) ) {
				mml->mmlState.unsendRestDurationMSec=-1;
			} else {
				return true;
			}
		}
		int i=mml->nextMmlCharIndex;
		if(parseMmlCommand(mml,&hasCommand,
						   &isOutputToneInfo,
						   &freqHz,&durationMSec,&ringTimeScale,&volume)==false) {
			mml->nextMmlCharIndex=i;
			return false;
		}			
		isAnyCommands |= hasCommand;
		if(ringTimeScale>1) { ringTimeScale=1; }
		if(ringTimeScale<1) {
			mml->mmlState.unsendRestDurationMSec=durationMSec*(1-ringTimeScale);			
		}
		if( isOutputToneInfo ) {
			if(t2kAddTone(inChannel,freqHz,durationMSec,volume)==false) {
				mml->nextMmlCharIndex=i;
				mml->mmlState.unsendRestDurationMSec=-1;
				return true;
			}
			totalDurationMSec+=durationMSec;
			if(totalDurationMSec>kBufferingMSec) {	// buffering 60msec
				break;
			}
		}
	}
	if( isFinishMML(mml) && mml->repeatStartIndex>=0) {
		mml->nextMmlCharIndex=mml->repeatStartIndex;
		goto again;
	}
	return isAnyCommands;
}
static bool isFinishMML(MmlInfo *inMML) {
	return inMML->nextMmlCharIndex>=inMML->mmlStrLength;
}
static bool checkMML(const char *inMmlString,int inMmlLength) {
	assert(inMmlLength>0);

	MmlInfo mml;
	initMML(&mml,inMmlString,inMmlLength);

	float freqHz;
	int16_t durationMSec;
	float ringTimeScale;
	uint8_t volume;
	bool hasCommand;
	bool isOutputToneInfo;
	bool isAnyCommands=false;
	while(isFinishMML(&mml)==false) {
		if(parseMmlCommand(&mml,&hasCommand,
						   &isOutputToneInfo,
						   &freqHz,&durationMSec,&ringTimeScale,&volume,
						   false)==false) {
#ifdef TEST_ON_PC
	printf("ERROR at index=%d\n",mml.nextMmlCharIndex);
#endif
			return false;
		}			
		isAnyCommands|=hasCommand;
	}
	return isAnyCommands;
}
static bool parseMmlCommand(MmlInfo *ioMML,
							bool *outHasCommand,
							bool *outIsOutputToneInfo,
							float *outFreqHz,
							int16_t *outDurationMSec,float *outRingTimeScale,
							uint8_t *outVolume,
							bool inIsSupportRepeat) {
	int i=ioMML->nextMmlCharIndex;
	const char *mmlStr=ioMML->mmlStr;
	int mmlLen=ioMML->mmlStrLength;
	i=skipWhiteSpace(mmlStr,i,mmlLen);
	if(i>=mmlLen) {
		if(outHasCommand!=NULL) { *outHasCommand=false; }
		return true;
	}
	if(isMmlCommand(mmlStr[i])==false) {
		ERROR("MML ERROR: invalid MML command '%c' (index=%d).\n",mmlStr[i],i);
		printMmlErrorInfo(mmlStr,i,mmlLen);
		return false;
	}

	bool isOutputToneInfo=false;
	float freqHz=-1;
	int16_t durationMSec=0;
	float ringTimeScale=0;
	uint8_t volume=0;

	char c=mmlStr[i];
	if( isNoteCommand(c) || c=='R' || c=='r') {
		char noteCommand=c;
		int offset;
		if(c=='R' || c=='r') {
			offset=88;
		} else {
			offset=getNoteOffset(noteCommand,ioMML->mmlState.musicalTransposition);
		}
		if(offset<0) {
			ERROR("MML ERROR: getNoteOffset in parseMmlCommand.\n");
			printMmlErrorInfo(mmlStr,i,mmlLen);
			return false;
		}
		i=skipWhiteSpace(mmlStr,i+1,mmlLen);
		c=mmlStr[i];
		int tmpOctaveShift=0;
		if(c=='^') {
			tmpOctaveShift=12;
			i++;
		} else if(c=='v' || c=='V') {
			tmpOctaveShift=-12;
			i++;
		}
		int shift;
		Rational noteLength;
		int strength;
		bool hasNatural;
		int t=checkNoteCommand(mmlStr,i,mmlLen,
							   &shift,&hasNatural,
							   &noteLength,&ringTimeScale,&strength,
							   ioMML->mmlState.defaultLength,
							   ioMML->mmlState.baseStrength);
		if(t<0) {
			ERROR("MML ERROR: checkNoteCommand (index=%d).\n",i);
			printMmlErrorInfo(mmlStr,i,mmlLen);
			return false;
		}			
		i=t;
		if(hasNatural) {
			offset=getNoteOffset(noteCommand,0);	// natural
			i=skipWhiteSpace(mmlStr,i,mmlLen);		// i indexed next term already.
			c=mmlStr[i];
		}
		ioMML->mmlState.lengthSubTotal=add(ioMML->mmlState.lengthSubTotal,noteLength);
		int freqIndex;
		if(offset!=88) {
			freqIndex=ioMML->mmlState.currentOctaveIndex+tmpOctaveShift+offset+shift;
		} else {
			freqIndex=88;
		}
		if(freqIndex<0 || 89<=freqIndex) {
			ERROR("MML ERROR: invalid frexIndex (around index=%d).\n",i);
			printMmlErrorInfo(mmlStr,i,mmlLen);
			return false;
		}

		isOutputToneInfo=true;
		freqHz=gFreqTable[freqIndex];
		durationMSec=getRationalValue(noteLength)*4*60/ioMML->mmlState.tempo*1000;
		volume=(uint8_t)(strength/127.0*255);

#ifdef TEST_ON_PC
	printf("octave=%d offset=%d shift=%d\n",
		   ioMML->mmlState.currentOctaveIndex,offset,shift);
	printf("Note=%s\n",gFreqNameStr[freqIndex]);
	printf("noteLength=");printRational(noteLength);
	printf("noteLength in float=%f\n",getRationalValue(noteLength));
	printf("duration msec=%d\n",durationMSec);
	printf("tempo=%f\n",ioMML->mmlState.tempo);
#endif
	} else if(c=='@') {
		i=skipWhiteSpace(mmlStr,i+1,mmlLen);
		c=mmlStr[i];
		switch(c) {
			case 'K':	// @K or @k
			case 'k': {
					int musicalTransposition;
					i=checkMusicalTransposition(mmlStr,i+1,mmlLen,&musicalTransposition);
					if(i<0) { return false; }
//Serial.printf("UPDATE musical tarnsposition=%d\n",musicalTransposition);
					ioMML->mmlState.musicalTransposition=musicalTransposition;
				}
				break;
			case 'T':	// @T or @t
			case 't': {
					i=skipWhiteSpace(mmlStr,i+1,mmlLen);
					int numerator;
					i=checkInteger(mmlStr,i,&numerator);
					if(i<0) { return false; }
					i=skipWhiteSpace(mmlStr,i,mmlLen);
					c=mmlStr[i];
					if(c!='/') { return false; }
					i=skipWhiteSpace(mmlStr,i+1,mmlLen);
					int denominator;
					i=checkInteger(mmlStr,i,&denominator);
					if(i<0) { return false; }
					if(denominator!=4 && denominator!=8) { return false; }	
					ioMML->mmlState.musicBeat=MakeRational(numerator,denominator);
#ifdef TEST_ON_PC
	printf("music beat=%d/%d\n",numerator,denominator);
#endif
				}
				break;
			case 'M':	// @M or @m : set tempo
			case 'm': {
					i=skipWhiteSpace(mmlStr,i+1,mmlLen);
					// int startPos=i;
					c=mmlStr[i];
					if(c=='=') {
						if(ioMML->mmlState.initialTempo<0) {
							ioMML->mmlState.initialTempo=120;
						}
						ioMML->mmlState.tempo=ioMML->mmlState.initialTempo;
						i=skipWhiteSpace(mmlStr,i+1,mmlLen);
					} else if(c=='*') {
						float tempoScale;
						i=checkTempoValue(mmlStr,i+1,mmlLen,&tempoScale);
						if(i<0) { return false; }
						if(ioMML->mmlState.initialTempo<0) {
							ioMML->mmlState.initialTempo=120;
						}
						ioMML->mmlState.tempo*=tempoScale;
					} else if(c=='/') {
						float tempoScale;
						i=checkTempoValue(mmlStr,i+1,mmlLen,&tempoScale);
						if(i<0) { return false; }
						if(ioMML->mmlState.initialTempo<0) {
							ioMML->mmlState.initialTempo=120;
						}
						ioMML->mmlState.tempo/=tempoScale;
					} else {
						float tempoValue;
						i=checkTempoValue(mmlStr,i,mmlLen,&tempoValue);
						if(i<0) { return false; }
						if(ioMML->mmlState.initialTempo<0) {
							ioMML->mmlState.initialTempo=tempoValue;
						}
						ioMML->mmlState.tempo=tempoValue;
					}
#ifdef TEST_ON_PC
	printf("tempo=%f\n",ioMML->mmlState.tempo);
#endif
				}
				break;
			case 'V':	// @V or @v : set default strength
			case 'v': {
					int baseStrength;
					i=checkBaseStrength(mmlStr,i+1,mmlLen,&baseStrength);
					if(i<0) { return false; }
#ifdef TEST_ON_PC
	printf("Set Default Strength: %d\n",baseStrength);
#endif
					ioMML->mmlState.baseStrength=baseStrength;
				}
				break;
		}
	} else {
		isOutputToneInfo=false;
		switch(c) {
			case '%': i=ioMML->mmlStrLength; break;
			case '&': // just ignore
				i++; break;
			case 'O':
			case 'o': {
					int octaveIndex;
					i=checkOctaveCommand(mmlStr,i+1,mmlLen,&octaveIndex);
					if(i<0) { return false; }
#ifdef TEST_ON_PC
	printf("OCTAVE COMMAND: now octaveLevelIndex=%d\n",octaveIndex);
#endif
					ioMML->mmlState.currentOctaveIndex=octaveIndex;
				}
				break;
			case '<': {
					int octaveIndex=ioMML->mmlState.currentOctaveIndex+12;
					if(octaveIndex>87) { octaveIndex=87; }
#ifdef TEST_ON_PC
	printf("OCTAVE UP: now octaveLevelIndex %d ->%d\n",ioMML->mmlState.currentOctaveIndex,octaveIndex);
#endif
					ioMML->mmlState.currentOctaveIndex=octaveIndex;
				}
				i++;
				break;
			case '>': {
					int octaveIndex=ioMML->mmlState.currentOctaveIndex-12;
					if(octaveIndex<-9) { octaveIndex=-9; }
#ifdef TEST_ON_PC
	printf("OCTAVE DOWN: now octaveLevelIndex=%d\n",octaveIndex);
#endif
					ioMML->mmlState.currentOctaveIndex=octaveIndex;
				}
				i++;
				break;
			case 'L':
			case 'l': {
					Rational defaultLength;
					i=checkNoteLength(mmlStr,i+1,mmlLen,
									  &defaultLength,ioMML->mmlState.defaultLength);
					if(i<0) { return false; }
#ifdef TEST_ON_PC
	printf("Set Default Length: "); printRational(defaultLength);
#endif
					ioMML->mmlState.defaultLength=defaultLength;
				}
				break;
			case 'V':
			case 'v': {
					int baseStrength;
					i=checkBaseStrength(mmlStr,i+1,mmlLen,&baseStrength);
					if(i<0) { return false; }
#ifdef TEST_ON_PC
	printf("Set Default Strength: %d\n",baseStrength);
#endif
					ioMML->mmlState.baseStrength=baseStrength;
				}
				break;
			// N float == float value means frequency [Hz]
			// ex: N440 <-- A
			case 'N':
			case 'n': {
					i=checkNumber(mmlStr,i+1,&freqHz);
					if(i<0) { return false; }
					isOutputToneInfo=true;
					durationMSec=getRationalValue(ioMML->mmlState.defaultLength)*4*60
								 /ioMML->mmlState.tempo*1000;
					ringTimeScale=1;
					volume=(uint8_t)(ioMML->mmlState.baseStrength/127.0*255);
				}
				break;
			case 'T':
			case 't': {
					float tempoValue;
					i=skipWhiteSpace(mmlStr,i+1,mmlLen);
					i=checkTempoValue(mmlStr,i,mmlLen,&tempoValue);
					if(i<0) { return false; }
					if(ioMML->mmlState.initialTempo<0) {
						ioMML->mmlState.initialTempo=tempoValue;
					}
					ioMML->mmlState.tempo=tempoValue;
#ifdef TEST_ON_PC
	printf("tempo=%f\n",ioMML->mmlState.tempo);
#endif
				}
				break;
			case '$':
				i++;
				if(inIsSupportRepeat==false) {
					// no nothing (just ignore)
				} else {
					ioMML->repeatStartIndex=i;
				}
				break;
		}
	}
	ioMML->nextMmlCharIndex=i;

	if(outHasCommand!=NULL) { *outHasCommand=true; }
	if(outIsOutputToneInfo!=NULL) { *outIsOutputToneInfo=isOutputToneInfo; }
	if(outFreqHz!=NULL) { *outFreqHz=freqHz; }
	if(outDurationMSec!=NULL) { *outDurationMSec=durationMSec; }
	if(outRingTimeScale!=NULL) { *outRingTimeScale=ringTimeScale; }
	if(outVolume!=NULL) { *outVolume=volume; }
	return true;
}
// inMmlString[inStartPos-1] is in [A-G] or [a-g] or R or r.
static int checkNoteCommand(const char *inMmlString,int inStartPos,int inMmlLength,
							int *outShift,bool *outHasNatural,
						    Rational *outNoteLength,
							float *outRingTime,
							int *outStrength,
							Rational inDefaultLength,int inBaseStrength) {
	int i=inStartPos;
	float ringTime=1.0f;
	int strength=inBaseStrength;
	int shift=0;
	bool hasNatural=false;

	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	char c=inMmlString[i];
	if(c=='+') {
		for(; inMmlString[i]=='+'; i=skipWhiteSpace(inMmlString,i+1,inMmlLength)) {
			shift++;
		}
	} else if(c=='-') {
		for(; inMmlString[i]=='-'; i=skipWhiteSpace(inMmlString,i+1,inMmlLength)) {
			shift--;
		}
	} else if(c=='=') {
		hasNatural=true;
		i++;
	}
	Rational noteLength;
	i=checkNoteLength(inMmlString,i,inMmlLength,&noteLength,inDefaultLength);
	if(i<0) { return -1; }

	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	if(inMmlString[i]=='*') {
		i++;
		i=skipWhiteSpace(inMmlString,i,inMmlLength);
		i=checkNumber(inMmlString,i,&ringTime);
		if(i<0) { return -1; }
	}	

	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	if(inMmlString[i]==':') {
		i++;
		i=skipWhiteSpace(inMmlString,i,inMmlLength);
		i=checkNoteStrength(inMmlString,i,inMmlLength,&strength,inBaseStrength);
		if(i<0) { return -1; }
	} else if(inMmlString[i]=='\'') {
		i++;
		strength=inBaseStrength+20;
	}
#ifdef TEST_ON_PC
	printf("checkNoteCommand::noteLength=");printRational(noteLength);
	printf("                  ringTime=%f\n",ringTime);
	printf("                  strength=%d\n",strength);
	printf("                  shift   =%d\n",shift);
	printf("                hasNatural=%d\n",hasNatural);
#endif
	if(outShift     !=NULL) { *outShift     =shift;      }
	if(outHasNatural!=NULL) { *outHasNatural=hasNatural; }
	if(outNoteLength!=NULL) { *outNoteLength=noteLength; }
	if(outRingTime  !=NULL) { *outRingTime  =ringTime;   }
	if(outStrength  !=NULL) { *outStrength  =strength;   }

	return i;
}
static int checkMusicalTransposition(const char *inMmlStr,int inStartPos,int inMmlLen,
									 int *outMusicalTransposition) {
	int i=inStartPos;
	int musicalTransposition;
	i=skipWhiteSpace(inMmlStr,i,inMmlLen);
	char c=inMmlStr[i];
	if(c=='+') {
// Serial.printf("checkMusicalTransposition[+] - IN index=%d\n",inStartPos);
		i=skipWhiteSpace(inMmlStr,i+1,inMmlLen);
		i=checkInteger(inMmlStr,i,&musicalTransposition);
		if(i<0) {
			ERROR("ERROR checkMusicalTransposition: "
				  "musical transposition command @K+ or @k+ needs integer "
				  "(MML index=%d)\n",inStartPos);
			printMmlErrorInfo(inMmlStr,i,inMmlLen);
			return -1;
		}
		if(musicalTransposition>7) {
			ERROR("ERROR checkMusicalTransposition: "
				  "musical tarnsposition command @K or @k only support "
				  "-7 to +7. current transposition is %d"
				  "(MML index=%d)\n",musicalTransposition,inStartPos);
			printMmlErrorInfo(inMmlStr,i,inMmlLen);
			return -1;
		}
	} else if(c=='-') {
// Serial.printf("checkMusicalTransposition[-] - IN index=%d\n",inStartPos);
		i=skipWhiteSpace(inMmlStr,i+1,inMmlLen);
		i=checkInteger(inMmlStr,i,&musicalTransposition);
		if(i<0) {
			ERROR("ERROR checkMusicalTransposition: "
				  "musical transposition command @K- or @k- needs integer "
				  "(MML index=%d)\n",inStartPos);
			printMmlErrorInfo(inMmlStr,i,inMmlLen);
			return -1;
		}
		if(musicalTransposition>7) {
			ERROR("ERROR checkMusicalTransposition: "
				  "musical tarnsposition command @K or @k only support "
				  "-7 to +7. current transposition is %d"
				  "(MML index=%d)\n",musicalTransposition,inStartPos);
			printMmlErrorInfo(inMmlStr,i,inMmlLen);
			return -1;
		}
		musicalTransposition*=-1;;
	} else if(c=='=') {	// @K= or @k= indicates no musical transposition.
// Serial.printf("checkMusicalTransposition[=] - IN index=%d\n",inStartPos);
		i++;
		musicalTransposition=0;
	} else {
		ERROR("ERROR checkMusicalTransposition: "
			  "musical transposition command @K or @k needs follow char "
			  "+,- or =. now the char is '%c' (MML index=%d)\n",
			  c,inStartPos);
		printMmlErrorInfo(inMmlStr,i,inMmlLen);
		return -1;
	}
	if(outMusicalTransposition!=NULL) {
		*outMusicalTransposition=musicalTransposition;
	}
	return i;
}

static int checkBaseStrength(const char *inMmlStr,int inStartPos,int inMmlLen,
							 int *outBaseStrength) {
	int i=inStartPos;
	int baseStrength;
	i=skipWhiteSpace(inMmlStr,i,inMmlLen);
	if(inMmlStr[i]==':') { i=skipWhiteSpace(inMmlStr,i+1,inMmlLen); }
	i=checkInteger(inMmlStr,i,&baseStrength);
	if(i<0) {
		ERROR("ERROR checkBaseStrength: "
			  "base strength command V,v or @V or @V needs integer value."
			  "(MML index=%d).\n",inStartPos);
		return -1;
	}
	if(baseStrength>128) { baseStrength=127; }
	if(outBaseStrength!=NULL) { *outBaseStrength=baseStrength; }
	return i;
}


static int gMusicalTranspositionOffset[15][7]={
	// A  B  C  D  E  F  G
	{ -1,-1,-1,-1,-1,-1,-1,},	// -7 == Cb major
	{ -1,-1,-1,-1,-1, 0,-1,},	// -6 == Gb major
	{ -1,-1, 0,-1,-1, 0,-1,},	// -5 == Db major
	{ -1,-1, 0,-1,-1, 0, 0,},	// -4 == Ab major
	{ -1,-1, 0, 0,-1, 0, 0,},	// -3 == Eb major
	{  0,-1, 0, 0,-1, 0, 0,},	// -2 == Bb major
	{  0,-1, 0, 0, 0, 0, 0,},	// -1 == F  major
	{  0, 0, 0, 0, 0, 0, 0,},	//  0 == C  major
	{  0, 0, 0, 0, 0,+1, 0,},	// +1 == G  major
	{  0, 0,+1, 0, 0,+1, 0,},	// +2 == D  major
	{  0, 0,+1, 0, 0,+1,+1,},	// +3 == A  major
	{  0, 0,+1,+1, 0,+1,+1,},	// +4 == E  major
	{ +1, 0,+1,+1, 0,+1,+1,},	// +5 == B  major 
	{ +1, 0,+1,+1,+1,+1,+1,},	// +6 == F# major
	{ +1,+1,+1,+1,+1,+1,+1,},	// +7 == C# major
};
static int getNoteOffset(char c,int inMusicalTransposition) {
	int t=7+inMusicalTransposition;
	switch(c) {
		case 'A':
		case 'a':
			return 9+gMusicalTranspositionOffset[t][0];
		case 'B':
		case 'b':
			return 11+gMusicalTranspositionOffset[t][1];
		case 'C':
		case 'c':
			return 0+gMusicalTranspositionOffset[t][2];
		case 'D':
		case 'd':
			return 2+gMusicalTranspositionOffset[t][3];
		case 'E':
		case 'e':
			return 4+gMusicalTranspositionOffset[t][4];
		case 'F':
		case 'f':
			return 5+gMusicalTranspositionOffset[t][5];
		case 'G':
		case 'g':
			return 7+gMusicalTranspositionOffset[t][6];
		default:
#ifdef TEST_ON_PC
	printf("!!!!! SYSTEM ERROR !!!!!\n");
#endif
			return -1;
	}
}
static int checkNoteLength(const char *inMmlString,int inStartPos,int inMmlLength,
						   Rational *outNoteLength,Rational inDefaultNoteLength) {
	int i=inStartPos;
	Rational noteLength=MakeRational(0,1);	// =0=0/1

	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	char c=inMmlString[i];
	if(c!='.' && c!='_' && c!='/' && c!='*' && (c<'0' || '9'<c)) {
		if(outNoteLength!=NULL) { *outNoteLength=inDefaultNoteLength; }
		return i;
	}

	Rational noteTermLength;
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	i=checkNoteLengthTerm(inMmlString,i,inMmlLength,
						  &noteTermLength,inDefaultNoteLength);
	if(i<0) { return -1; }
	noteLength=add(noteLength,noteTermLength);

	for(;;) {
		i=skipWhiteSpace(inMmlString,i,inMmlLength);
		if(inMmlString[i]=='+') {
			i++;
			i=skipWhiteSpace(inMmlString,i,inMmlLength);
			i=checkNoteLengthTerm(inMmlString,i,inMmlLength,
								  &noteTermLength,inDefaultNoteLength);
			if(i<0) { return -1; }
			noteLength=add(noteLength,noteTermLength);
		} else if(inMmlString[i]=='-') {
			i++;
			i=skipWhiteSpace(inMmlString,i,inMmlLength);
			i=checkNoteLengthTerm(inMmlString,i,inMmlLength,
								  &noteTermLength,inDefaultNoteLength);
			if(i<0) { return -1; }
			noteLength=sub(noteLength,noteTermLength);
		} else {
			if(outNoteLength!=NULL) { *outNoteLength=noteLength; }
			return i;
		}
	}
}
static int checkNoteLengthTerm(const char *inMmlString,int inStartPos,int inMmlLength,
						   	   Rational *outNoteLength,Rational inDefaultNoteLength) {
	int i=inStartPos;
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	char c=inMmlString[i];
	Rational noteLength;
	if(c<'0' || '9'<c) {
		noteLength=inDefaultNoteLength;
	} else {
		i=checkNoteLengthNumber(inMmlString,i,inMmlLength,
								&noteLength,inDefaultNoteLength);
		if(i<0) { return -1; }
	}
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	c=inMmlString[i];
	if(c!='.' && c!='/' && c!='_') {
		if(outNoteLength!=NULL) { *outNoteLength=noteLength; }
		return i;
	}
	Rational noteLengthModifierFactor;
	i=checkNoteLengthModifierFactor(inMmlString,i,inMmlLength,&noteLengthModifierFactor);
	if(i<0) { return -1; }
	
	noteLength=mul(noteLength,noteLengthModifierFactor);
	if(outNoteLength!=NULL) { *outNoteLength=noteLength; }
	return i;
}
static int checkNoteLengthNumber(const char *inMmlString,int inStartPos,int inMmlLength,
								 Rational *outNoteLengthNumberValue,
								 Rational inDefaultNoteLength) {
	int i=inStartPos;
	char c=inMmlString[i];
	Rational lengthNumber;
	switch(c) {
		case '1':	// 1 or 16 or 12
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,1);
				i++;
				goto leave;
			} else if(c=='6') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,16);	// == 1/16
				i++;
				goto leave;
			} else if(c=='2') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,12);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '2':	// 2 or 24
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,2);
				i++;
				goto leave;
			} else if(c=='4') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,24);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '3':	// 3 or 32
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,3);
				i++;
				goto leave;
			} else if(c=='2') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,32);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '4':	// 4 or 48
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,4);
				i++;
				goto leave;
			} else if(c=='8') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,48);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '6':	// 6 or 64
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,6);
				i++;
				goto leave;
			} else if(c=='4') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,64);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '8':	// only 8
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,8);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		case '9':	// 9 or 96
			c=inMmlString[i+1];
			if(c<'0' || '9'<c) {
				lengthNumber=MakeRational(1,9);
				i++;
				goto leave;
			} else if(c=='6') {
				i++;
				c=inMmlString[i+1];
				if('0'<=c && c<='9') { goto onError; }
				lengthNumber=MakeRational(1,96);
				i++;
				goto leave;
			} else {
				goto onError;
			}
			break;
		default:
			goto onError;
	}
leave:
	if(outNoteLengthNumberValue!=NULL) {
		*outNoteLengthNumberValue=lengthNumber;
	}
	return i;

onError:
	ERROR("MML ERROR: invalid note length number (around index=%d).\n",i);
	printMmlErrorInfo(inMmlString,i,inMmlLength);
	return -1;
}
static int checkNoteLengthModifierFactor(const char *inMmlString,int inStartPos,
										 int inMmlLength,
										 Rational *outNoteLengthFactor) {
	int i=inStartPos;
	char c;
	Rational factor=MakeRational(1,1);

	for(;;) {
	   	c=inMmlString[i];
		if(c=='.') {
			i++;
			c=inMmlString[i];
			if(c!='.') {
				factor=mul(factor,MakeRational(3,2));
				goto leave;
			} else {
				// '..'
				i++;
				factor=mul(factor,MakeRational(7,4));
			}
		} else if(c=='_') {
			int n;
			for(n=1; inMmlString[i]=='_'; i++,n*=2) {
				// empty;
			}
			factor=mul(factor,MakeRational(n,1));
		} else if(c=='/') {
			int d;
			for(d=1; inMmlString[i]=='/'; i++,d*=2) {
				// empty;
			}
			factor=mul(factor,MakeRational(1,d));
		} else {
			break;
		}
	}
leave:
	if(outNoteLengthFactor!=NULL) {
		*outNoteLengthFactor=factor;
	}
	return i;
}
static int checkNoteStrength(const char *inMmlString,int inStartPos,int inMmlLength,
							 int *outStrength,int inBaseStrength) {
	int strength=inBaseStrength;
	int i=inStartPos;
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	char c=inMmlString[i];
	if(c=='+') {
		i++;
		int delta;
		i=checkInteger(inMmlString,i,&delta);
		if(i<0) { return -1; }
		strength+=delta;
	} else if(c=='-') {
		i++;
		int delta;
		i=checkInteger(inMmlString,i,&delta);
		if(i<0) { return -1; }
		strength-=delta;
	} else {
		c=inMmlString[i];
		if('0'<=c && c<='9') {
			i=checkInteger(inMmlString,i,&strength);
			if(i<0) { return -1; }
		}
	}
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	if(inMmlString[i]=='\'') {
		strength+=20;
		i++;
	}
	if(outStrength!=NULL) { *outStrength=strength; }
	return i;
}

static int checkOctaveCommand(const char *inMmlString,int inStartPos,int inMmlLength,
							  int *outCurrentOctaveIndex) {
	int i=inStartPos;
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	int octaveNumber;
	i=checkInteger(inMmlString,i,&octaveNumber);
	if(i<0) {
		ERROR("ERROR checkOctaveCommand: octave command 'O' or 'o' "
			  "should have integer value. "
			  "(mml str index=%d)\n",inStartPos);
		return -1;
	}
	if(octaveNumber<0) { octaveNumber=0; }
	if(8<octaveNumber) { octaveNumber=8; }
	int octaveIndex=octaveNumber*12-9;
	if(outCurrentOctaveIndex!=NULL) { *outCurrentOctaveIndex=octaveIndex; }
	return i;
}

static int checkTempoValue(const char *inMmlString,int inStartPos,int inMmlLength,
						   float *outTempoValue) {
	int startPos=skipWhiteSpace(inMmlString,inStartPos,inMmlLength);
	int i=startPos;
	Rational rational;
	i=checkRational(inMmlString,i,inMmlLength,&rational);
	float tempoNumber;
	if(i<0) {
		i=checkNumber(inMmlString,startPos,&tempoNumber);
		if(i<0) { return -1; }
	} else {
		tempoNumber=getRationalValue(rational);
	}
	if(outTempoValue!=NULL) { *outTempoValue=tempoNumber; }
	return i;
}

static int checkRational(const char *inMmlString,int inStartPos,int inMmlLength,
						 Rational *outRational) {
	int i=inStartPos;
	int numerator;
	i=checkInteger(inMmlString,i,&numerator);
	if(i<0) { return -1; }
	i=skipWhiteSpace(inMmlString,i,inMmlLength);
	if(inMmlString[i]!='/') { return -1; }
	i=skipWhiteSpace(inMmlString,i+1,inMmlLength);
	int denominator;
	i=checkInteger(inMmlString,i,&denominator);
	if(i<0) { return -1; }
	if(outRational!=NULL) { *outRational=MakeRational(numerator,denominator); }
	return i;
}
static int checkNumber(const char *inMmlString,int inStartPos,float *outNumber) {
	int i=inStartPos;
	float number=0;
	char c;
	for(;; i++,number=number*10+c-'0') {
		c=inMmlString[i];
		if(c<'0' || '9'<c) { break; }
	}
	if(c=='.') {
		float s=1.0f;
		for(i++; ; i++,number+=(c-'0')*s) {
			c=inMmlString[i];
			if(c<'0' || '9'<c) { break; }
			s*=0.1f;
		}
	}
	if(outNumber!=NULL) { *outNumber=number; }
	return i;
}
static int checkInteger(const char *inMmlString,int inStartPos,int *outIntValue) {
	int i=inStartPos;
	int number=0;
	char c;
	for(;; i++,number=number*10+c-'0') {
		c=inMmlString[i];
		if(c<'0' || '9'<c) { break; }
	}
	if(outIntValue!=NULL) { *outIntValue=number; }
	return i;
}
static bool isWhiteSpace(const char inChar) {
	return inChar==' ' || inChar=='\t' || inChar=='\n' || inChar=='\r';
}
static bool isNoteCommand(char inChar) {
	return  ('A'<=inChar && inChar<='G') || ('a'<=inChar && inChar<='g');
}
static bool isMmlCommand(char inChar) {
	if( isNoteCommand(inChar) ) { return true; }
	return inChar=='%'  // comment
		|| inChar=='R' || inChar=='r'	// rest
		|| inChar=='O' || inChar=='o'	// octave
		|| inChar=='<' || inChar=='>'	// octave shift
		|| inChar=='L' || inChar=='l'	// default tone length
		|| inChar=='V' || inChar=='v'	// volume (strength)
		|| inChar=='N' || inChar=='n'	// beep
		|| inChar=='T' || inChar=='t'	// tempo
		|| inChar=='@'  // reassignment
		|| inChar=='|'	// separate
		|| inChar=='!'	// callback
		|| inChar=='&' 	// ignore
		|| inChar=='$';	// repeat mark
}
static int skipWhiteSpace(const char *inMmlString,int inStartPos,int inMmlLength) {
	int i;
	for(i=inStartPos; i<inMmlLength; i++) {
		if(isWhiteSpace(inMmlString[i])==false) { break; }
	}
	return i;
}

static void printMmlErrorInfo(const char *inMmlStr,int inIndex,int inMmlLen) {
	for(int t=inIndex-5; t<inIndex+5; t++) {
		if(t<0 || t>=inMmlLen) { continue; }
		if(t==inIndex) {
			Serial.printf("[%c]",inMmlStr[t]);
		} else {
			Serial.printf("%c",inMmlStr[t]);
		}
	}
	Serial.printf("\n");
}

