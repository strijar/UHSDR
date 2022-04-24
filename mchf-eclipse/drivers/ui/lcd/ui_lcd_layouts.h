/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
 **                                                                                **
 **                                        UHSDR                                   **
 **               a powerful firmware for STM32 based SDR transceivers             **
 **                                                                                **
 **--------------------------------------------------------------------------------**
 **                                                                                **
 **  File name:		ui_lcd_layouts.h                                               **
 **  Description:   Layout definitions header file                                 **
 **  Licence:		GNU GPLv3                                                      **
 **  Author: 		Slawomir Balon/SP9BSL                                          **
 ************************************************************************************/

#ifndef UI_LCD_LAYOUTS_H_
#define UI_LCD_LAYOUTS_H_


#define COL_SPECTRUM_GRAD					0x40
#define Grid                RGB(COL_SPECTRUM_GRAD,COL_SPECTRUM_GRAD,COL_SPECTRUM_GRAD)      // COL_SPECTRUM_GRAD = 0x40
#define WATERFALL_HEIGHT 70
#define SPECTRUM_MAX_MARKER 3
#define SPECTRUM_SCOPE_GRID_VERT_COUNT  8
#define SPECTRUM_SCOPE_GRID_HORIZ 16

#define ENC_COL_W (42)
#define ENC_ROW_H (28)

#define ENC_ROW_2ND_OFF (14)
#define Xspacing 5
#define MSG_BUFFER_SIZE 60		    // this defines the buffer size for text line in maximum possible configuration.

typedef enum
{
    RESOLUTION_320_240,
    RESOLUTION_480_320,
    RESOLUTION_800_480
} disp_resolution_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
} UiArea_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;

    uint8_t padding;
    uint8_t size;
    uint8_t space;
} UiMeter_t;

typedef struct
{
	UiArea_t region;
	void (*function_short_press)();
	void (*function_long_press)();
} touchaction_descr_t;

typedef struct
{
	const touchaction_descr_t* actions;
	int32_t size;
} touchaction_list_descr_t;

typedef struct
{
    uint16_t x;
    uint16_t y;
} UiCoord_t ;

typedef struct {
	UiCoord_t Size;					// Lcd dimension

	UiCoord_t StartUpScreen_START;

	UiArea_t SpectrumWindow;		// Definition of spectrum window parameters

	UiCoord_t TUNE_FREQ;			// Frequency display control

	uint16_t TUNE_SPLIT_FREQ_X; 	//Shift with a small split to the right to close the frequency digits
	uint16_t TUNE_SPLIT_MARKER_X;
	uint16_t TUNE_SPLIT_FREQ_Y_TX;

	UiCoord_t TUNE_SFREQ;			// Second frequency display control

	UiArea_t BAND_MODE;			    // Band selection control

	UiArea_t DEMOD_MODE_MASK;		// Demodulator mode control

	UiArea_t AGC_MASK;				//AGC display mask

	UiArea_t TUNE_STEP; 			// Tunning step control

    UiArea_t INFO_BAR;              // Information bar
    UiArea_t MID_BAR;               // Middle bar
	UiArea_t BOTTOM_BAR;			// Bottom bar
	UiArea_t FILTER_IND;
	UiArea_t DSP_IND;
    UiArea_t TUNE_BOX;

	UiCoord_t ENCODER_IND;			// Encoder controls indicator
	uint8_t ENCODER_MODE;			//horizontal/vertical draw order

	UiArea_t PW_IND;				// Power level

	UiMeter_t SM_IND;				// Meter position
	UiCoord_t PWR_IND;				// Supply Voltage indicator
	UiCoord_t TEMP_IND;				// Temperature Indicator
	UiCoord_t RTC_IND;				// RTC

	UiCoord_t CW_DECODER_WPM;

	UiCoord_t TextMsgLine;			// coordinates for text line (CW decoder or freedv output)
	uint16_t TextMsg_buffer_max;	// Text message buffer size
	uint8_t  TextMsg_font;

	UiCoord_t SNAP_CARRIER;		// central position of variable freq marker

	UiCoord_t PWR_NUM_IND;	// Location of numerical FWD/REV power indicator

#ifdef USE_FREEDV
	UiCoord_t FREEDV_SNR;	//freeDV coordinates for status display
	UiCoord_t FREEDV_BER;
	uint16_t FREEDV_FONT;
#endif

	UiCoord_t DisplayDbm;

	UiCoord_t MEMORYLABEL;

	uint16_t LOADANDDEBUG_Y;
	uint16_t DEBUG_X;
	uint16_t LOAD_X;
	uint16_t SpectrumWindowPadding;

	uint16_t MENUSIZE;				// number of menu items per page/screen

	UiCoord_t MENU_IND;       	// X position of description of menu item being changed
	uint16_t MENU_CURSOR_X;     // Position of cursor used to indicate selected item
	uint8_t MENU_TEXT_SIZE_MAX;	// One line maximum length

	const touchaction_list_descr_t* touchaction_list;
} LcdLayout;

enum MODE_{
	MODE_VERTICAL=0,
	MODE_HORIZONTAL
};

enum LcdLayout_{
	LcdLayout_480x320 = 0,
	LcdLayout_320x240,
    LcdLayout_800x480,
	LcdLayoutsCount
};

extern const LcdLayout LcdLayouts[LcdLayoutsCount];
extern disp_resolution_t disp_resolution;

#endif /* UI_LCD_LAYOUTS_H_ */
