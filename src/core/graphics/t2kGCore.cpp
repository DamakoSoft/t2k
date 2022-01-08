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

// This program is based on M5Stack_LCD_DMA by MHageGH and modified for game
// development. You can get the original MHageGH's M5Stack_LCD_DMA here.
// https://github.com/MhageGH/M5Stack_LCD_DMA
// We would like to thank Mr. MHageGH for making such a wonderful program
// available to the public. Thank you very much!

#include <M5Core2.h>

// #define ENABLE_DEBUG_MESSAGE
#include <t2kCommon.h>

#include <string.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <driver/spi_master.h>
#include <soc/gpio_struct.h>
#include <driver/gpio.h>

#include <utility/Config.h>
#include <utility/In_eSPI.h>

#include <t2kGCore.h>

const int kLcdWidth =320;
const int kLcdHeight=240;
static uint8_t *gFrameBuffer[2]={NULL,NULL};
int gCurrentBufferToDraw=0;

void t2kLcdReset(bool inState);

const int kNumOfDmaTransfer=8;	// 8 is max.
const int kDmaWidth =320;
const int kDmaBufferHeight=240/kNumOfDmaTransfer;

const int kGRamGapHeight=120/kNumOfDmaTransfer;

static spi_device_handle_t gSpi;

static bool lcdInit(spi_device_handle_t inSpi);
static bool lcdCmd(spi_device_handle_t inSpi,const uint8_t inCmd);
static void lcdData(spi_device_handle_t inSpi,const uint8_t *inData,int inLen);

static void sendFramebuffer(spi_device_handle_t inSpi,
 					  		int inX,int inY,int inWidth,int inHeight,
							uint16_t *inFrameBuffer);
static void sendFrameBufferFinish(spi_device_handle_t inSpi);

#define CurrentBuffer (gFrameBuffer[gCurrentBufferToDraw])

static uint16_t *gDmaBuffer=NULL;

static volatile uint64_t gToFlipped=0;
static volatile uint64_t gFlipped=0;

static spi_device_handle_t spiStart();
static void lcdSpiPreTransferCallback(spi_transaction_t *inSpiTransaction);

static void flipPump(void *inArgs);
static bool createFramebuffer();
static void flip();

bool t2kGCoreInit() {
    gSpi=spiStart();
    if( lcdInit(gSpi) ) {
		Serial.printf("SUCCESS lcdInit\n");
	} else {
		ERROR("ERROR t2kGCoreInit: lcdInit\n");
	}

    // for M5Stack backlight PWM control.
	const int BLK_PWM_CHANNEL=7;
    ledcSetup(7,44100,8);
	ledcAttachPin(TFT_BL, BLK_PWM_CHANNEL);
    ledcWrite(BLK_PWM_CHANNEL,255);	// max brightness

    bool ret=createFramebuffer();
	if(ret==false) { ERROR("ERROR: t2kGCoreInit() FAILED.\n"); }
	return ret;
}

void t2kGCoreStart() {
	const int kGCoreCpuID=0;
	xTaskCreatePinnedToCore(flipPump,"t2kGCore",4096,NULL,1,NULL,kGCoreCpuID);
}
static void flipPump(void *inArgs) {
	Serial.printf("=== t2kGCore:flipPump Started ===\n");
	while(true) {
		// wait request
		while(gToFlipped<=gFlipped) {
			delay(10);
			//taskYIELD();
//Serial.printf("flipPump WAIT gToFlipped=%d gFlipped=%d\n",gToFlipped,gFlipped);
		}
		flip();
		gFlipped++;
	}
}

void t2kFlip() {
	gToFlipped++;
	DEBUG("Request Flip=%d    \n",gToFlipped);
	while(gToFlipped!=gFlipped) {
		delay(10);
		// taskYIELD();
	}
	DEBUG("Flip Done=%d    \n",gToFlipped);
}

