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

// see https://www.a-quest.com/blog_data/esp32/TestClickNoise.ino
//     http://blog-yama.a-quest.com/?eid=970190

#include <t2kCommon.h>

#include <driver/i2s.h>
#include <esp_task_wdt.h>

// #define DEBUG

#include <t2kSCore.h>

enum SoundCommand {
	SC_INVALID,
	SC_Tone,
	SC_Seq,
	// SC_Reg,

	SC_DUMP_CHANNEL_INFO,
};

struct CommandPacket {
	uint8_t command;
	uint8_t channel;
	float   freqHz;
	int16_t durationMSec;
	uint8_t volume;
};

struct ToneInfo {
	bool isAlive;
	float deltaTheta;
	float durationMSec;
	float scale;			// minus means no data.
};

const float kPI=3.14159265359f;
const float k2PI=2*kPI;

const i2s_port_t kI2SPort=I2S_NUM_0;

const int kLogicalSamplingHz=8000;	// 44100;
const int kOverSamplingRatio=1;
const uint32_t kI2S_SamplingHz=kLogicalSamplingHz*kOverSamplingRatio;	

const int kNumOfDmaBuffers=8;
const int kLengthOfDmaBuffer=32;	// num of samples at single channel.

const int kToneSeqQueueLength=32;

static volatile bool gQuiet=true;

static AXP192 gAxp;

static volatile ToneInfo gToneInfo[kNumOfChannels];
static QueueHandle_t gToneSeqQueue[kNumOfChannels];
static volatile float gMasterVolume[kNumOfChannels];	// 0 to 1

static void tonePump(void * /* inARGS */);
static bool getNextTone(int inChannel);
static bool soundCommandDispatcher(CommandPacket *inPacket,int inWait);
static bool setToneInfo(CommandPacket *inPacket);
static bool appendSeq(CommandPacket *inCommandPacket,/* bool inIsAlive, */ int inWait);
static void dumpChannelInfo();
static esp_err_t soundWrite(int16_t inVal);

