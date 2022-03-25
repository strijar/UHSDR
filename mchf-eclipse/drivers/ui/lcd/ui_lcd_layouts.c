/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
 **                                                                                **
 **                                        UHSDR                                   **
 **               a powerful firmware for STM32 based SDR transceivers             **
 **                                                                                **
 **--------------------------------------------------------------------------------**
 **                                                                                **
 **  File name:		ui_lcd_layouts.c                                               **
 **  Description:   Layout definitions file                                        **
 **  Licence:		GNU GPLv3                                                      **
 **  Author: 		Slawomir Balon/SP9BSL                                          **
 ************************************************************************************/

#ifndef BOOTLOADER_BUILD
#include "uhsdr_board.h"
#include "ui_lcd_layouts.h"
#include "ui_driver.h"

//------------------------------------------------------------------------------------------------------------------------------------------
//some local 480x320 calculations to make life easier in case of change something

#define R480320_TUNE_FREQ_X             		280
#define R480320_TUNE_FREQ_Y             		36
#define R480320_TUNE_SPLIT_FREQ_X           	(R480320_TUNE_FREQ_X+80)//Shift with a small split to the right to close the frequency digits.
#define R480320_TUNE_SPLIT_MARKER_X         	(R480320_TUNE_FREQ_X+40)
#define R480320_TUNE_SPLIT_FREQ_Y_TX        	(R480320_TUNE_FREQ_Y+12)
#define R480320_TUNE_SFREQ_X            		(R480320_TUNE_FREQ_X + 120)// Second frequency display control
#define R480320_TUNE_SFREQ_Y            		(R480320_TUNE_FREQ_Y - 20)

#define R480320_DisplayDbm_X					(R480320_TUNE_FREQ_X+45)
#define R480320_DisplayDbm_Y					(R480320_TUNE_FREQ_Y-36)

#define R480320_MEMORYLABEL_X 					(R480320_TUNE_FREQ_X+45+(SMALL_FONT_WIDTH * 11)+4)
#define R480320_MEMORYLABEL_Y 					(R480320_TUNE_FREQ_Y-36)

#define R480320_BAND_MODE_X             		(R480320_TUNE_FREQ_X + 160)
#define R480320_BAND_MODE_Y             		(R480320_TUNE_FREQ_Y + 7)
#define R480320_BAND_MODE_MASK_X            	(R480320_BAND_MODE_X - 1)
#define R480320_BAND_MODE_MASK_Y            	(R480320_BAND_MODE_Y - 1)
#define R480320_BAND_MODE_MASK_H            	13
#define R480320_BAND_MODE_MASK_W            	33

// Demodulator mode control
#define R480320_DEMOD_MODE_X            		(R480320_TUNE_FREQ_X -61)
#define R480320_DEMOD_MODE_Y            		(R480320_TUNE_FREQ_Y - 20)
#define R480320_DEMOD_MODE_MASK_X           	R480320_DEMOD_MODE_X
#define R480320_DEMOD_MODE_MASK_Y           	(R480320_DEMOD_MODE_Y - 1)
#define R480320_DEMOD_MODE_MASK_H           	13
#define R480320_DEMOD_MODE_MASK_W           	58	//same as R480320_DIGMODE_IND_W

// AGC display box
//#define R480320_AGC_MASK_X (R480320_DEMOD_MODE_MASK_X-41)
#define R480320_AGC_MASK_X (R480320_TUNE_FREQ_X)
#define R480320_AGC_MASK_Y R480320_DEMOD_MODE_MASK_Y
#define R480320_AGC_MASK_W 41
#define R480320_AGC_MASK_H R480320_DEMOD_MODE_MASK_H

// Tunning step control
#define R480320_TUNE_STEP_X             		(R480320_TUNE_FREQ_X + 45)
#define R480320_TUNE_STEP_Y             		(R480320_TUNE_FREQ_Y - 21)
#define R480320_TUNE_STEP_MASK_H            	15
#define R480320_TUNE_STEP_MASK_W            	(SMALL_FONT_WIDTH*7)

#define R480320_PW_IND_X                		(R480320_TUNE_FREQ_X)
#define R480320_PW_IND_Y                		(R480320_DEMOD_MODE_Y - 16)
#define R480320_PW_IND_W						41

#define R480320_SM_IND_X                		0
#define R480320_SM_IND_Y                		0
#define R480320_SM_IND_W 						(200)
#define R480320_SM_IND_H 						(72 - 14)

#define R480320_LEFTBOXES_IND_X              	0
#define R480320_LEFTBOXES_IND_Y              	64
#define R480320_LEFTBOX_WIDTH 					58 // used for the lower left side controls
#define R480320_LEFTBOX_ROW_H  					31
#define R480320_LEFTBOX_ROW_2ND_OFF  			15

#define R480320_ENCODER_IND_X                	128
#define R480320_ENCODER_IND_Y                	64

#define R480320_DIGMODE_IND_X              		R480320_DEMOD_MODE_MASK_X  // 218
#define R480320_DIGMODE_IND_Y              		0
#define R480320_DIGMODE_IND_W					R480320_DEMOD_MODE_MASK_W

//------------------------------------------------------------------------------------------------------------------------------------------
//some local 320x240 calculations to make life easier in case of change something
#define R320240_TUNE_FREQ_X             		116
#define R320240_TUNE_FREQ_Y             		100
#define R320240_TUNE_SPLIT_FREQ_X           	(R320240_TUNE_FREQ_X+80)//Shift with a small split to the right to close the frequency digits.
#define R320240_TUNE_SPLIT_MARKER_X         	(R320240_TUNE_FREQ_X+40)
#define R320240_TUNE_SPLIT_FREQ_Y_TX        	(R320240_TUNE_FREQ_Y+12)
#define R320240_TUNE_SFREQ_X            		(R320240_TUNE_FREQ_X + 120)// Second frequency display control
#define R320240_TUNE_SFREQ_Y            		(R320240_TUNE_FREQ_Y - 20)

#define R320240_DisplayDbm_X					(R320240_TUNE_FREQ_X+45)
#define R320240_DisplayDbm_Y					(R320240_TUNE_FREQ_Y-36)

#define R320240_MEMORYLABEL_X 					(R320240_TUNE_FREQ_X+45+(SMALL_FONT_WIDTH * 11)+4)
#define R320240_MEMORYLABEL_Y 					(R320240_TUNE_FREQ_Y-36)