static bool createFramebuffer() {
    for(int i=0; i<2; i++) {
		gFrameBuffer[i]=(uint8_t *)malloc(kGRamWidth*kGRamHeight*sizeof(uint8_t));
    	if(gFrameBuffer[i]==NULL) {
			Serial.printf("================NO FrameBuffer MEMORY %d\n",i);
			return false;
		}
   	}
  	gDmaBuffer=(uint16_t *)heap_caps_malloc(kDmaWidth*kDmaBufferHeight*sizeof(uint16_t),MALLOC_CAP_DMA);
   	if(gDmaBuffer==NULL) {
		Serial.println("================NO DMA MEMORY");
		return false;
	}
	return true;
}

// @param brightness 0~255
void t2kSetBrightness(uint8_t inBrightness) {
	t2kLcdBrightness(inBrightness);
}

uint8_t *t2kGetFramebuffer() {
	return gFrameBuffer[gCurrentBufferToDraw];
}

void t2kFill(uint8_t inColorRGB322) {
	memset(gFrameBuffer[gCurrentBufferToDraw],inColorRGB322,kGRamWidth*kGRamHeight);
}

void t2kCopyFromFrontBuffer() {
	uint8_t *front=gFrameBuffer[(gCurrentBufferToDraw+1)%2];
	memcpy(gFrameBuffer[gCurrentBufferToDraw],front,kGRamWidth*kGRamHeight);
}

void t2kPSet(int inX,int inY,uint8_t inColorRGB332) {
	if(inX<0 || kGRamWidth<=inX || inY<0 || kGRamHeight<=inY) { return; }
	CurrentBuffer[FBA(inX,inY)]=inColorRGB332;
}


// ref  https://cdn-shop.adafruit.com/datasheets/ILI9341.pdf
// 	and http://blog.livedoor.jp/prittyparakeet/archives/2016-11-04.html
//
// kLCD_CMD_... LCD Command
// kLCD_PRN_... LCD Parameter
const uint8_t kLCD_CMD_PowerControl1=0xC0;
const uint8_t kLCD_PRM_PC1_3_80_V=0x23;		// 3.80 [volt]

const uint8_t kLCD_CMD_PowerControl2=0xC1;
const uint8_t kLCD_PRM_PC2_VCIx2=10;

const uint8_t kLCD_CMD_VcomControl1 =0xC5;
const uint8_t kLCD_PRM_VcomH_4_250V =0x3E;	//  4.250 [volt]
const uint8_t kLCD_PRM_VcomL_M1_500V=0x28;	// -1.500 [volt]

const uint8_t kLCD_CMD_VcomControl2=0xC7;
const uint8_t kLCD_PRM_VMH_VML_M58 =0x86;

const uint8_t kLCD_CMD_MemoryAccessControl=0x36;
const uint8_t kLCD_PRM_MAC_RowAddressOrder	   	 =0x80;
const uint8_t kLCD_PRM_MAC_ColumnAddressOrder  	 =0x40;
const uint8_t kLCD_PRM_MAC_RowColumnExchange   	 =0x20;
const uint8_t kLCD_PRM_MAC_VerticalRefreshOrder	 =0x10;
const uint8_t kLCD_PRM_MAC_RgbBgrOrder		   	 =0x08;
const uint8_t kLCD_PRM_MAC_HorizontalRefreshOrder=0x40;

const uint8_t kLCD_CMD_PixelFormatSet=0x3A;
const uint8_t kLCD_PRM_PFS_RgbInterfaceFormat_16BitsPerPixel=0x50;
const uint8_t kLCD_PRM_PFS_McuInterfaceFormat_16BitsPerPixel=0x05;

const uint8_t kLCD_CMD_FrameRateControl=0xB1;
const uint8_t kLCD_PRM_DivisionRatioStraight=0x00;
const uint8_t kLCD_PRM_FrameRate100Hz		=0x13;