const i2s_config_t kI2SConfig = {
	// .mode=(i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
	.mode=(i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
	.sample_rate=kI2S_SamplingHz,
	.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT,
	//.channel_format=I2S_CHANNEL_FMT_RIGHT_LEFT,
	.channel_format=I2S_CHANNEL_FMT_ALL_RIGHT,
	.communication_format=(i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
	.intr_alloc_flags=0,	// default interrupt priority
	.dma_buf_count=kNumOfDmaBuffers,
	.dma_buf_len=kLengthOfDmaBuffer,
	.use_apll=false,		// disable APLL
	
	.tx_desc_auto_clear=true,
	.fixed_mclk=0,
};

// for Core2
#define CONFIG_I2S_BCK_PIN      12
#define CONFIG_I2S_LRCK_PIN     0
#define CONFIG_I2S_DATA_PIN     2
#define CONFIG_I2S_DATA_IN_PIN  34
bool t2kSCoreInit() {
	gAxp.SetSpkEnable(true);
	i2s_driver_install(kI2SPort,&kI2SConfig,0,NULL);
	i2s_pin_config_t tx_pin_config = {
	    .bck_io_num           = CONFIG_I2S_BCK_PIN,
	    .ws_io_num            = CONFIG_I2S_LRCK_PIN,
	    .data_out_num         = CONFIG_I2S_DATA_PIN,
	    .data_in_num          = CONFIG_I2S_DATA_IN_PIN,
	  };
	i2s_set_pin(kI2SPort,&tx_pin_config);

	i2s_stop(kI2SPort);
	for(int i=0; i<kNumOfChannels; i++) {
		gToneInfo[i].isAlive=false;
		// gToneInfo[i].theta=0;
		gToneSeqQueue[i]=xQueueCreate(kToneSeqQueueLength,sizeof(ToneInfo));
		if(gToneSeqQueue[i]==0) {
			Serial.printf("t2kSCoreInit: can not create tone seq queue %d\n",i);
			return false;
		}
		gMasterVolume[i]=0.5f;
	}

	return true;
}

void t2kSCoreStart() {
	const int kSCoreCpuID=0;
	xTaskCreatePinnedToCore(tonePump,"tonePump",4096,NULL,1,NULL,kSCoreCpuID);
}

bool t2kSetMasterVolume(int8_t inChannel,uint8_t inVolume) {
	if(inChannel<0) {
		for(int i=0; i<kNumOfChannels; i++) {
			gMasterVolume[i]=inVolume/255.0f;
		}
	} else {
		if(inChannel>=kNumOfChannels) { return false; }
		gMasterVolume[inChannel]=inVolume/255.0f;
	}
	return true;
}

bool t2kTone(uint8_t inChannel,float inFreqHz,int16_t inDurationMSec,uint8_t inVolume) {
	if(inChannel>=kNumOfChannels) { return false; }
	CommandPacket packet;
	packet.command=SC_Tone;
	packet.channel=inChannel;
	packet.freqHz =inFreqHz;
	packet.durationMSec=inDurationMSec;
	packet.volume=inVolume;
	gQuiet=false;
	return soundCommandDispatcher(&packet,portMAX_DELAY);
}

bool t2kAddTone(uint8_t inChannel,float inFreqHz,int16_t inDurationMSec,uint8_t inVolume) {
	if(inChannel>=kNumOfChannels) { return false; }
	CommandPacket packet;
	packet.command=SC_Seq;
	packet.channel=inChannel;
	packet.freqHz =inFreqHz;
	packet.durationMSec=inDurationMSec;
	packet.volume=inVolume;
	gQuiet=false;
	return soundCommandDispatcher(&packet,0);
}

bool t2kStartToneSeq(uint8_t inChannel) {
	if(inChannel>=kNumOfChannels) { return false; }
	gToneInfo[inChannel].isAlive=true;
	gQuiet=false;
	return true;
}

bool t2kClearToneSeq(uint8_t inChannel) {
	if(inChannel>=kNumOfChannels) { return false; }
	if(inChannel==kAllChannels) {
		for(int i=0; i<kNumOfChannels; i++) { gToneInfo[i].isAlive=false; }
		gQuiet=true;
		bool result=true;
		for(int i=0; i<kNumOfChannels; i++) {
			if(xQueueReset(gToneSeqQueue[inChannel])!=pdTRUE) { result=false; }
		}
		return result;
	} else {
		gToneInfo[inChannel].isAlive=false;
		bool quiet=true;
		for(int i=0; i<kNumOfChannels; i++) {
			if (gToneInfo[i].isAlive ) {
				quiet=false; 
				break;
			}
		}
		gQuiet=quiet;
		return xQueueReset(gToneSeqQueue[inChannel])==pdTRUE;
	}
}

void t2kQuiet() {
	gQuiet=true;
}

static float gDeltaSigma=0;
static void tonePump(void * /* inARGS */) {
	i2s_start(kI2SPort);
	i2s_zero_dma_buffer(kI2SPort);

	disableCore0WDT();

	float theta[kNumOfChannels];
	for(int i=0; i<kNumOfChannels; i++) { theta[i]=0; }

	float t;
	float tmp;
	const float dtMSec=1000.0f/(kI2S_SamplingHz*2);
	volatile ToneInfo *toneInfo;
	for(;;) {
		if( gQuiet ) { i2s_zero_dma_buffer(kI2SPort); }
		t=0;
		toneInfo=gToneInfo;
		for(int i=0; i<kNumOfChannels; i++,toneInfo++) {
			if( toneInfo->isAlive ) { 
				if(toneInfo->scale<0) {
					if(getNextTone(i)==false) { continue; }
				}
				if(toneInfo->deltaTheta<0) {
					// noise
					DEBUG_LN("NOISE");
					t+=(((float)rand()/RAND_MAX)*2-1)*toneInfo->scale;
				} else {
					tmp=theta[i]+toneInfo->deltaTheta;
					tmp-=((int)(tmp/k2PI))*k2PI;
					theta[i]=tmp;
					t+=sin(theta[i])*toneInfo->scale*gMasterVolume[i];
				}
			}
			if(toneInfo->durationMSec>0) { toneInfo->durationMSec-=dtMSec; }
			if(toneInfo->durationMSec<=0) {
				if(getNextTone(i)==false) {
					toneInfo->isAlive=false;
					toneInfo->scale=-1;	// mark no data
				}
			}
		}
		int16_t dataToSend=(int16_t)(t+gDeltaSigma);
		gDeltaSigma=(t+gDeltaSigma)-dataToSend;
		soundWrite(dataToSend);
	}
}
static bool getNextTone(int inChannel) {
	if(inChannel<0 || kNumOfChannels<=inChannel) { return false; }

	ToneInfo nextTone;
	if(xQueueReceive(gToneSeqQueue[inChannel],&nextTone,0)==pdTRUE) {
		volatile ToneInfo *toneInfo=gToneInfo+inChannel;
		toneInfo->isAlive=true;
		// toneInfo->theta <- do not change
		toneInfo->deltaTheta=nextTone.deltaTheta;
		toneInfo->durationMSec=(float)nextTone.durationMSec;
		toneInfo->scale=nextTone.scale;
//Serial.printf("getNexTone: ch=%d deltaTheta=%f\n",inChannel,toneInfo->deltaTheta);
		return true;
	}
	return false;
}
static bool soundCommandDispatcher(CommandPacket *inCommandPacket,int inWait) {
	switch(inCommandPacket->command) {
		case SC_Tone:	return setToneInfo(inCommandPacket);
		case SC_Seq:	return appendSeq(inCommandPacket,/* true, */ inWait);
		case SC_DUMP_CHANNEL_INFO:
			dumpChannelInfo();
			return true;
		default:
			Serial.printf("soundCommandDispatcher: unknown command=%d\n",
						  inCommandPacket->command);
			return false;
	}
}
static void dumpChannelInfo() {
	i2s_start(kI2SPort);
	soundWrite(0);
	for(int i=0; i<kNumOfChannels; i++) {
		Serial.printf("ch=%d alive=%d delta=%f duration=%f scale=%f\n",
					  i,gToneInfo[i].isAlive,
					  gToneInfo[i].deltaTheta,
					  gToneInfo[i].durationMSec,
					  gToneInfo[i].scale);
	}
}
static bool setToneInfo(CommandPacket *inPacket) {
	uint8_t ch=inPacket->channel;
	if(ch>=kNumOfChannels) {
		DEBUG("t2kSCore (tone): No shuch channels (ch=%d)\n",ch);
		return false;
	}
	if(xQueueReset(gToneSeqQueue[ch])!=pdTRUE) {
		Serial.printf("setToneInfo: can not reset tone seq queue %d\n",ch);
	}
	const float f=inPacket->freqHz;
	if(f==0) {
		gToneInfo[ch].isAlive=false;
	} else if(f<0) {
		gToneInfo[ch].isAlive=true;
		// gToneInfo[ch].theta=0; 	theta, don't care
		gToneInfo[ch].deltaTheta=-1;	// mark noise
	} else {
		DEBUG("SetTone freq=%d\n",(int)f);
		gToneInfo[ch].isAlive=true;
		gToneInfo[ch].deltaTheta=k2PI*f/(kI2S_SamplingHz*2);
		gToneInfo[ch].durationMSec=(float)inPacket->durationMSec;
		// use same value to theta for continuity.
		DEBUG("ch=%d duration=%f\n",ch,gToneInfo[ch].durationMSec);
	}
	gToneInfo[ch].scale=inPacket->volume/255.0f*0x8000/3.0f;
	return true;
}
static bool appendSeq(CommandPacket *inCommandPacket,/* bool inIsAlive, */ int inWait) {
	uint8_t ch=inCommandPacket->channel;
	if(ch>=kNumOfChannels) {
		DEBUG("t2kSCore (tone): No shuch channels (ch=%d)\n",ch);
		return false;
	}
	float f=inCommandPacket->freqHz;
	ToneInfo toneInfoPacket;
	toneInfoPacket.isAlive=true; // inIsAlive;
	// toneInfo.theta <- don't care
	if(f>=0) {
		toneInfoPacket.deltaTheta=k2PI*f/(kI2S_SamplingHz*2);	// [rad/msec]
	} else {
		toneInfoPacket.deltaTheta=-1;	// mark noise.
	}
	toneInfoPacket.durationMSec=(float)inCommandPacket->durationMSec;
	toneInfoPacket.scale=inCommandPacket->volume/255.0f*0x8000/3.0f;
	return xQueueSend(gToneSeqQueue[ch],&toneInfoPacket,inWait)==pdTRUE;
}

// write to I2S DMA buffer
// push a sample.
// return:
// 	 1 if success
// 	 0 if timeouted
// 	-1 if falied (ESP_FAIL)
const int SOUND_BUF_SIZE=32;
static int16_t gSoundBuf[SOUND_BUF_SIZE];
static int gSoundBufIndex=0;
static esp_err_t soundWrite(int16_t inVal) {
	gSoundBuf[gSoundBufIndex++]=inVal;
	if(gSoundBufIndex==SOUND_BUF_SIZE) {
		gSoundBufIndex=0;
		size_t bytesWritten;
		return i2s_write(kI2SPort,gSoundBuf,sizeof(gSoundBuf),&bytesWritten,portMAX_DELAY);
	} else {
		return 1;
	}
}