#define R320240_BAND_MODE_X             		(R320240_TUNE_FREQ_X + 160)
#define R320240_BAND_MODE_Y             		(R320240_TUNE_FREQ_Y + 7)
#define R320240_BAND_MODE_MASK_X            	(R320240_BAND_MODE_X - 1)
#define R320240_BAND_MODE_MASK_Y            	(R320240_BAND_MODE_Y - 1)
#define R320240_BAND_MODE_MASK_H            	13
#define R320240_BAND_MODE_MASK_W            	33

// Demodulator mode control
#define R320240_DEMOD_MODE_X            		(R320240_TUNE_FREQ_X + 1)
#define R320240_DEMOD_MODE_Y            		(R320240_TUNE_FREQ_Y - 20)
#define R320240_DEMOD_MODE_MASK_X           	(R320240_DEMOD_MODE_X - 1)
#define R320240_DEMOD_MODE_MASK_Y           	(R320240_DEMOD_MODE_Y - 1)
#define R320240_DEMOD_MODE_MASK_H           	13
#define R320240_DEMOD_MODE_MASK_W           	41

// AGC display box
#define R320240_AGC_MASK_X (R320240_DEMOD_MODE_MASK_X-41)
#define R320240_AGC_MASK_Y R320240_DEMOD_MODE_MASK_Y
#define R320240_AGC_MASK_W (R320240_DEMOD_MODE_MASK_W-6)
#define R320240_AGC_MASK_H R320240_DEMOD_MODE_MASK_H

// Tunning step control
#define R320240_TUNE_STEP_X             		(R320240_TUNE_FREQ_X + 45)
#define R320240_TUNE_STEP_Y             		(R320240_TUNE_FREQ_Y - 21)
#define R320240_TUNE_STEP_MASK_H            	15
#define R320240_TUNE_STEP_MASK_W            	(SMALL_FONT_WIDTH*7)

#define R320240_PW_IND_X                		(R320240_DEMOD_MODE_X -1)
#define R320240_PW_IND_Y                		(R320240_DEMOD_MODE_Y - 16)
#define R320240_PW_IND_W						41

#define R320240_SM_IND_X                		116
#define R320240_SM_IND_Y                		0
#define R320240_SM_IND_W 						(200)
#define R320240_SM_IND_H 						(72 - 14)

#define R320240_LEFTBOXES_IND_X              	0
#define R320240_LEFTBOXES_IND_Y              	130
#define R320240_LEFTBOX_WIDTH 					58 // used for the lower left side controls
#define R320240_LEFTBOX_ROW_H  					28
#define R320240_LEFTBOX_ROW_2ND_OFF  			13

#define R320240_ENCODER_IND_X                	0
#define R320240_ENCODER_IND_Y                	16