const uint8_t kLCD_CMD_DisplayFunctionControl=0xB6;
const uint8_t kLCD_PRM_DFC_IntervalScan	  =0x08;
const uint8_t kLCD_PRM_DFC_V63V0VcomlVcomh=0x00;
const uint8_t kLCD_PRM_DFC_LcdNormalyWhite=0x80;
const uint8_t kLCD_PRM_DFC_ScanCycle85ms  =0x02;
const uint8_t kLCD_PRM_DFC_320Lines		  =0x27;

const uint8_t kLCD_CMD_GammaSet=0x26;
const uint8_t kLCD_PRM_GS_Gamma2_2=0x01;

const uint8_t kLCD_CMD_PositiveGammaCorrection=0xE0;
const uint8_t kLCD_CMD_NegativeGammaCorrection=0xE1;

const uint8_t kLCD_CMD_SleepOut=0x11;
const uint8_t kLCD_PRM_SO_NO_PARAMETER=0x00;

const uint8_t kLCD_CMD_DisplayON=0x29;
const uint8_t kLCD_PRM_DO_NO_PARAMETER=0x00;

const uint8_t kLCD_CMD_DisplayInversionON=0x21;
const uint8_t kLCD_PRM_DIO_NO_PARAMETER=0x00;

typedef struct {
	uint8_t cmd;
	uint8_t data[16];

	// No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
	uint8_t databytes;
} LcdInitCmd;
const uint8_t kSendDummyData=0x80;
const uint8_t kEND=0xFF;

DRAM_ATTR const LcdInitCmd kIliInitCmds[] = {
    // ref to TFT_eSPI::init() In_eSPI.cpp in M5Stack Libarary
    {0xEF, {0x03, 0x80, 0x02}, 3},				// undocumented magic sequence?
    {0xCF, {0x00, 0xC1, 0x30}, 3},				// 		same as above
    {0xED, {0x64, 0x03, 0x12, 0x81}, 4},		//		same as above
    {0xE8, {0x85, 0x00, 0x78}, 3},				//		same as above
    {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},	// 		same as above
    {0xF7, {0x20}, 1},							//		same as above
    {0xEA, {0x00, 0x00}, 2},					//		same as above
    { kLCD_CMD_PowerControl1, { kLCD_PRM_PC1_3_80_V }, 1},
    { kLCD_CMD_PowerControl2, { kLCD_PRM_PC2_VCIx2  }, 1},

    { kLCD_CMD_VcomControl1, { kLCD_PRM_VcomH_4_250V,
							   kLCD_PRM_VcomL_M1_500V }, 2},
    { kLCD_CMD_VcomControl2, { kLCD_PRM_VMH_VML_M58 }, 1},

    { kLCD_CMD_MemoryAccessControl, { kLCD_PRM_MAC_RowAddressOrder
									  | kLCD_PRM_MAC_RowColumnExchange
									  | kLCD_PRM_MAC_RgbBgrOrder }, 1},

    { kLCD_CMD_PixelFormatSet, { kLCD_PRM_PFS_RgbInterfaceFormat_16BitsPerPixel
								 | kLCD_PRM_PFS_McuInterfaceFormat_16BitsPerPixel }, 1},

    { kLCD_CMD_FrameRateControl, { kLCD_PRM_DivisionRatioStraight,
								   kLCD_PRM_FrameRate100Hz }, 2},

    { kLCD_CMD_DisplayFunctionControl, { kLCD_PRM_DFC_IntervalScan
									   	 | kLCD_PRM_DFC_V63V0VcomlVcomh,
										 kLCD_PRM_DFC_LcdNormalyWhite
										 | kLCD_PRM_DFC_ScanCycle85ms,
										 kLCD_PRM_DFC_320Lines}, 3},

    {0xF2, {0x00}, 1},	// undocumented magic sequence

    { kLCD_CMD_GammaSet, { kLCD_PRM_GS_Gamma2_2 }, 1},
    { kLCD_CMD_PositiveGammaCorrection, { 0x0F, 0x31, 0x2B,	// VP63, VP62, VP61
										  0x0C,				// VP59
										  0x0E,				// VP57
										  0x08,				// VP50
										  0x4E,				// VP43
										  0xF1,				// VP27 and VP36
										  0x37,				// VP20
										  0x07,				// VP13
										  0x10,				// VP6
										  0x03,				// VP4
										  0x0E, 0x09, 0x00	// VP2, VP1, VP0
										}, 15},
    { kLCD_CMD_NegativeGammaCorrection, { 0x00, 0x0E, 0x14,	// VN63, VN62, VN61
										  0x03,				// VN59
										  0x11,				// VN57
										  0x07,				// VN50
										  0x31,				// VN43
										  0xC1,				// VN36 and VN27
										  0x48,				// VN20
										  0x08,				// VN13
										  0x0F,				// VN6
										  0x0C,				// VN4
										  0x31, 0x36, 0x0F	// VN2, VN1, VN0
										}, 15},
    { kLCD_CMD_SleepOut,  { kLCD_PRM_SO_NO_PARAMETER }, kSendDummyData },
    { kLCD_CMD_DisplayON, { kLCD_PRM_DO_NO_PARAMETER }, kSendDummyData },

	// ref to setRotation(1) in ILI9341_Rotation.h in M5Stack Library
	// this command is importand (do not remove!)
    { kLCD_CMD_MemoryAccessControl, { kLCD_PRM_MAC_RgbBgrOrder }, 1},

	// for new M5Stack basic
	{ kLCD_CMD_DisplayInversionON, { kLCD_PRM_DIO_NO_PARAMETER }, kSendDummyData },

    { 0, { 0 }, kEND } 
};

// #define PIN_NUM_MISO GPIO_NUM_19
#ifdef M5StacK_BASIC
	#define PIN_NUM_MISO GPIO_NUM_19
	#define PIN_NUM_MOSI GPIO_NUM_23
	#define PIN_NUM_CLK  GPIO_NUM_18
	#define PIN_NUM_CS   GPIO_NUM_14
	#define PIN_NUM_DC   GPIO_NUM_27
	#define PIN_NUM_RST  GPIO_NUM_33
	#define PIN_NUM_BCKL GPIO_NUM_32
#else
	#define PIN_NUM_MISO GPIO_NUM_38
	#define PIN_NUM_MOSI GPIO_NUM_23
	#define PIN_NUM_CLK  GPIO_NUM_18
	#define PIN_NUM_CS   GPIO_NUM_5
	#define PIN_NUM_DC   GPIO_NUM_15
	// #define PIN_NUM_RST  GPIO_NUM_33
	// #define PIN_NUM_BCKL GPIO_NUM_32
#endif

#include <driver/spi_common.h>

static spi_device_handle_t spiStart() {
	Serial.printf("START spiStart()...\n");
    spi_bus_config_t buscfg={
        .mosi_io_num=PIN_NUM_MOSI,
        .miso_io_num=PIN_NUM_MISO,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=240*320*2+8,
        .flags=SPICOMMON_BUSFLAG_MASTER, 
        .intr_flags=0
	};
    esp_err_t result=spi_bus_initialize(VSPI_HOST,&buscfg,1);	// DMA 1 ch.
    // ESP_ERROR_CHECK(result);
	if(result!=ESP_OK) {
		ERROR("ERROR spiStart: spi_bus_initialize\n");
	} else {
		Serial.printf("spi_bus_initialize() OK.\n");
	}

    spi_device_interface_config_t devcfg={
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .mode=0, //SPI mode 0
        .duty_cycle_pos=0,
        .cs_ena_pretrans=0,
        .cs_ena_posttrans=0,
        // .clock_speed_hz=SPI_MASTER_FREQ_26M,	// 40 * 1000 * 1000,	
        .clock_speed_hz=SPI_MASTER_FREQ_40M,	
        .input_delay_ns=0,
        .spics_io_num=PIN_NUM_CS, // CS pin
        // .flags=0,
        .flags=SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX,
        .queue_size=7,	// We want to be able to queue 7 transactions at a time
        .pre_cb=lcdSpiPreTransferCallback, // Specify pre-transfer callback to handle D/C line
        .post_cb=0
	};
    spi_device_handle_t hSpi;
    result=spi_bus_add_device(VSPI_HOST,&devcfg,&hSpi);
    //ESP_ERROR_CHECK(result);
	if(result!=ESP_OK) {
		ERROR("ERROR spiStart: spi_bus_add_device\n");
	} else {
		Serial.printf("spi_bus_add_device() OK.\n");
		Serial.printf("SUCCESS spiStart hSpi=%p\n",hSpi);
	}

    return hSpi;
}