#define R320240_DIGMODE_IND_X              		0
#define R320240_DIGMODE_IND_Y              		191
#define R320240_DIGMODE_IND_W					58
//------------------------------------------------------------------------------------------------------------------------------------------
//Touchscreen definitions for 480x320
static const touchaction_descr_t R480320_touchactions_normal[] =
{
#ifndef SDR_AMBER_480_320
	#ifndef OVI40_MOD_480_320
		{ {0,110,160,16}, UiAction_ToggleWaterfallScopeDisplay,    UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {(480/2)-16,110,48,16}, UiAction_ChangeSpectrumZoomLevelDown,    UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {(480/2)+100,110,48,16}, UiAction_ChangeSpectrumZoomLevelUp,      UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {R480320_TUNE_FREQ_X+16*7,R480320_TUNE_FREQ_Y,16*3,24}, UiAction_ChangeFrequencyToNextKhz,       NULL }, // Frequency display, last three digits :Set last 3 digits to zero
		{ {R480320_TUNE_FREQ_X,R480320_TUNE_FREQ_Y,16*7,24}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band
		{ {R480320_DEMOD_MODE_X,R480320_DEMOD_MODE_Y,R480320_DEMOD_MODE_MASK_W,R480320_DEMOD_MODE_MASK_H}, UiVk_ModSelVirtualKeys,                NULL }, // Demod Mode Box: mode switch
		{ {R480320_PW_IND_X,R480320_PW_IND_Y,64,30},								UiAction_ChangePowerLevel,               NULL }, // Power Box: TX Power Increase
		{ {R480320_BAND_MODE_X,R480320_BAND_MODE_Y,R480320_BAND_MODE_MASK_W/2,R480320_BAND_MODE_MASK_H}, UiAction_ChangeBandDownOrUp,             NULL }, // Left Part Band Display: Band down
		{ {R480320_BAND_MODE_X+R480320_BAND_MODE_MASK_W*3/4,R480320_BAND_MODE_Y,R480320_BAND_MODE_MASK_W/2,R480320_BAND_MODE_MASK_H}, UiAction_ChangeBandUpOrDown,             NULL }, // Right Part Band Display: Band up
		{ {R480320_LEFTBOXES_IND_X,R480320_LEFTBOXES_IND_Y,R480320_LEFTBOX_WIDTH,R480320_LEFTBOX_ROW_H}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box: Restart I2S
		{ {0,110,480,176}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions}, // Scope Draw Area: Tune to Touch
		{ {R480320_DIGMODE_IND_X,R480320_DIGMODE_IND_Y,R480320_DIGMODE_IND_W,16}, UiVk_ModSelVirtualKeys,              NULL }, // Digital Mode Box: Switch Digi Mode
		{ {R480320_TUNE_STEP_X,R480320_TUNE_STEP_Y,R480320_TUNE_STEP_MASK_W,R480320_TUNE_STEP_MASK_H}, UiAction_ChangeDynamicTuning,            NULL }, // Step Box: Dynamic Tuning Toggle
		{ {R480320_TUNE_STEP_X,R480320_TUNE_STEP_Y,R480320_TUNE_STEP_MASK_W,R480320_TUNE_STEP_MASK_H}, UiAction_ChangeDynamicTuning, NULL }, // Step Box: Dynamic Tuning Toggle
	#else
		{ {0,124,120,16}, UiAction_ToggleWaterfallScopeDisplay, UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {180,124,120,16}, UiAction_ChangeSpectrumZoomLevelDown, UiAction_ZoomResetToOne }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {360,124,120,16}, UiAction_ChangeSpectrumZoomLevelUp, UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {384,90,37,23}, UiAction_ChangeFrequencyToNextKhz, NULL }, // Frequency display, last three digits :Set last 3 digits to zero
		{ {276,90,104,23}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band
		{ {122,0,63,12}, UiVk_ModSelVirtualKeys, UiAction_ChangeDigitalMode }, // Digi Mode Box: mode switch
		{ {122,30,69,28}, UiVk_ModSelVirtualKeys, UiAction_ChangeDemodModeToAlternativeMode }, // Digi Mode Box: mode switch - double
		{ {188,0,14,12}, UiAction_ChangeDemodModeToAlternativeMode, NULL }, // Demod Mode Box: sideband switch
		{ {205,0,56,12}, UiAction_ChangePowerLevel, NULL }, // Power Box: TX Power Increase
		{ {70,14,42,28}, UiDriver_RIT_Reset,NULL }, // RIT Box: RIT Reset
		{ {446,88,14,15}, UiAction_ChangeBandDownOrUp, NULL }, // Left Part Band Display: Band down
		{ {465,88,14,15}, UiAction_ChangeBandUpOrDown, NULL }, // Right Part Band Display: Band up
		{ {193,30,69,28}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box
		{ {0,141,480,158}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
		{ {205,15,56,13}, UiAction_ChangeDynamicTuning, NULL }, // Step Box: Dynamic Tuning Toggle
	#endif
#else //SDR_AMBER_480_320
		//X,Y,W,H
		{ {0,124,120,16}, UiAction_ToggleWaterfallScopeDisplay, UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {180,124,120,16}, UiAction_ChangeSpectrumZoomLevelDown, UiAction_ZoomResetToOne }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {360,124,120,16}, UiAction_ChangeSpectrumZoomLevelUp, UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {384,90,37,23}, UiAction_ChangeFrequencyToNextKhz, NULL }, // Frequency display, last three digits :Set last 3 digits to zero
		{ {276,90,104,23}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band
		{ {122,0,63,12}, UiVk_ModSelVirtualKeys, UiAction_ChangeDigitalMode }, // Digi Mode Box: mode switch
		{ {122,30,69,28}, UiVk_ModSelVirtualKeys, UiAction_ChangeDemodModeToAlternativeMode }, // Digi Mode Box: mode switch - double
		{ {188,0,14,12}, UiAction_ChangeDemodModeToAlternativeMode, NULL }, // Demod Mode Box: sideband switch
		{ {205,0,56,12}, UiAction_ChangePowerLevel, NULL }, // Power Box: TX Power Increase
		{ {70,14,42,28}, UiDriver_RIT_Reset,NULL }, // RIT Box: RIT Reset
		{ {446,88,14,15}, UiAction_ChangeBandDownOrUp, NULL }, // Left Part Band Display: Band down
		{ {465,88,14,15}, UiAction_ChangeBandUpOrDown, NULL }, // Right Part Band Display: Band up
		{ {193,30,69,28}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box
		{ {0,141,480,158}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
		{ {205,15,56,13}, UiAction_ChangeDynamicTuning, NULL }, // Step Box: Dynamic Tuning Toggle
		{ {122,15,40,13}, UiAction_ChangeRXInputStateDown, NULL }, // Amber - Change RX Input State Down
		{ {162,15,40,13}, UiAction_ChangeRXInputStateUp, NULL }, // Amber - Change RX Input State Up
#endif
};

// this is the map for menu mode, right now only used for debugging/experimental purposes
static const touchaction_descr_t R480320_touchactions_menu[] =
{
		{ { R480320_SM_IND_X+R480320_SM_IND_W-16,R480320_SM_IND_Y,16,16 }, UiAction_ChangeDebugInfoDisplay, NULL}, // S-Meter db: toggle show tp coordinates
};

static const touchaction_list_descr_t R480320_touch_regions[] =
{
		// ATTENTION: the size calculation only works for true arrays, not for pointers!
		{ R480320_touchactions_normal, sizeof(R480320_touchactions_normal)/sizeof(*R480320_touchactions_normal) },
		{ R480320_touchactions_menu, sizeof(R480320_touchactions_menu)/sizeof(*R480320_touchactions_menu) },
};

//------------------------------------------------------------------------------------------------------------------------------------------
//Touchscreen definitions for 320x240
static const touchaction_descr_t R320240_touchactions_normal[] =
{
//		{ {64,128,60,16}, UiAction_ToggleWaterfallScopeDisplay,UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {60,128,64,16}, UiAction_ToggleWaterfallScopeDisplay,UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
//		{ {180,128,40,16}, UiAction_ChangeSpectrumZoomLevelDown,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {156,128,64,16}, UiAction_ChangeSpectrumZoomLevelDown,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Middle Part: Decrease Zoom Level
//		{ {280,128,40,16}, UiAction_ChangeSpectrumZoomLevelUp,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {252,128,64,16}, UiAction_ChangeSpectrumZoomLevelUp,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {R320240_TUNE_FREQ_X+16*7,R320240_TUNE_FREQ_Y,16*3,24},UiAction_ChangeFrequencyToNextKhz,NULL }, // Tune button:Set last 3 digits to zero
//		{ {R320240_TUNE_FREQ_X,R320240_TUNE_FREQ_Y,16*7,24}, UiVk_BndSelVirtualKeys,       NULL }, // Frequency display, first digits :Select the band
		{ {R320240_TUNE_FREQ_X,R320240_TUNE_FREQ_Y,16*7,24}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band/freq
//		{ {R320240_DEMOD_MODE_X,R320240_DEMOD_MODE_Y,R320240_DEMOD_MODE_MASK_W,R320240_DEMOD_MODE_MASK_H}, UiAction_ChangeDemodMode,NULL }, // Demod Mode Box: mode switch
		{ {R320240_DEMOD_MODE_X,R320240_DEMOD_MODE_Y,R320240_DEMOD_MODE_MASK_W,R320240_DEMOD_MODE_MASK_H}, UiAction_ChangeDemodModeToAlternativeMode,NULL }, // Demod Mode Box: mode switch
		{ {R320240_PW_IND_X,R320240_PW_IND_Y,64,16},UiAction_ChangePowerLevel,NULL }, // Power Box: TX Power Increase
        { {74,14,37,29}, UiDriver_RIT_Reset,NULL }, // RIT Box: RIT Reset
		{ {R320240_BAND_MODE_X,R320240_BAND_MODE_Y,R320240_BAND_MODE_MASK_W/2,R320240_BAND_MODE_MASK_H}, UiAction_ChangeBandDownOrUp,NULL }, // Left Part Band Display: Band down
		{ {R320240_BAND_MODE_X+R320240_BAND_MODE_MASK_W*3/4,R320240_BAND_MODE_Y,R320240_BAND_MODE_MASK_W/2,R320240_BAND_MODE_MASK_H}, UiAction_ChangeBandUpOrDown,NULL }, // Right Part Band Display: Band up
		{ {R320240_LEFTBOXES_IND_X,R320240_LEFTBOXES_IND_Y,R320240_LEFTBOX_WIDTH,R320240_LEFTBOX_ROW_H}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box: Restart I2S
//		{ {60,128,256,90}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
		{ {60,146,256,72}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
		{ {R320240_DIGMODE_IND_X,R320240_DIGMODE_IND_Y,R320240_DIGMODE_IND_W,16}, UiVk_ModeVirtualKeys,UiAction_ChangeDigitalMode }, // Digital Mode Box: Switch Digi Mode
		{ {R320240_TUNE_STEP_X,R320240_TUNE_STEP_Y,R320240_TUNE_STEP_MASK_W,R320240_TUNE_STEP_MASK_H}, UiAction_ChangeDynamicTuning,NULL }, // Step Box: Dynamic Tuning Toggle
};
//------------------------------------------------------------------------------------------------------------------------------------------
////Touchscreen definitions for 320x240 Alternative wide spectrum mode
static const touchaction_descr_t R320240_touchactions_widespectrum[] =
{
#ifndef SDR_AMBER // not Amber
		//{X, Y, W, H}
		{ {0,136,80,13}, UiAction_ToggleWaterfallScopeDisplay,UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {120,136,80,13}, UiAction_ChangeSpectrumZoomLevelDown,UiAction_ZoomResetToOne }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {240,136,80,13}, UiAction_ChangeSpectrumZoomLevelUp,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {226,109,48,24},UiAction_ChangeFrequencyToNextKhz,NULL }, // Tune button:Set last 3 digits to zero
//		{ {118,109,105,24}, UiVk_BndSelVirtualKeys,       NULL }, // Frequency display, first digits :Select the band
		{ {118,109,105,24}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band/freq
//		{ {113,87,41,13}, UiAction_ChangeDemodMode,NULL }, // Demod Mode Box: mode switch
//		{ {113,87,41,13}, UiAction_ChangeDemodModeToAlternativeMode,NULL }, // Demod Mode Box: sideband switch
		{ {138,71,16,12}, UiAction_ChangeDemodModeToAlternativeMode,NULL }, // Demod Mode Box: sideband switch
		{ {113,56,41,13},UiAction_ChangePowerLevel,NULL }, // Power Box: TX Power Increase
		{ {74,14,37,26}, UiDriver_RIT_Reset,NULL }, // RIT Box: RIT Reset
		{ {287,108,15,16}, UiAction_ChangeBandDownOrUp,NULL }, // Left Part Band Display: Band down
		{ {305,108,15,16}, UiAction_ChangeBandUpOrDown,NULL }, // Right Part Band Display: Band up
		{ {217,56,58,29}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box: Restart I2S
		{ {1,152,318,73}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
//		{ {74,71,80,13}, UiAction_ChangeDigitalMode,UiVk_ModeVirtualKeys }, // Digital Mode Box: Switch Digi Mode
		{ {74,71,61,13}, UiVk_ModeVirtualKeys,UiAction_ChangeDigitalMode }, // Digital Mode Box: Switch Digi Mode
//		{ {157,87,57,13}, UiAction_ChangeDynamicTuning,NULL }, // Step Box: Dynamic Tuning Toggle
		{ {113,87,41,13}, UiAction_ChangeDynamicTuning,NULL }, // Step Box: Dynamic Tuning Toggle
#else //Amber 320x240
		//{X, Y, W, H}
		{ {0,136,80,13}, UiAction_ToggleWaterfallScopeDisplay,UiAction_ChangeSpectrumSize }, // Spectrum Bar Left Part: WaterfallScope Toggle
		{ {120,136,80,13}, UiAction_ChangeSpectrumZoomLevelDown,UiAction_ZoomResetToOne }, // Spectrum Bar Middle Part: Decrease Zoom Level
		{ {240,136,80,13}, UiAction_ChangeSpectrumZoomLevelUp,UiAction_CheckSpectrumTouchActions }, // Spectrum Bar Right Part: Increase Zoom Level
		{ {226,109,48,24}, UiAction_ChangeFrequencyToNextKhz, NULL }, // Tune button:Set last 3 digits to zero
		{ {118,109,105,24}, UiVk_BndSelVirtualKeys, UiVk_BndFreqSetVirtualKeys }, // Frequency display, first digits :Select the band/freq
		{ {138,71,16,12}, UiAction_ChangeDemodModeToAlternativeMode, NULL }, // Demod Mode Box: sideband switch
		{ {113,56,41,13}, UiAction_ChangePowerLevel, NULL }, // Power Box: TX Power Increase
		{ {74,14,37,26}, UiDriver_RIT_Reset,NULL }, // RIT Box: RIT Reset
		{ {287,108,15,16}, UiAction_ChangeBandDownOrUp, NULL }, // Left Part Band Display: Band down
		{ {305,108,15,16}, UiAction_ChangeBandUpOrDown, NULL }, // Right Part Band Display: Band up
		{ {217,56,58,28}, UiVk_DSPVirtualKeys, Codec_RestartI2S }, // DSP Box: Restart I2S
		{ {1,152,318,73}, UiAction_ChangeFrequencyByTouch, UiAction_CheckSpectrumTouchActions }, // Scope Draw Area: Tune to Touch
//		{ {74,71,61,13}, UiAction_ChangeDigitalMode, UiVk_ModeVirtualKeys }, // Digital Mode Box: Switch Digi Mode
		{ {74,71,61,13}, UiVk_ModeVirtualKeys, UiAction_ChangeDigitalMode }, // Digital Mode Box: Switch Digi Mode
		{ {157,56,58,28}, UiVk_ModeVirtualKeys, UiAction_ChangeDigitalMode }, // Digital Mode Box - double touch zone
		{ {157,86,57,13}, UiAction_ChangeDynamicTuning, NULL }, // Step Box: Dynamic Tuning Toggle
		{ {74,86,40,13}, UiAction_ChangeRXInputStateDown, NULL }, // Amber - Change RX Input State Down
		{ {114,86,40,13}, UiAction_ChangeRXInputStateUp, NULL }, // Amber - Change RX Input State Up
#endif
};

// this is the map for menu mode, right now only used for debugging/experimental purposes
static const touchaction_descr_t R320240_touchactions_menu[] =
{
		{ { R320240_SM_IND_X+R320240_SM_IND_W-16,R320240_SM_IND_Y,16,16 }, UiAction_ChangeDebugInfoDisplay, NULL}, // S-Meter db: toggle show tp coordinates
};

static const touchaction_list_descr_t R320240__touch_regions[] =
{
		// ATTENTION: the size calculation only works for true arrays, not for pointers!
		{ R320240_touchactions_normal, sizeof(R320240_touchactions_normal)/sizeof(*R320240_touchactions_normal) },
		{ R320240_touchactions_menu, sizeof(R320240_touchactions_menu)/sizeof(*R320240_touchactions_menu) },
};

static const touchaction_list_descr_t R320240__touch_regions_widespectrum[] =
{
		// ATTENTION: the size calculation only works for true arrays, not for pointers!
		{ R320240_touchactions_widespectrum, sizeof(R320240_touchactions_widespectrum)/sizeof(*R320240_touchactions_widespectrum) },
		{ R320240_touchactions_menu, sizeof(R320240_touchactions_menu)/sizeof(*R320240_touchactions_menu) },
};
//------------------------------------------------------------------------------------------------------------------------------------------
const LcdLayout LcdLayouts[LcdLayoutsCount]=
{
		//-----------------------------------------------------------------
		{		//320x240
				.Size = { 320, 240 },
				.StartUpScreen_START = { 0, 10 },
				.SpectrumWindow = {  .x = 58, .y = 128, .w = 260, .h = 94  },

				.SpectrumWindowPadding=2,

				.TUNE_FREQ= { R320240_TUNE_FREQ_X, R320240_TUNE_FREQ_Y },
				.TUNE_SPLIT_FREQ_X=R320240_TUNE_SPLIT_FREQ_X,
				.TUNE_SPLIT_MARKER_X=R320240_TUNE_SPLIT_MARKER_X,
				.TUNE_SPLIT_FREQ_Y_TX=R320240_TUNE_SPLIT_FREQ_Y_TX,
				.TUNE_SFREQ = { R320240_TUNE_SFREQ_X, R320240_TUNE_SFREQ_Y },
				.DisplayDbm = { R320240_DisplayDbm_X, R320240_DisplayDbm_Y },
				.MEMORYLABEL= { R320240_MEMORYLABEL_X, R320240_MEMORYLABEL_Y} ,

				.BAND_MODE = { R320240_BAND_MODE_X, R320240_BAND_MODE_Y},
				.BAND_MODE_MASK = { .x = R320240_BAND_MODE_MASK_X, .y = R320240_BAND_MODE_MASK_Y, .h = R320240_BAND_MODE_MASK_H, .w = R320240_BAND_MODE_MASK_W },

				.DEMOD_MODE_MASK = { .x = R320240_DEMOD_MODE_MASK_X, .y = R320240_DEMOD_MODE_MASK_Y, .h = R320240_DEMOD_MODE_MASK_H, .w = R320240_DEMOD_MODE_MASK_W},
				.AGC_MASK = {.x = R320240_AGC_MASK_X, .y = R320240_AGC_MASK_Y, .h = R320240_AGC_MASK_H,
						                .w = R320240_AGC_MASK_W},
				.TUNE_STEP={.x=R320240_TUNE_STEP_X, .y=R320240_TUNE_STEP_Y, .h=R320240_TUNE_STEP_MASK_H, .w=R320240_TUNE_STEP_MASK_W},

				.ENCODER_IND = { R320240_ENCODER_IND_X, R320240_ENCODER_IND_Y},
				.ENCODER_MODE=MODE_VERTICAL,

				.DIGMODE={.x=R320240_DIGMODE_IND_X,.y=R320240_DIGMODE_IND_Y,.w=R320240_DIGMODE_IND_W},

				.LEFTBOXES_IND = { .x=R320240_LEFTBOXES_IND_X, .y=R320240_LEFTBOXES_IND_Y, .w=R320240_LEFTBOX_WIDTH, .h=R320240_LEFTBOX_ROW_H},
				.LEFTBOXES_MODE=MODE_VERTICAL,
				.LEFTBOXES_ROW_2ND_OFF=R320240_LEFTBOX_ROW_2ND_OFF,

				.PW_IND = { .x=R320240_PW_IND_X, .y=R320240_PW_IND_Y, .w=R320240_PW_IND_W},
				.TEMP_IND={.x = 0, .y = 0},
				.RTC_IND={.x= 0, .y = 79},

				.LOADANDDEBUG_Y=95,
				.DEBUG_X=0,
				.LOAD_X=280,

				.PWR_NUM_IND = { 1, 80},

				.CW_DECODER_WPM = { 0, 108},

				.SNAP_CARRIER = { 27, 122},

				.TextMsgLine = { 5, 92},
				.TextMsg_buffer_max=44,
				.TextMsg_font=4,

				.FREEDV_SNR = { 5, 116},
				.FREEDV_BER = { 5, 104},
				.FREEDV_FONT=4,

				.SM_IND={.x=R320240_SM_IND_X,.y=R320240_SM_IND_Y,.h=R320240_SM_IND_H,.w=R320240_SM_IND_W},
				.PWR_IND={.x = 4, .y = 193+15},

				.BOTTOM_BAR={.x=0,.y=228, .h=16, .w=62},

				.MENUSIZE=7,
				.MENU_IND = { 60, 128},
				.MENU_CHANGE_X=244,
				.MENU_CURSOR_X=311,
				.MENU_TEXT_SIZE_MAX=34,

				.touchaction_list=R320240__touch_regions
		},

		//-----------------------------------------------------------------
		{		//480x320
#ifndef SDR_AMBER //OVI40, Sparrow
	#ifndef OVI40_MOD_480_320
				.Size = { 480, 320},
				.StartUpScreen_START ={ 80, 60},
				.SpectrumWindow={ .x = 0, .y = 110, .w = 480, .h = 176 },
				.SpectrumWindowPadding=0,

				.TUNE_FREQ= { R480320_TUNE_FREQ_X, R480320_TUNE_FREQ_Y},
				.TUNE_SPLIT_FREQ_X=R480320_TUNE_SPLIT_FREQ_X,
				.TUNE_SPLIT_MARKER_X=R480320_TUNE_SPLIT_MARKER_X,
				.TUNE_SPLIT_FREQ_Y_TX=R480320_TUNE_SPLIT_FREQ_Y_TX,
				.TUNE_SFREQ = { R480320_TUNE_SFREQ_X, R480320_TUNE_SFREQ_Y},
				.DisplayDbm = { R480320_DisplayDbm_X, R480320_DisplayDbm_Y},
				.MEMORYLABEL = { R480320_MEMORYLABEL_X, R480320_MEMORYLABEL_Y},

				.BAND_MODE = { R480320_BAND_MODE_X, R480320_BAND_MODE_Y},
				.BAND_MODE_MASK = { .x = R480320_BAND_MODE_MASK_X, .y = R480320_BAND_MODE_MASK_Y, .h = R480320_BAND_MODE_MASK_H, .w= R480320_BAND_MODE_MASK_W},

				.DEMOD_MODE_MASK = { .x = R480320_DEMOD_MODE_MASK_X, .y = R480320_DEMOD_MODE_MASK_Y, .h = R480320_DEMOD_MODE_MASK_H,
                .w = R480320_DEMOD_MODE_MASK_W},
				.AGC_MASK = {.x = R480320_AGC_MASK_X, .y = R480320_AGC_MASK_Y, .h = R480320_AGC_MASK_H,
		                .w = R480320_AGC_MASK_W},
				.TUNE_STEP={.x=R480320_TUNE_STEP_X, .y=R480320_TUNE_STEP_Y, .h=R480320_TUNE_STEP_MASK_H, .w=R480320_TUNE_STEP_MASK_W},

				.ENCODER_IND = { R480320_ENCODER_IND_X, R480320_ENCODER_IND_Y},
				.ENCODER_MODE=MODE_HORIZONTAL,

				.DIGMODE={.x=R480320_DIGMODE_IND_X,.y=R480320_DIGMODE_IND_Y,.w=R480320_DIGMODE_IND_W},

				.LEFTBOXES_IND = { .x=R480320_LEFTBOXES_IND_X, .y=R480320_LEFTBOXES_IND_Y, .w=R480320_LEFTBOX_WIDTH, .h=R480320_LEFTBOX_ROW_H},
				.LEFTBOXES_MODE=MODE_HORIZONTAL,
				.LEFTBOXES_ROW_2ND_OFF=R480320_LEFTBOX_ROW_2ND_OFF,

				.PW_IND = { .x=R480320_PW_IND_X, .y=R480320_PW_IND_Y, .w=R480320_PW_IND_W},
				.TEMP_IND={.x = 370,.y = 64},
				.RTC_IND={.x = 415,.y = 80},

				.LOADANDDEBUG_Y=96,
				.DEBUG_X=0,
				.LOAD_X=280,

				.PWR_NUM_IND ={ 320, 96},

				.CW_DECODER_WPM = { 420, 290},

				.SNAP_CARRIER= { 242, 29},

				.TextMsgLine = { 0, 290 },
				.TextMsg_buffer_max=50,
				.TextMsg_font=0,

				//.FREEDV_SNR = { 280, 28},
				//.FREEDV_BER = {380, 28},
				.FREEDV_SNR = {410,288},
				.FREEDV_BER = {410,297},
				.FREEDV_FONT=4,

				.SM_IND={.x = R480320_SM_IND_X, .y = R480320_SM_IND_Y, .h = R480320_SM_IND_H, .w = R480320_SM_IND_W},
				.PWR_IND={ .x = 420, .y = 307},

  #ifdef UI_BRD_OVI40
				.BOTTOM_BAR={.x=0,.y=308, .h=16, .w=74},
  #else
				.BOTTOM_BAR={.x=32,.y=308, .h=16, .w=74},
  #endif
				.MENUSIZE=14,
				.MENU_IND = { 80, 110 },
				.MENU_CHANGE_X=280,
				.MENU_CURSOR_X=360,
				.MENU_TEXT_SIZE_MAX=40,

				.touchaction_list=R480320_touch_regions
	#else
				.Size = { 480, 320 },
				.StartUpScreen_START = { 80, 60 },
				.SpectrumWindow = { .x = 0, .y = 123, .w = 480, .h = 174 },
				.SpectrumWindowPadding = 0,

				.TUNE_FREQ = { 276, 90 },
				.TUNE_SPLIT_FREQ_X = 290,
				.TUNE_SPLIT_MARKER_X = 276,
				.TUNE_SPLIT_FREQ_Y_TX = 276,
				.TUNE_SFREQ = { 396, 60 },
				.DisplayDbm = { 273, 60 },
				.MEMORYLABEL = { 355, 60 },
				.BAND_MODE = { 436, 90 },
				.BAND_MODE_MASK = { .x = 435, .y = 89, .h = 13, .w = 33 },
				.DEMOD_MODE_MASK = { .x = 186, .y = 0, .h = 13, .w = 16 },
				.AGC_MASK = {.x = 122, .y = 15, .h = 13, .w = 80 },
				.TUNE_STEP={.x = 205, .y = 15, .h = 13, .w = 56 },
				.ENCODER_IND = { 0, 15 },
				.ENCODER_MODE = MODE_VERTICAL,
				.DIGMODE={.x = 122, .y = 0, .w = 63},
				.LEFTBOXES_IND = { .x = 122, .y=30, .w = 69, .h = 28 },
				.LEFTBOXES_MODE = MODE_HORIZONTAL,
				.LEFTBOXES_ROW_2ND_OFF = 13,
				.PW_IND = { .x = 205, .y = 0, .w = 56 },
				.TEMP_IND = { .x = 0, .y = 0 },
				.RTC_IND = { .x = 127, .y = 60 },

				.LOADANDDEBUG_Y = 75,
				.DEBUG_X = 0,
				.LOAD_X = 440,

				.PWR_NUM_IND = { 320, 99 },

				.CW_DECODER_WPM = { 0, 92 },

				.SNAP_CARRIER = { 27, 107 },
				.TextMsgLine = { 0, 75 },
				.TextMsg_buffer_max = 50,
				.TextMsg_font = 0,

				//.FREEDV_SNR = { 280, 28},
				//.FREEDV_BER = {380, 28},
				.FREEDV_SNR = { 0, 90 },
				.FREEDV_BER = { 0, 102 },
				.FREEDV_FONT = 4,

				.SM_IND = { .x = 273, .y = 0, .h = 56, .w = 203 },
				.PWR_IND = { .x = 207, .y = 60 },
				.BOTTOM_BAR = {.x=0,.y=308, .h=16, .w=79},//.w=74},

				.MENUSIZE = 14,
				.MENU_IND = { 140, 129 },
				.MENU_CHANGE_X = 280,
				.MENU_CURSOR_X = 360,
				.MENU_TEXT_SIZE_MAX = 40,

				.touchaction_list = R480320_touch_regions
	#endif
#else //Amber_480x320
				.Size = { 480, 320 },
				.StartUpScreen_START = { 80, 60 },
				.SpectrumWindow = { .x = 0, .y = 123, .w = 480, .h = 174 },
				.SpectrumWindowPadding = 0,

				.TUNE_FREQ = { 276, 90 },
				.TUNE_SPLIT_FREQ_X = 356,
				.TUNE_SPLIT_MARKER_X = 316,
				.TUNE_SPLIT_FREQ_Y_TX = 102,
				.TUNE_SFREQ = { 396, 60 },
				.DisplayDbm = { 273, 60 },
				.MEMORYLABEL = { 355, 60 },
				.BAND_MODE = { 436, 90 },
				.BAND_MODE_MASK = { .x = 435, .y = 89, .h = 13, .w = 33 },
				.DEMOD_MODE_MASK = { .x = 186, .y = 0, .h = 13, .w = 16 },
				.AGC_MASK = {.x = 122, .y = 15, .h = 13, .w = 80 },
				.TUNE_STEP={.x = 205, .y = 15, .h = 13, .w = 56 },
				.ENCODER_IND = { 0, 15 },
				.ENCODER_MODE = MODE_VERTICAL,
				.DIGMODE={.x = 122, .y = 0, .w = 63},
				.LEFTBOXES_IND = { .x = 122, .y=30, .w = 69, .h = 28 },
				.LEFTBOXES_MODE = MODE_HORIZONTAL,
				.LEFTBOXES_ROW_2ND_OFF = 13,
				.PW_IND = { .x = 205, .y = 0, .w = 56 },
				.TEMP_IND = { .x = 0, .y = 0 },
				.RTC_IND = { .x = 127, .y = 60 },

				.LOADANDDEBUG_Y = 75,
				.DEBUG_X = 0,
				.LOAD_X = 440,

				.PWR_NUM_IND = { 320, 99 },

				.CW_DECODER_WPM = { 0, 92 },

				.SNAP_CARRIER = { 27, 107 },
				.TextMsgLine = { 0, 75 },
				.TextMsg_buffer_max = 50,
				.TextMsg_font = 0,

				//.FREEDV_SNR = { 280, 28},
				//.FREEDV_BER = {380, 28},
				.FREEDV_SNR = { 0, 90 },
				.FREEDV_BER = { 0, 102 },
				.FREEDV_FONT = 4,

				.SM_IND = { .x = 273, .y = 0, .h = 56, .w = 203 },
				.PWR_IND = { .x = 207, .y = 60 },
		#ifndef SDR_AMBER_4INCH
				.BOTTOM_BAR = {.x=0,.y=308, .h=16, .w=79},//.w=74},
		#else
				.BOTTOM_BAR = {.x=2,.y=308, .h=16, .w=67},
		#endif

				.MENUSIZE = 14,
				.MENU_IND = { 140, 129 },//{ 80, 130 },
				.MENU_CHANGE_X = 280,
				.MENU_CURSOR_X = 430,//360,
				.MENU_TEXT_SIZE_MAX = 40,

				.touchaction_list = R480320_touch_regions
#endif
		},
		//-----------------------------------------------------------------
		{		//800x480
				.Size = { 800, 480},
				.StartUpScreen_START ={ 80, 60},
				.SpectrumWindow={ .x = 0, .y = 110, .w = 480, .h = 176 },
				.SpectrumWindowPadding=0,

				.TUNE_FREQ= { R480320_TUNE_FREQ_X, R480320_TUNE_FREQ_Y},
				.TUNE_SPLIT_FREQ_X=R480320_TUNE_SPLIT_FREQ_X,
				.TUNE_SPLIT_MARKER_X=R480320_TUNE_SPLIT_MARKER_X,
				.TUNE_SPLIT_FREQ_Y_TX=R480320_TUNE_SPLIT_FREQ_Y_TX,
				.TUNE_SFREQ = { R480320_TUNE_SFREQ_X, R480320_TUNE_SFREQ_Y},
				.DisplayDbm = { R480320_DisplayDbm_X, R480320_DisplayDbm_Y},
				.MEMORYLABEL = { R480320_MEMORYLABEL_X, R480320_MEMORYLABEL_Y},

				.BAND_MODE = { R480320_BAND_MODE_X, R480320_BAND_MODE_Y},
				.BAND_MODE_MASK = { .x = R480320_BAND_MODE_MASK_X, .y = R480320_BAND_MODE_MASK_Y, .h = R480320_BAND_MODE_MASK_H, .w= R480320_BAND_MODE_MASK_W},

				.DEMOD_MODE_MASK = { .x = R480320_DEMOD_MODE_MASK_X, .y = R480320_DEMOD_MODE_MASK_Y, .h = R480320_DEMOD_MODE_MASK_H,
                .w = R480320_DEMOD_MODE_MASK_W},
				.AGC_MASK = {.x = R480320_AGC_MASK_X, .y = R480320_AGC_MASK_Y, .h = R480320_AGC_MASK_H,
		                .w = R480320_AGC_MASK_W},
				.TUNE_STEP={.x=R480320_TUNE_STEP_X, .y=R480320_TUNE_STEP_Y, .h=R480320_TUNE_STEP_MASK_H, .w=R480320_TUNE_STEP_MASK_W},

				.ENCODER_IND = { R480320_ENCODER_IND_X, R480320_ENCODER_IND_Y},
				.ENCODER_MODE=MODE_HORIZONTAL,

				.DIGMODE={.x=R480320_DIGMODE_IND_X,.y=R480320_DIGMODE_IND_Y,.w=R480320_DIGMODE_IND_W},

				.LEFTBOXES_IND = { .x=R480320_LEFTBOXES_IND_X, .y=R480320_LEFTBOXES_IND_Y, .w=R480320_LEFTBOX_WIDTH, .h=R480320_LEFTBOX_ROW_H},
				.LEFTBOXES_MODE=MODE_HORIZONTAL,
				.LEFTBOXES_ROW_2ND_OFF=R480320_LEFTBOX_ROW_2ND_OFF,

				.PW_IND = { .x=R480320_PW_IND_X, .y=R480320_PW_IND_Y, .w=R480320_PW_IND_W},
				.TEMP_IND={.x = 370,.y = 64},
				.RTC_IND={.x = 415,.y = 80},

				.LOADANDDEBUG_Y=96,
				.DEBUG_X=0,
				.LOAD_X=280,

				.PWR_NUM_IND ={ 320, 96},

				.CW_DECODER_WPM = { 420, 290},

				.SNAP_CARRIER= { 242, 29},

				.TextMsgLine = { 0, 290 },
				.TextMsg_buffer_max=50,
				.TextMsg_font=0,

				//.FREEDV_SNR = { 280, 28},
				//.FREEDV_BER = {380, 28},
				.FREEDV_SNR = {410,288},
				.FREEDV_BER = {410,297},
				.FREEDV_FONT=4,

				.SM_IND={.x = R480320_SM_IND_X, .y = R480320_SM_IND_Y, .h = R480320_SM_IND_H, .w = R480320_SM_IND_W},
				.PWR_IND={ .x = 420, .y = 307},

#ifdef UI_BRD_OVI40
				.BOTTOM_BAR={.x=0,.y=308, .h=16, .w=74},
#else
				.BOTTOM_BAR={.x=32,.y=308, .h=16, .w=74},
#endif
				.MENUSIZE=14,
				.MENU_IND = { 80, 110 },
				.MENU_CHANGE_X=280,
				.MENU_CURSOR_X=360,
				.MENU_TEXT_SIZE_MAX=40,

				.touchaction_list=R480320_touch_regions
//		}
		},
		//-----------------------------------------------------------------
		{		//320x240_wide_spectrum
				.Size = { 320, 240 },
				.StartUpScreen_START = { 0, 10 },
				.SpectrumWindow = { .x = 4, .y = 136, .w = 312, .h = 89 },//x0_w320

				.SpectrumWindowPadding = 0,

				.TUNE_FREQ = { 116, 109 },
				.TUNE_SPLIT_FREQ_X = 196,
				.TUNE_SPLIT_MARKER_X = 167,
				.TUNE_SPLIT_FREQ_Y_TX = 121,
				.TUNE_SFREQ = { 236, 87 },
				.DisplayDbm = { 277, 56 },
#ifdef SDR_AMBER
				.MEMORYLABEL= { 64, 123 } ,
#else
				.MEMORYLABEL= { 157, 87 } ,
#endif
				.BAND_MODE = { 276, 112 },
				.BAND_MODE_MASK = { .x = 275, .y = 111, .h = 13, .w = 33 },
				.DEMOD_MODE_MASK = { .x = 138, .y = 71, .h = 12, .w = 16 },
#ifndef SDR_AMBER
//				.DEMOD_MODE_MASK = { .x = 113, .y = 86, .h = 13, .w = 41 },
				.AGC_MASK = { .x = 74, .y = 86, .h = 13, .w = 36 },
				.TUNE_STEP={ .x = 113, .y = 86, .h = 13, .w = 41 },
#else
//				.DEMOD_MODE_MASK = { .x = 138, .y = 71, .h = 12, .w = 16 },
				.AGC_MASK = { .x = 74, .y = 86, .h = 13, .w = 80 },
				.TUNE_STEP={ .x = 157, .y = 86, .h = 13, .w = 57 },
#endif
//				.TUNE_STEP={ .x = 157, .y = 86, .h = 13, .w = 57 },

				.ENCODER_IND = { 0, 0 },
				.ENCODER_MODE = MODE_VERTICAL,
//#ifndef SDR_AMBER
//				.DIGMODE= { .x = 74, .y = 71, .w = 80 },
//#else
				.DIGMODE= { .x = 74, .y = 71, .w = 63 },
//#endif
				.LEFTBOXES_IND = { .x = 157, .y = 56, .w = 58, .h = 28 },
				.LEFTBOXES_MODE = MODE_HORIZONTAL,
				.LEFTBOXES_ROW_2ND_OFF = 13,

				.PW_IND = { .x = 113, .y = 56, .w = 41 },
				.TEMP_IND={ .x = 0, .y = 35 },
				.RTC_IND={ .x= 5, .y = 86 },

				.LOADANDDEBUG_Y = 100,
				.DEBUG_X = 0,
				.LOAD_X = 280,

				.PWR_NUM_IND = { 1, 100 },

				.CW_DECODER_WPM = { 0, 113 },

				.SNAP_CARRIER = { 27, 128 },

				.TextMsgLine = { 5, 99 },
				.TextMsg_buffer_max = 44,
				.TextMsg_font = 4,

				.FREEDV_SNR = { 5, 123 },
				.FREEDV_BER = { 5, 113 },
				.FREEDV_FONT = 4,

				.SM_IND = { .x = 113, .y = 0, .h = 32, .w = 203 },
				.PWR_IND = { .x = 16, .y = 72 },

				.BOTTOM_BAR = { .x = 2, .y = 231, .h = 13, .w=62 },

				.MENUSIZE = 7,
				.MENU_IND = { 60, 140 },
				.MENU_CHANGE_X = 244,
				.MENU_CURSOR_X = 310,
				.MENU_TEXT_SIZE_MAX = 34,

				.touchaction_list = R320240__touch_regions_widespectrum
		}
};


disp_resolution_t disp_resolution;
#endif