static bool lcdCmd(spi_device_handle_t inSpi,const uint8_t inCmd) {
    spi_transaction_t t;
    memset(&t,0,sizeof(t));	//Zero out the transaction
    t.length=8;				//Command is 8 bits
    t.tx_buffer=&inCmd;		//The data is the cmd itself
    t.user=(void *)0;		//D/C needs to be set to 0
    esp_err_t result=spi_device_polling_transmit(inSpi,&t); //Transmit!
    // assert(result==ESP_OK);	//Should have had no issues.
	if(result!=ESP_OK) { ERROR("ERROR lcdCmd: spi_device_polling_transmit\n"); }
	return result==ESP_OK;
}

static void lcdData(spi_device_handle_t inSpi, const uint8_t *inData, int inLen) {
    if(inLen==0) { return; }	// no need to send anything
    spi_transaction_t t;
    memset(&t,0,sizeof(t));		// Zero out the transaction
    t.length=inLen*8;			// Len is in bytes, transaction length is in bits.
    t.tx_buffer=inData;			// Data
    t.user=(void *)1;			// D/C needs to be set to 1
    esp_err_t ret=spi_device_polling_transmit(inSpi, &t);	// Transmit!
    assert(ret==ESP_OK);		// Should have had no issues.
}

static void lcdSpiPreTransferCallback(spi_transaction_t *inSpiTransaction) {
    int dc=(int)inSpiTransaction->user;
    gpio_set_level(PIN_NUM_DC,dc);
}

static bool lcdInit(spi_device_handle_t inSpi) {
    gpio_set_direction(PIN_NUM_DC,GPIO_MODE_OUTPUT);
    // gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);	// for M5 Basic
    // gpio_set_direction(PIN_NUM_BCKL,GPIO_MODE_OUTPUT);	// for M5 Basic
	
    // gpio_set_level(PIN_NUM_RST, 0);
	t2kLcdReset(false);
    vTaskDelay(100/portTICK_RATE_MS);

    //gpio_set_level(PIN_NUM_RST, 1);
	t2kLcdReset(true);
    vTaskDelay(100/portTICK_RATE_MS);

    const LcdInitCmd *lcdInitCmds=kIliInitCmds;
	bool result=true;
    int cmd=0;
    while(lcdInitCmds[cmd].databytes!=0xff) {
        if(lcdCmd(inSpi,lcdInitCmds[cmd].cmd)==false) {
			result=false;
			Serial.printf("CMD=%d\n",cmd);
			break;
		}
        lcdData(inSpi,lcdInitCmds[cmd].data, lcdInitCmds[cmd].databytes&0x1F);
        if(lcdInitCmds[cmd].databytes&0x80) { vTaskDelay(100/portTICK_RATE_MS); }
        cmd++;
    }
    // gpio_set_level(PIN_NUM_BCKL, 1); ///Enable backlight
	t2kLcdBrightness(255);

	return result;
}

static void sendFramebuffer(spi_device_handle_t inSpi,
					  		int inX,int inY,int inWidth,int inHeight,
							uint16_t *inFrameBuffer) {
    static spi_transaction_t trans[6];
    for(int i=0; i<6; i++) {
        memset(&trans[i],0,sizeof(spi_transaction_t));
        if((i&0x1)==0) {
            trans[i].length=8;
            trans[i].user=(void *)0;
        } else {
            trans[i].length=8*4;
            trans[i].user=(void *)1;
        }
        trans[i].flags=SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0]=0x2A;				// column address set
    trans[1].tx_data[0]=inX>>8;				// start col high
    trans[1].tx_data[1]=inX&0xFF;			// start col low
	const int right=inX+inWidth-1;
    trans[1].tx_data[2]=right>>8;   		// end col high
    trans[1].tx_data[3]=right&0xFF; 		// end col low
    trans[2].tx_data[0]=0x2B;				// page address set
    trans[3].tx_data[0]=inY>>8;				// start page high
    trans[3].tx_data[1]=inY&0xFF;			// start page low
	const int bottom=inY+inHeight-1;
    trans[3].tx_data[2]=bottom>>8;			// end page high
    trans[3].tx_data[3]=bottom&0xFF;		// end page low
    trans[4].tx_data[0]=0x2C;				// memory write
    trans[5].tx_buffer=inFrameBuffer;		// finally send the line data
    trans[5].length=inWidth*2*8*inHeight;	// data length, in bits
    trans[5].flags=0;						// undo SPI_TRANS_USE_TXDATA flag

    esp_err_t result;
    for(int i=0; i<6; i++) {
        result=spi_device_queue_trans(inSpi,&trans[i],portMAX_DELAY);
        //assert(result==ESP_OK);
		if(result!=ESP_OK) { ERROR("ERROR sendFramebuffer: spi_device_queue_trans\n"); }
    }
}

static void sendFrameBufferFinish(spi_device_handle_t inSpi) {
    spi_transaction_t *rtrans;
    esp_err_t result;
    for(int i=0; i<6; i++) {
        result=spi_device_get_trans_result(inSpi,&rtrans,portMAX_DELAY);
        // assert(ret == ESP_OK);
		if(result!=ESP_OK) {
			ERROR("ERROR sendFrameBufferFinish: spi_device_get_trans_result\n");
		}
    }
}

static void flip() {
	DEBUG_LN("**** FLIP IN");
	uint8_t *srcP=gFrameBuffer[gCurrentBufferToDraw];
	for(int i=0; i<kNumOfDmaTransfer; i++) {
		const int startY=i*kGRamGapHeight;
		const int endY=startY+kGRamGapHeight;
		uint16_t *destP=gDmaBuffer;
		for(int srcY=startY; srcY<endY; srcY++,destP+=kDmaWidth*2,srcP+=kGRamWidth) {
			for(int srcX=0,destX=0; srcX<kGRamWidth; srcX++,destX+=2) {
				uint16_t srcColor=(uint16_t)srcP[srcX];
				uint16_t destColor;
				if(srcColor==0xFF) {
					destColor=0xFFFF;
				} else {
					destColor= ((srcColor & 0xE0)<<8)
						  		| ((srcColor & 0x1C)<<6) 
								| ((srcColor & 0x03)<<3);
					// swap hi-low due to little engian
					destColor = ((destColor & 0xFF)<<8) | (destColor>>8);
				}
				destP[destX]=destP[destX+1]
				=destP[kDmaWidth+destX]=destP[kDmaWidth+destX+1]=destColor;
			}
		}
		sendFramebuffer(gSpi,0,startY*2,kDmaWidth,kDmaBufferHeight,gDmaBuffer);
		sendFrameBufferFinish(gSpi);
	}
    gCurrentBufferToDraw ^= 0x01;	// 0 -> 1 -> 0 -> 1 ...
	DEBUG_LN("**** FLIP OUT");
}

