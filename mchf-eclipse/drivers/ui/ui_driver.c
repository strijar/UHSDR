/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
 **                                                                                 **
 **                                        UHSDR                                    **
 **               a powerful firmware for STM32 based SDR transceivers              **
 **                                                                                 **
 **---------------------------------------------------------------------------------**
 **                                                                                 **
 **  File name:                                                                     **
 **  Description:                                                                   **
 **  Last Modified:                                                                 **
 **  Licence:		GNU GPLv3                                                      **
 ************************************************************************************/

// Common
#include "uhsdr_board.h"
#include "profiling.h"
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <src/uhsdr_version.h>
#include "hardware/uhsdr_board_config.h"
#include "ui_lcd_layouts.h"
#include "ui_menu.h"
#include "uhsdr_rtc.h"
#include "adc.h"
#include "drivers/ui/oscillator/osc_si5351a.h"
#include "audio_nr.h"
#include "uhsdr_keypad.h"
#include "serial_eeprom.h"
#include "ui.h"
// LCD
#include "ui_lcd_hy28.h"
#include "ui_spectrum.h"

#include "freedv_uhsdr.h"

// Encoders
#include "ui_rotary.h"

#include "cat_driver.h"

// Codec control
#include "codec.h"

#include "audio_management.h"
#include "ui_driver.h"

#include "ui_configuration.h"
#include "config_storage.h"

#include "cw_gen.h"
#include "uhsdr_digi_buffer.h"

#include "radio_management.h"
#include "soft_tcxo.h"

#include "rtty.h"
#include "cw_decoder.h"
#include "psk.h"

#include "audio_convolution.h"
#include "audio_agc.h"

#define SPLIT_ACTIVE_COLOUR         		Yellow      // colour of "SPLIT" indicator when active
#define SPLIT_INACTIVE_COLOUR           	Grey        // colour of "SPLIT" indicator when NOT active

#define TOUCH_SHOW_REGIONS_AND_POINTS       //this definition enables the drawing of boxes of regions and put the pixel in touch point

static uint32_t UiDriver_DarkColor(uint32_t color);
static void     UiDriver_MapColors();

static void     UiDriver_CreateMeters(void);
static void     UiDriver_DeleteMeters(void);
static void 	UiDriver_UpdateMeterRX(uchar val);
static void 	UiDriver_UpdateMeterTX(float val, uchar warn);

static void 	UiDriver_InitFrequency(void);

static void     UiDriver_UpdateLcdFreq(ulong dial_freq,ushort color,ushort mode);
static bool 	UiDriver_IsButtonPressed(ulong button_num);
static void		UiDriver_TimeScheduler(void);				// Also handles audio gain and switching of audio on return from TX back to RX
static void 	UiDriver_ChangeToNextDemodMode(bool select_alternative_mode);
static void 	UiDriver_ChangeBand(bool is_up);
static bool 	UiDriver_CheckFrequencyEncoder(void);

static void     UiDriver_DisplayBand(const BandInfo* band);
static void     UiDriver_DisplayBandForFreq(uint32_t freq, bool force);

static void     UiDriver_DisplayEncoderMode(uint8_t enc);

static void     UiDriver_DisplayAfGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRfGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplaySQL(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRit(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRttySpeed(uint8_t enc, uint8_t style);
static void     UiDriver_DisplaySidetoneGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRttyShift(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayPskSpeed(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRfGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayRit(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayPWRGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayCmpLevel(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayBas(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayTreble(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayKeyerSpeed(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayNoiseBlanker(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayNR(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayAGC(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayNotch(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayPeak(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayInGain(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayIn(uint8_t enc, uint8_t style);
static void     UiDriver_DisplayMeter(uint8_t enc, uint8_t style);

#ifdef SDR_AMBER
static void     UiDriver_DisplayPre(uint8_t enc, uint8_t style);
#endif

static void     UiDriver_DisplayDSPMode(void);
static void 	UiDriver_DisplayPowerLevel(void);
static void     UiDriver_DisplayTemperature(int temp);
static void     UiDriver_DisplayVoltage(void);

static void 	UiDriver_HandleSMeter(void);
static void 	UiDriver_HandleTXMeters(void);
static bool     UiDriver_HandleVoltage(void);

static void     UiDriver_CreateSomeKindDisplay(void);

static void 	UiDriver_HandleLoTemperature(void);

static bool	    UiDriver_LoadSavedConfigurationAtStartup(void);
static bool	    UiDriver_TouchscreenCalibration(void);

static void     UiDriver_PowerDownCleanup(bool saveConfiguration);

static void UiDriver_HandlePowerLevelChange(const BandInfo* band, uint8_t power_level);
static void UiDriver_HandleBandButtons(uint16_t button);

static void UiDriver_KeyTestScreen(void);

static bool UiDriver_SaveConfiguration(void);


static inline void UiDriver_FButton_F4ActiveVFO(void);
static inline void UiDriver_FButton_F5Tune(void);
static void UiAction_ShowMems(void);

#ifdef SDR_AMBER
static void UiDriver_ShowGreeting(uint8_t variant);
#endif

static void UiDriver_LoadXVTR1(void);
static void UiDriver_LoadXVTR2(void);
static void UiDriver_LoadXVTR3(void);
static void UiDriver_LoadXVTR4(void);
static void UiDriver_StoreXVTR1(void);
static void UiDriver_StoreXVTR2(void);
static void UiDriver_StoreXVTR3(void);
static void UiDriver_StoreXVTR4(void);
static void UiAction_ToggleListXVTR(void);
static void UiAction_OffXVTR(void);
static void UiDriver_LoadBwData(void);
static void UiDriver_ChoiceKeyer(void);
static void UiDriver_ChoiceExt(void);
static void UiAction_ChoiceMems(void);
static void UiDriver_ChoiceXvtr(void);
static void UiDriver_ChoiceNormal(void);

typedef enum {
    ENC_MODE_AUDIO_GAIN = 0,
    ENC_MODE_BASS_GAIN,
    ENC_MODE_TREBLE_GAIN,
    ENC_MODE_CMP_LEVEL,
    ENC_MODE_PWR_GAIN,
    ENC_MODE_ST_GAIN,
    ENC_MODE_RF_GAIN,
    ENC_MODE_RIT,
    ENC_MODE_CW_SPEED,
    ENC_MODE_PSK_SPEED,
    ENC_MODE_RTTY_SPEED,
    ENC_MODE_RTTY_SHIFT,
    ENC_MODE_NB,
    ENC_MODE_NOTCH_F,
    ENC_MODE_NR,
    ENC_MODE_AGC,
    ENC_MODE_SQL,
    ENC_MODE_PEAK_F,
    ENC_MODE_INPUT_GAIN,
    ENC_MODE_INPUT,
    ENC_MODE_METER,
#ifdef SDR_AMBER
    ENC_MODE_PRE,
#endif
    ENC_NUM_MODES
} EncoderModes;

#define ENC_STATE_BITS  6
#define ENC_MODE_MASK   ((1<<ENC_STATE_BITS) - 1)

typedef enum {
    ENC_STATE_NORM = 0,
    ENC_STATE_CAROUSEL,
    ENC_STATE_TUNE_NORM,
    ENC_STATE_TUNE_CAROUSEL
} EncoderStates;

// Tuning steps
const ulong tune_steps[T_STEP_MAX_STEPS] =
{
		T_STEP_1HZ,
		T_STEP_5HZ,
		T_STEP_10HZ,
		T_STEP_50HZ,
		T_STEP_100HZ,
		T_STEP_500HZ,
		T_STEP_1KHZ,
		T_STEP_5KHZ,
		T_STEP_9KHZ,
		T_STEP_10KHZ,
		T_STEP_100KHZ,
		T_STEP_1MHZ,
		T_STEP_10MHZ
};


// The following are calibrations for the S-meter based on 6 dB per S-unit, 10 dB per 10 dB mark above S-9
// The numbers within are linear gain values, not logarithmic, starting with a zero signal level of 1
// There are 33 entries, one corresponding with each point on the S-meter
#define	S_Meter_Cal_Size	33	// number of entries in table below
const float S_Meter_Cal[S_Meter_Cal_Size] =
{
		14.1,	    //1.41,	        //1, S0.5, 3dB
		20,		    //2,		    //2, S1, 6dB
		28.1,	    //2.81,	        //3, S1.5, 9dB
		30,		    //3,		    //4, S2, 12dB
		56.2,	    //5.62,	        //5, S2.5, 15dB
		79.4,	    //7.94,	        //6, S3, 18dB
		112.2,	    //11.22,	    //7, S3.5, 21dB
		158.5,	    //15.85,	    //8, S4, 24dB
		223.9,	    //22.39,	    //9, S4.5, 27dB
		316.3,	    //31.63,	    //10, S5, 30dB
		446.7,	    //44.67,	    //11, S5.5, 33dB
		631,	    //63.10,	    //12, S6, 36dB
		891.3,	    //89.13,	    //13, S6.5, 39dB
		1258.9,	    //125.89,	    //14, S7, 42dB
		1778.3,	    //177.83,	    //15, S7.5, 45dB
		2511.9,	    //251.19,	    //16, S8, 48dB
		3548.1,	    //354.81,	    //17, S8.5, 51dB
		5011.9,	    //501.19,	    //18, S9, 54dB
		8912.5,	    //891.25,	    //19, +5, 59dB
		15848.9,	//1584.89,	    //20, +10, 64dB
		28183.8,	//2818.38,	    //21, +15, 69dB
		50118.7,	//5011.87,	    //22, +20, 74dB
		89125.1,	//8912.51,	    //23, +25, 79dB
		158489.3,	//15848.93,	    //24, +30, 84dB
		281838.2,	//28183.82,	    //25, +35, 89dB
		501187.2,	//50118.72,	    //26, +40, 94dB
		891250.9,	//89125.09,	    //27, +45, 99dB
		1585893.2,	//158489.32,	//28, +50, 104dB
		2818382.9,	//281838.29,	//29, +55, 109dB
		5011872.3,	//501187.23,	//30, +60, 114dB
		8912509.4,	//891250.94,	//31, +65, 119dB
		15848931.9,	//1584893.19,	//32, +70, 124dB
		28183829.3,	//2818382.93	//33, +75, 129dB
};

// if S-meter is based on FFT / dBm calculation
const float S_Meter_Cal_dbm[S_Meter_Cal_Size] =
{
		// dBm vs. S-value
		-124.0,	// S0.5
		-121.0,	// S1
		-118.0,	// S1.5
		-115.0,	// S2
		-112.0,	// S2.5
		-109.0,	// S3
		-106.0,	// S3.5
		-103.0,	// S4
		-100.0,	// S4.5
		-97.0,	// S5
		-94.0,	// S5.5
		-91.0,	// S6
		-88.0,	// S6.5
		-85.0,	// S7
		-82.0,	// S7.5
		-79.0,	// S8
		-76.0,	// S8.5
		-73.0,	// S9
		-68.0,	// S9+5
		-63.0,	// +10
		-58.0,	// +15
		-53.0,	// +20
		-48.0,	// +25
		-43.0,	// +30
		-38.0,	// +35
		-33.0,	// +40
		-28.0,	// +45
		-23.0,	// +50
		-18.0,	// +55
		-13.0,	// +60
		-8.0,	// +65
		-3.0,	// +70
		2.0,    // +75
};

BandRegs vfo[VFO_MAX];

// ------------------------------------------------
// Keypad state
__IO KeypadState				ks;

// ------------------------------------------------
// Power supply meter
PowerMeter					pwmt;


// ------------------------------------------------

uchar drv_state = 0;
ui_driver_mode_t ui_driver_state = { .dmod_mode = 255, .digital_mode = 255 }; // initialize so that at startup we detect change
bool filter_path_change = false;

// check if touched point is within rectangle of valid action
bool UiDriver_CheckTouchRegion(const UiArea_t* tr_p)
{
	return ((ts.tp->hr_x <= (tr_p->x+tr_p->w)) &&
				(ts.tp->hr_x >= (tr_p->x)) &&
				(ts.tp->hr_y <= (tr_p->y+tr_p->h))) &&
				(ts.tp->hr_y >= (tr_p->y));

}


int32_t change_and_limit_int(volatile int32_t val, int32_t change, int32_t min, int32_t max)
{
	val +=change;
	if (val< min)
	{
		val = min;
	}
	else if (val>  max)
	{
		val = max;
	}
	return val;
}


uint32_t change_and_limit_uint(volatile uint32_t val, int32_t change, uint32_t min, uint32_t max)
{
	if (change < 0 && ( -change  > (val - min)))
	{
		val = min;
	}
	else if (change > 0 && change >  max - val)
	{
		val = max;
	}
	else
	{
		val +=change;
	}
	return val;
}

uint32_t change_and_wrap_uint(volatile uint32_t val, int32_t change, uint32_t min, uint32_t max)
{
	if (change  > ((int32_t)max - (int32_t)val))
	{
		val = min;
	}
	else if ((change + (int32_t)val) <  (int32_t)min)
	{
		val = max;
	}
	else
	{
		val +=change;
	}
	return val;
}

void incr_wrap_uint8(volatile uint8_t* ptr, uint8_t min, uint8_t max )
{
	*ptr = (change_and_wrap_uint(*ptr,+1,min,max))&0xff;
}

void incr_wrap_uint16(volatile uint16_t* ptr, uint16_t min, uint16_t max )
{
	*ptr = (change_and_wrap_uint(*ptr,+1,min,max))&0xff;
}

void decr_wrap_uint8(volatile uint8_t* ptr, uint8_t min, uint8_t max )
{
	*ptr = (change_and_wrap_uint(*ptr,-1,min,max))&0xff;
}

void decr_wrap_uint16(volatile uint16_t* ptr, uint16_t min, uint16_t max )
{
	*ptr = (change_and_wrap_uint(*ptr,-1,min,max))&0xff;
}

bool is_touchscreen_pressed()
{
	return (ts.tp->state == TP_DATASETS_VALID);	// touchscreen data available
}

bool is_vfo_b()
{
	return (ts.vfo_mem_mode & VFO_MEM_MODE_VFO_B) != 0;
}

vfo_name_t get_active_vfo()
{
    return is_vfo_b()?VFO_B:VFO_A;
}
// FIXME: The DSP stuff should go in a separate file
/**
 *
 * @return true if the noise blanker is enabled (if if turned "off)
 */
bool is_dsp_nb()
{
	return (ts.dsp.active & DSP_NB_ENABLE) != 0;
//	return (ts.nb_setting > 0); // noise blanker ON
}

/**
 *
 * @return true if the noise blanker settings indicate it should process things
 */
bool is_dsp_nb_active()
{
    return is_dsp_nb() && (ts.dsp.nb_setting > 0);
}

bool is_dsp_nr()
{
	return (ts.dsp.active & DSP_NR_ENABLE) != 0;
}

bool is_dsp_anr()
{
    return (ts.dsp.active & DSP_ANR_ENABLE) != 0;
}

bool is_dsp_nr_postagc()
{
	return (ts.dsp.active & DSP_NR_POSTAGC_ENABLE) != 0;
}

bool is_dsp_notch()
{
	return (ts.dsp.active & DSP_NOTCH_ENABLE) != 0;
}

bool is_dsp_mnotch()
{
	return (ts.dsp.active & DSP_MNOTCH_ENABLE) != 0;
}

bool is_dsp_mpeak()
{
	return (ts.dsp.active & DSP_MPEAK_ENABLE) != 0;
}

#define KEYACTION_NOP    (NULL)				// This action for the pressed key is treated as being executed, but it is a no-operation
#define KEYACTION_PASS ((void(*)())-1)		// This action for the pressed key is treated as not present, i.e. we do not report the key event has been processed

typedef struct
{
	uint32_t key_code;
	// use KEYACTION_NONE and KEYACTION_PASS to handled nop and pass for further processing,
	// see comments for these constants
	void (*press_func)(); // executed if short press of key is detected
	void (*hold_func)();  // executed if press and hold of key is detected
} keyaction_descr_t;

typedef struct
{
	const keyaction_descr_t* actions;
	int32_t size;
} keyaction_list_descr_t;


/*
 * @brief find the matching region in a list of region and associated function
 * @returns: true, if a match for the touch coordinates region was found.
 */
bool UiDriver_ProcessTouchActions(const touchaction_list_descr_t* tld, bool is_long_press)
{
	bool retval = false;
	if (tld != NULL)
	{
		for (uint32_t idx = 0; idx < tld->size; idx++)
		{
			if (UiDriver_CheckTouchRegion(&tld->actions[idx].region))
			{
			    if (is_long_press)
			    {
			        if (tld->actions[idx].function_long_press != NULL)
			        {
			            (*tld->actions[idx].function_long_press)();
			        }
			    }
			    else
			    {
			        if (tld->actions[idx].function_short_press != NULL)
			        {
			            (*tld->actions[idx].function_short_press)();
			        }
			    }
			    retval = true;
			    break;
			}
		}
	}
	return retval;
}

/*
 * @brief find the matching keycode in a list of keycodes and associated functions
 * @returns: true, if a match for the keycode was found and a function (which can be a "no operation") has been executed.
 */

bool UiDriver_ProcessKeyActions(const keyaction_list_descr_t* kld)
{
	bool retval = false;
	if (kld != NULL)
	{
		for (uint32_t idx = 0; idx < kld->size; idx++)
		{
			if (kld->actions[idx].key_code == ks.button_id)
			{
				void (*func_ptr)() =  ks.press_hold == true ? kld->actions[idx].hold_func:kld->actions[idx].press_func;
				if (func_ptr != KEYACTION_NOP && func_ptr != KEYACTION_PASS)
				{
					(*func_ptr)();
				}
				retval = func_ptr != KEYACTION_PASS;
				break;
			}
		}
	}

	return retval;
}

static void UiDriver_DisplayMessageStart(void)
{
    UiSpectrum_Clear();
}

static void UiDriver_DisplayMessageStop(void)
{
    if(ts.menu_mode)
    {
        UiMenu_RenderMenu(MENU_RENDER_ONLY);    // update menu display, was destroyed by message
    }
    else
    {
        UiSpectrum_Init();          // not in menu mode, redraw spectrum scope
    }
}


/**
 * @brief restarts lcd blanking timer, called in all functions which detect user interaction with the device
 */
void UiDriver_LcdBlankingStartTimer()
{
	if(ts.lcd_backlight_blanking & LCD_BLANKING_ENABLE)     // is LCD blanking enabled?
	{
		uint32_t ltemp = (ulong)(ts.lcd_backlight_blanking & LCD_BLANKING_TIMEMASK);      // get setting of LCD blanking timing
		ltemp *= 100;       // multiply to convert to deciseconds
		ts.lcd_blanking_time = ltemp + ts.sysclock;     // calculate future time at which LCD is to be turned off
		ts.lcd_blanking_flag = false;       // clear flag to make LCD turn on
	}
}

static void   UiDriver_LcdBlankingProcessTimer(void)
{
	// Process LCD auto-blanking
	if(ts.lcd_backlight_blanking & LCD_BLANKING_ENABLE)      // is LCD auto-blanking enabled?
	{
		if(ts.sysclock > ts.lcd_blanking_time)      // has the time expired and the LCD should be blanked?
		{
			ts.lcd_blanking_flag = true;             // yes - blank the LCD
		}
		else                                        // time not expired
		{
			ts.lcd_blanking_flag = false;             // un-blank the LCD
		}
	}
	else                                  // auto-blanking NOT enabled
	{
		ts.lcd_blanking_flag = false;               // always un-blank the LCD in this case
	}
}

static char ui_txt_msg_buffer[MSG_BUFFER_SIZE];

static int ui_txt_msg_idx= 0;
static bool ui_txt_msg_update = false;

void UiDriver_RIT_Reset() {
	ts.rit_value = 0;
	UiDriver_UpdateDisplayAfterParamChange();
}

void UiDriver_TextMsgClear() {
	uint32_t fillcnt;

	for (fillcnt=0; fillcnt < ts.Layout->TextMsg_buffer_max; fillcnt++) {
		ui_txt_msg_buffer[fillcnt]=' ';
	}

	ui_txt_msg_buffer[fillcnt]='\0';

    UiLcdHy28_PrintText(
        ts.Layout->TextMsgLine.x,
        ts.Layout->TextMsgLine.y,
        ui_txt_msg_buffer,
        sd.txt_colour, Black,
        ts.Layout->TextMsg_font
    );

    ui_txt_msg_idx = 0;
    ui_txt_msg_update = true;
}

void UiDriver_TextMsgDisplay() {
    if (ui_txt_msg_update == true) {
        ui_txt_msg_update = false;

        if (ui_txt_msg_idx==0) {
        	uint32_t fillcnt;

        	for (fillcnt=0; fillcnt < ts.Layout->TextMsg_buffer_max; fillcnt++) {
        		ui_txt_msg_buffer[fillcnt]=' ';
        	}
        	ui_txt_msg_buffer[fillcnt]='\0';
        }

        UiLcdHy28_PrintText(
            ts.Layout->TextMsgLine.x,
            ts.Layout->TextMsgLine.y,
            ui_txt_msg_buffer,
            sd.txt_colour, Black,
            ts.Layout->TextMsg_font
        );
    }
}

void UiDriver_TextMsgPutChar(char ch)
{
    if (ch=='\n' || ch == '\r')
    {
        ui_txt_msg_idx=0;
    	ui_txt_msg_buffer[ui_txt_msg_idx] = '\0';
    }
    else if (ui_txt_msg_idx < (ts.Layout->TextMsg_buffer_max))
    {
        ui_txt_msg_idx++;
    	ui_txt_msg_buffer[ui_txt_msg_idx] = '\0'; // set the line end before we add the character prevents unterminated strings
        ui_txt_msg_buffer[ui_txt_msg_idx-1]=ch; //fill from left to right
    }
    else
    {
        for (int shift_count = 0;shift_count < (ts.Layout->TextMsg_buffer_max-1);shift_count++)
        {
            ui_txt_msg_buffer[shift_count]=ui_txt_msg_buffer[shift_count+1];
        }
        ui_txt_msg_buffer[ts.Layout->TextMsg_buffer_max-1]=ch;
    }
    ui_txt_msg_update = true;
}

void UiDriver_TextMsgPutSign(const char *s)
{
	UiDriver_TextMsgPutChar('<');
	UiDriver_TextMsgPutChar(s[0]);
	UiDriver_TextMsgPutChar(s[1]);
	UiDriver_TextMsgPutChar('>');
}

static void UiDriver_LcdBlankingStealthSwitch(void)
{
	if(ts.lcd_backlight_blanking & LCD_BLANKING_ENABLE)
	{         // Yes - is MSB set, indicating "stealth" (backlight timed-off) mode?
		ts.lcd_backlight_blanking &= ~LCD_BLANKING_ENABLE;
	} // yes - clear that bit, turning off "stealth" mode
	else
	{
		if(ts.lcd_backlight_blanking & LCD_BLANKING_TIMEMASK)    // bit NOT set AND the timing set to NON-zero?
		{
			ts.lcd_backlight_blanking |= LCD_BLANKING_ENABLE;       // no - turn on MSB to activate "stealth" mode
		}
	}
}

void UiDriver_DisplayFilter() {
    const char *filter_names[2];
    uint32_t   color = sd.txt_colour;
    uint32_t   bg_color = Black;

    AudioFilter_GetNamesOfFilterPath(ts.filter_path, filter_names);

    if (filter_names[1] == NULL)
        filter_names[1] = " ";

    if (filter_path_change) {
        UiLcdHy28_DrawFullRect(
            ts.Layout->FILTER_IND.x,
            ts.Layout->FILTER_IND.y,
            ts.Layout->FILTER_IND.h,
            ts.Layout->FILTER_IND.w,
            sd.txt_colour
        );

        color = Black;
        bg_color = sd.txt_colour;
    } else {
        UiLcdHy28_DrawFullRect(
            ts.Layout->FILTER_IND.x + 1,
            ts.Layout->FILTER_IND.y + 1,
            ts.Layout->FILTER_IND.h - 1,
            ts.Layout->FILTER_IND.w - 1,
            Black
        );

        UiLcdHy28_DrawEmptyRect(
            ts.Layout->FILTER_IND.x,
            ts.Layout->FILTER_IND.y,
            ts.Layout->FILTER_IND.h,
            ts.Layout->FILTER_IND.w,
            sd.boxes_colour
        );
    }

    for (int i = 0; i < 2; i++)
        UiLcdHy28_PrintTextCentered(
                ts.Layout->FILTER_IND.x + 1,
                ts.Layout->FILTER_IND.y + 2 + i * 14,
                ts.Layout->FILTER_IND.w - 2,
                filter_names[i],
                color, bg_color,
                0
        );
}

// TODO: most of this belongs to radio management, not UI
static void UiDriver_ToggleDigitalMode(void)
{
	if (ts.digital_mode != DigitalMode_None)
	{
		// a valid digital mode is set but may not be active yet
		if (ts.dmod_mode != DEMOD_DIGI)
		{
			if (RadioManagement_IsApplicableDemodMode(DEMOD_DIGI))
			{
				// this will switch to the corresponding sideband if we come from
				// SSB, otherwise the automatically selected default (AUTO LSB/USB ON) or the previously used
				// will be the selected one.
				if (is_ssb(ts.dmod_mode))
				{
					ts.digi_lsb = RadioManagement_LSBActive(ts.dmod_mode);
				}
				RadioManagement_SetDemodMode(DEMOD_DIGI);
			}
		}
	}
	else
	{
		if (ts.dmod_mode == DEMOD_DIGI)
		{
			// we are in digital mode but the current digital mode is in fact
			// None, i.e. we are going analog now
			RadioManagement_SetDemodMode(ts.digi_lsb?DEMOD_LSB:DEMOD_USB);
		}
	}
	UiDriver_UpdateDisplayAfterParamChange();
}

/*
 * @brief Function will update LO and Display Digits, it will never change LO if not necessary
 *
 * @param full_update set to true in order to have the full display digits being updated
 *
 */
void UiDriver_FrequencyUpdateLOandDisplay(bool full_update) {
#ifdef USE_DISP_480_320
    UiDriver_UpdateFrequency(false, UFM_LARGE_A);
    UiDriver_UpdateFrequency(false, UFM_LARGE_B);
#else
	if (is_splitmode()) {
		UiDriver_UpdateFrequency(false, UFM_SMALL_TX);
		UiDriver_UpdateFrequency(false, UFM_SMALL_RX);

	} else {
		UiDriver_UpdateFrequency(false, UFM_AUTOMATIC);
	}
#endif
}

void UiDriver_DebugInfo_DisplayEnable(bool enable) {
	UiLcdHy28_PrintText(ts.Layout->DEBUG_X,ts.Layout->LOADANDDEBUG_Y,enable?"Debug->":"       ",Green,Black,0);

	if (enable == false) {
		UiLcdHy28_PrintText(ts.Layout->LOAD_X,ts.Layout->LOADANDDEBUG_Y,"     ",White,Black,0);
	}

	ts.show_debug_info = enable;
}

void UiDriver_SpectrumChangeLayoutParameters() {
	UiSpectrum_WaterfallClearData();
	AudioDriver_SetProcessingChain(ts.dmod_mode, false);

    ts.iq_freq_delta = 0;

	if (ts.menu_mode == false) {
		UiSpectrum_Init();      // init spectrum scope
	}
}

/**
 * Sets a power level and updates the display accordingly.
 * @param power_level
 */
void UiDriver_HandlePowerLevelChange(const BandInfo* band, uint8_t power_level) {
	if (RadioManagement_SetPowerLevel(band,power_level)) {
		UiDriver_DisplayPowerLevel();

		if (ts.menu_mode) {
			UiMenu_RenderMenu(MENU_RENDER_ONLY);
		}
	}
}

void UiDriver_RefreshPowerLevel(const BandInfo* band, uint8_t power_level) {
	UiDriver_HandlePowerLevelChange(band, power_level);
}

void UiDriver_HandleBandButtons(uint16_t button) {

	static const bool BAND_DOWN = false;
	static const bool BAND_UP = true;

	bool buttondirSwap = (ts.flags1 & FLAGS1_SWAP_BAND_BTN)?true:false;

	bool dir;

	if (button == BUTTON_BNDM) {
		dir = buttondirSwap ? BAND_UP : BAND_DOWN;
	} else {
		dir = buttondirSwap ? BAND_DOWN : BAND_UP;
	}

	UiDriver_ChangeBand(dir);
}


static void UiDriver_PublicsInit(void) {
	// Button state structure init state
	ks.button_id			= BUTTON_NONE;
	ks.button_pressed		= 0;
	ks.button_released		= 0;
	ks.button_processed		= 0;
	ks.debounce_time		= 0;


	// Auto button blink state
	//abst.blink_flag 		= 0;
	//abst.blink_skip 		= 0;

	// SWR meter init
	swrm.p_curr				= 0;
	swrm.fwd_calc			= 0;
	swrm.rev_calc			= 0;
	swrm.fwd_pwr			= 0;
	swrm.rev_pwr			= 0;
	swrm.fwd_dbm			= 0;
	swrm.rev_dbm			= 0;
	swrm.vswr			 	= 0;
	swrm.sensor_null		= SENSOR_NULL_DEFAULT;

	for (int idx = 0; idx < COUPLING_MAX; idx++)
	    swrm.coupling_calc[idx] = SWR_COUPLING_DEFAULT;

	swrm.pwr_meter_disp		= 0;	// Display of numerical FWD/REV power metering off by default
	swrm.pwr_meter_was_disp = 0;	// Used to indicate if FWD/REV numerical power metering WAS displayed

	// Power supply meter
	pwmt.p_curr				= 0;
	pwmt.pwr_aver 			= 0;
	pwmt.undervoltage_detected = false;

}

static void UiDriver_DspModeMaskInit(void)
{
    if(mchf_touchscreen.present)
    {
        //preventing DSP functions mask to have not proper value
        if (ts.dsp.mode_mask == 0 || ts.dsp.mode_mask == 1)
        {
            // empty mask is invalid, set it to all entries enabled
            ts.dsp.mode_mask = DSP_SWITCH_MODEMASK_ENABLE_DEFAULT;
        }
        else
        {
            // just make sure DSP OFF is always on the list
            ts.dsp.mode_mask|=DSP_SWITCH_MODEMASK_ENABLE_DSPOFF;
        }

        ts.dsp.mode_mask&=DSP_SWITCH_MODEMASK_ENABLE_MASK;
    }
    else
    {
        ts.dsp.mode_mask=DSP_SWITCH_MODEMASK_ENABLE_DEFAULT;        //disable masking when no touchscreen controller detected
    }
}


void UiDriver_Init()
{
	// Driver publics init
	UiDriver_PublicsInit();
	// Init frequency publics
	UiDriver_InitFrequency();

	Keypad_Scan();

	// Load stored data from eeprom or calibrate touchscreen
	bool run_keytest = (UiDriver_LoadSavedConfigurationAtStartup() == false && UiDriver_TouchscreenCalibration() == false);

	// set the encoders to their default values

	memset((void *) &ts.enc_state, 0, sizeof(ts.enc_state));

	uint8_t enc_default = 0;

	for (uint8_t enc = 0; enc < 3; enc++)
	    for (uint8_t state = 0; state < ENC_STATE_NUM; state++)
	        if (ts.enc_mode[enc][state] > ENC_NUM_MODES-1) {
	            enc_default = 1;
	            break;
	        }

    if (enc_default) {
        ts.enc_mode[ENC1][0] = ENC_MODE_AUDIO_GAIN;
        ts.enc_mode[ENC1][1] = ENC_MODE_RTTY_SPEED;
        ts.enc_mode[ENC1][2] = ENC_MODE_ST_GAIN;
        ts.enc_mode[ENC1][3] = ENC_MODE_PWR_GAIN;
        ts.enc_mode[ENC1][4] = ENC_MODE_CMP_LEVEL;

        ts.enc_mode[ENC2][0] = ENC_MODE_RF_GAIN;
        ts.enc_mode[ENC2][1] = ENC_MODE_RTTY_SHIFT;
        ts.enc_mode[ENC2][2] = ENC_MODE_BASS_GAIN;
        ts.enc_mode[ENC2][3] = ENC_MODE_NR;
        ts.enc_mode[ENC2][4] = ENC_MODE_NB;

        ts.enc_mode[ENC3][0] = ENC_MODE_RIT;
        ts.enc_mode[ENC3][1] = ENC_MODE_CW_SPEED;
        ts.enc_mode[ENC3][2] = ENC_MODE_PSK_SPEED;
        ts.enc_mode[ENC3][3] = ENC_MODE_TREBLE_GAIN;
        ts.enc_mode[ENC3][4] = ENC_MODE_INPUT_GAIN;
    }

	if (mchf_touchscreen.present) {
		//Touchscreen calibration test.
		//We cannot distinguish when touchscreen is uncalibrated with other method than comparing calibration values to empty state of EEPROM (0xff).
	    //It would be nice if someone has better idea how to do it without digging into calibration matrix computation to describe the allowed range of coefficients. Feb 2018, SP9BSL.
		bool IS_TSCalibrated=0;

		for(int16_t m=0; m<6; m++) {
			IS_TSCalibrated|=ts.tp->cal[m]!=0xffffffff;
		}

		UiDriver_StartupScreen_LogIfProblem(IS_TSCalibrated == 0,
				"WARNING:  TOUCHSCREEN NOT CALIBRATED!!!\nRun calibration first!");
	}

	if (ts.special_functions_enabled != 1) {
	  UiDriver_StartupScreen_LogIfProblem(ts.iq_freq_mode == FREQ_IQ_CONV_MODE_OFF,
			"WARNING:  Freq. Translation is OFF!!!\nTranslation is STRONGLY recommended!!");
	}

	// now run all inits which need to be done BEFORE going into test screen mode
	uint8_t mirtemp;

	if(ts.flags1 & FLAGS1_REVERSE_X_TOUCHSCREEN) {
		mirtemp = 1;
	} else {
		mirtemp = 0;
	}

	if(ts.flags1 & FLAGS1_REVERSE_Y_TOUCHSCREEN) {
		mirtemp += 2;
	}

	UiLcdHy28_TouchscreenInit(mirtemp);

	if (run_keytest) {
		UiDriver_KeyTestScreen();
	}

	osc->setPPM((float)ts.freq_cal/10.0);

	df.tune_new = vfo[get_active_vfo()].band[ts.band->band_mode].dial_value;		// init "tuning dial" frequency based on restored settings
	df.tune_old = 0; // with this we force a frequency change once the main loop becomes active

	ts.cw_lsb = RadioManagement_CalculateCWSidebandMode();			// determine CW sideband mode from the restored frequency

	UiDriver_DspModeMaskInit();

	sd.display_offset = INIT_SPEC_AGC_LEVEL;		// initialize setting for display offset/AGC

	// Reset inter driver requests flag
	ts.LcdRefreshReq	= 0;
	ts.new_band 		= ts.band->band_mode;
	df.step_new 		= df.tuning_step;

	// Extra HW init
	Board_PostInit();

	UiDriver_LcdBlankingStartTimer();			// init timing for LCD blanking
	ts.lcd_blanking_time = ts.sysclock + LCD_STARTUP_BLANKING_TIME;
	ts.low_power_shutdown_time = ts.sysclock + LOW_POWER_SHUTDOWN_DELAY_TIME;
	ts.cw_keyer_speed_bak = ts.cw_keyer_speed;
}

#define BOTTOM_BAR_LABEL_W (56)
#define POS_BOTTOM_BAR_F1_offset 2

void UiDriver_DrawFButtonLabel(uint8_t button_num, const char* label, uint32_t label_color) {
    UiLcdHy28_PrintTextCentered(
        ts.Layout->BOTTOM_BAR.x + POS_BOTTOM_BAR_F1_offset + (button_num - 1) * (ts.Layout->BOTTOM_BAR.w + 2),
        ts.Layout->BOTTOM_BAR.y - 2,
        BOTTOM_BAR_LABEL_W,
        label,
        label_color, Black,
        0
    );
}

void UiDriver_EncoderDisplay(const uint8_t row, const uint8_t column, const char *label, uint8_t style,
		const char temp[5], uint32_t color)
{

	uint32_t label_color, bg_color, brdr_color;

	switch (style) {
	    case ENC_STATE_NORM:
	        label_color = Black;
	        bg_color = sd.boxes_colour;
	        brdr_color = sd.boxes_colour;
	        break;

	    case ENC_STATE_CAROUSEL:
            label_color = sd.txt_colour;
            color = sd.txt_colour_dark;
            bg_color = sd.boxes_colour;
            brdr_color = sd.boxes_colour;
            break;

	    case ENC_STATE_TUNE_NORM:
        case ENC_STATE_TUNE_CAROUSEL:
            label_color = Black;
            color = sd.txt_colour_dark;
            bg_color = sd.txt_colour;
            brdr_color = sd.txt_colour;
            break;

	    default:
            label_color = Grey1;
            color = Grey;
            bg_color = Grey;
            brdr_color = Grey;
            break;
	}

	if (ts.Layout->ENCODER_MODE == MODE_HORIZONTAL) {
		UiLcdHy28_DrawEmptyRect(
		    ts.Layout->ENCODER_IND.x + ENC_COL_W * 2 * column + row * ENC_COL_W + column * Xspacing,
		    ts.Layout->ENCODER_IND.y ,
		    ENC_ROW_H - 2,
		    ENC_COL_W - 2, brdr_color
		);

		UiLcdHy28_PrintTextCentered(
		    ts.Layout->ENCODER_IND.x + 1 + ENC_COL_W * 2 * column + row * ENC_COL_W + column * Xspacing,
		    ts.Layout->ENCODER_IND.y + 1,
		    ENC_COL_W - 3,
		    label,
		    label_color, bg_color, 0
		);

		UiLcdHy28_PrintTextRight(
		    ts.Layout->ENCODER_IND.x + ENC_COL_W - 4 + ENC_COL_W * 2 * column + row * ENC_COL_W + column * Xspacing,
		    ts.Layout->ENCODER_IND.y + 1 + ENC_ROW_2ND_OFF,
		    temp,
		    color, Black, 0
		);
	} else {
        uint8_t ENC_ROW_H_WS        = 28;
        uint8_t ENC_ROW_2ND_OFF_WS  = 14;

        UiLcdHy28_DrawEmptyRect(
            ts.Layout->ENCODER_IND.x + ENC_COL_W * column,
            ts.Layout->ENCODER_IND.y + row * ENC_ROW_H_WS,
            ENC_ROW_H_WS,
            ENC_COL_W - 2,
            brdr_color
        );

        UiLcdHy28_PrintTextCentered(
            ts.Layout->ENCODER_IND.x + 1 + ENC_COL_W * column,
            ts.Layout->ENCODER_IND.y + row * ENC_ROW_H_WS,
            ENC_COL_W - 3,
            label,
            label_color, bg_color, 0
        );

        UiLcdHy28_PrintTextRight(
            ts.Layout->ENCODER_IND.x + ENC_COL_W - 4 + ENC_COL_W * column,
            ts.Layout->ENCODER_IND.y + row * ENC_ROW_H_WS + ENC_ROW_2ND_OFF_WS,
            temp,
            color, Black, 0
        );
    }
}

uint8_t UiDriver_GetModeCode()
{
    uint8_t mode;

    switch(ts.dmod_mode)
    {
    case DEMOD_USB: mode = CODE_SSB; break;
    case DEMOD_LSB: mode = CODE_SSB; break;
    case DEMOD_SAM: mode = CODE_SAM; break;
    case DEMOD_AM:  mode = CODE_AM; break;
    case DEMOD_FM:  mode = CODE_FM; break;
    case DEMOD_CW:  mode = CODE_CW; break;
    case DEMOD_DIGI:
        switch(ts.digital_mode)
        {
        case DigitalMode_RTTY:   mode = CODE_RTTY; break;
        case DigitalMode_BPSK:   mode = CODE_BPSK; break;
#ifdef USE_FREEDV
        case DigitalMode_FreeDV: mode = CODE_FREEDV; break;
#endif
        }
        break;
#ifdef USE_TWO_CHANNEL_AUDIO
    case DEMOD_IQ:        mode = CODE_IQ; break;
    case DEMOD_SSBSTEREO: mode = CODE_SSB_STEREO; break;
#endif
        default:
            break;
    }
    return mode;
}

void UiDriver_StoreBwData()
{
    uint8_t band = RadioManagement_GetBand(df.tune_new)->band_mode;
    uint8_t mode = UiDriver_GetModeCode();
    bw_data[mode][band] = ts.filter_path;
}

static void UiDriver_LoadBwData()
{
    uint8_t band = RadioManagement_GetBand(df.tune_new)->band_mode;
    uint8_t mode = UiDriver_GetModeCode();
    if(bw_data[mode][band]) // not empty
    {
    	ts.filter_path = bw_data[mode][band];
//    	UiDriver_UpdateDisplayAfterParamChange();
    }
}

static void UiDriver_StoreMemData(uint8_t mem_cell)
{
    uint8_t band = RadioManagement_GetBand(df.tune_new)->band_mode;
    uint8_t mode = 0;
    uint8_t sideband = 0;
    mem_cell--; // in argument using cell numder 1...4 for comfort ))

    switch(ts.dmod_mode)
    {
    case DEMOD_USB:
        mode = 0; sideband = 1;//"SSB-U"
        break;
    case DEMOD_LSB:
        mode = 0; sideband = 2;//"SSB-L"
        break;
    case DEMOD_SAM:
        if(ads.sam_sideband == SAM_SIDEBAND_LSB)
        {
            mode = 2; sideband = 2;//"SAM-L";
        }
        else if (ads.sam_sideband == SAM_SIDEBAND_USB)
        {
            mode = 2; sideband = 1;//"SAM-U";
        }
#ifdef USE_TWO_CHANNEL_AUDIO
        else if (ads.sam_sideband == SAM_SIDEBAND_STEREO)
        {
            mode = 2; sideband = 3;//"SAM-S";
        }
#endif
        else
        {
            mode = 2; sideband = 0;//"SAM";
        }
        break;
    case DEMOD_AM:
        mode = 1; sideband = 0;//"AM";
        break;
    case DEMOD_FM:
        mode = 4;//"FM"
        sideband = RadioManagement_FmDevIs5khz() ? 5 : 4; //"W"/"N"
        break;
    case DEMOD_CW:
        mode = 3;//"CW"
        sideband = ts.cw_lsb? 2: 1;//"L"/"U"
        break;
    case DEMOD_DIGI:
        sideband = ts.digi_lsb? 2: 1;//"L"/"U"
        switch(ts.digital_mode)
        {
        case DigitalMode_RTTY:
            mode = 5;//"RTTY"
            break;
        case DigitalMode_BPSK:
            mode = 6;//"BPSK"
            break;
#ifdef USE_FREEDV
        case DigitalMode_FreeDV:
            mode = 7;//"FreeDV"
            break;
#endif
        }
        break;
#ifdef USE_TWO_CHANNEL_AUDIO
    case DEMOD_IQ:
        mode = 8;//"IQ"
        sideband = 3;//"S";
        break;
    case DEMOD_SSBSTEREO:
        mode = 9;//"SSBSTEREO"
        sideband = 3;//"S";
        break;
#endif
        default:
            break;
    }
    mem_data[mem_cell][0][band] = df.tune_old;
    mem_data[mem_cell][1][band] = (ts.filter_path<<16)+(mode<<8)+sideband;
    if(ts.mem_disp)
    {
        UiAction_ShowMems();
    }
}

static void UiDriver_StoreMemData1(void)
{
    UiDriver_StoreMemData(1);
}

static void UiDriver_StoreMemData2(void)
{
    UiDriver_StoreMemData(2);
}

static void UiDriver_StoreMemData3(void)
{
    UiDriver_StoreMemData(3);
}

static void UiDriver_StoreMemData4(void)
{
    UiDriver_StoreMemData(4);
}

static void UiDriver_LoadMemData(uint8_t mem_cell)
{
    mem_cell--; // in argument using cell numder 1...4 for comfort ))
    uint8_t band = RadioManagement_GetBand(df.tune_old)->band_mode;
    ts.dvmode = false;
    uint32_t freq = mem_data[mem_cell][0][band];
    if((ts.txrx_mode!=TRX_MODE_RX)||(freq==0)||(band!=RadioManagement_GetBand(freq)->band_mode)||(freq<500000)||(freq>55000000))
    {
        return; // empty/crash mem cell or TUNE/TX
    }
    // frequency
    df.tune_new = mem_data[mem_cell][0][band];
    bool result = RadioManagement_ChangeFrequency(true, df.tune_new, TRX_MODE_RX);
    if(!result)
    {
        return; // Don't changed freq, alarm!!!
    }
    // mode & sideband
    uint8_t mode = (mem_data[mem_cell][1][band] & 0x0000FF00)>>8;
    uint8_t sideband = mem_data[mem_cell][1][band] & 0x000000FF;
    if(mode==0)
        {ts.dmod_mode = sideband==1?DEMOD_USB:DEMOD_LSB;}
    else if(mode==1)
        {ts.dmod_mode = DEMOD_AM;}
    else if(mode==2)
    {
        ts.dmod_mode = DEMOD_SAM;
        if(sideband == 2)
        {
            ads.sam_sideband = SAM_SIDEBAND_LSB;
        }
        else if (sideband ==1)
        {
            ads.sam_sideband = SAM_SIDEBAND_USB;
        }
#ifdef USE_TWO_CHANNEL_AUDIO
        else if (sideband == 3)
        {
            ads.sam_sideband = SAM_SIDEBAND_STEREO;
        }
#endif
        else if (sideband == 0)
        {
            ads.sam_sideband = SAM_SIDEBAND_BOTH;
        }
    }
    else if(mode==3)
    {
        ts.dmod_mode = DEMOD_CW;
        ts.cw_lsb = sideband==2?true:false;
    }
    else if(mode==4)
    {
        ts.dmod_mode = DEMOD_FM;
        sideband = RadioManagement_FmDevIs5khz() ? 5 : 4; //"W"/"N"
        if(sideband == 5) // "W"
        {
            ts.flags2 = ts.flags2 | FLAGS2_FM_MODE_DEVIATION_5KHZ;
        }
        else if(sideband == 4) // "N"
        {
            ts.flags2 = ts.flags2 & ~FLAGS2_FM_MODE_DEVIATION_5KHZ;
        }
    }
    else if(mode==5)
    {
        ts.dmod_mode = DEMOD_DIGI;
        ts.digi_lsb = sideband==2?true:false;
        ts.digital_mode = DigitalMode_RTTY;
        ts.dvmode = true;
    }
    else if(mode==6)
    {
        ts.dmod_mode = DEMOD_DIGI;
        ts.digi_lsb = sideband==2?true:false;
        ts.digital_mode = DigitalMode_BPSK;
        ts.dvmode = true;
    }
#ifdef USE_FREEDV
    else if(mode==7)
    {
        ts.dmod_mode = DEMOD_DIGI;
        ts.digi_lsb = sideband==2?true:false;
        ts.digital_mode = DigitalMode_FreeDV;
        ts.dvmode = true;
    }
#endif
#ifdef USE_TWO_CHANNEL_AUDIO
    else if(mode==8)
    {
        ts.dmod_mode = DEMOD_IQ;
        ts.stereo_enable = true;
    }
    else if(mode==9)
    {
        ts.dmod_mode = DEMOD_SSBSTEREO;
        ts.stereo_enable = true;
    }
#endif
    RadioManagement_SetDemodMode(ts.dmod_mode);
    // filter
    ts.filter_path = (mem_data[mem_cell][1][band]>>16);
    // final
    if(ts.mem_disp)
    {
        ts.mem_disp = false;
    }
    df.tuning_step  = tune_steps[df.selected_idx[UiDriver_GetModeCode()]];
    UiDriver_UpdateDisplayAfterParamChange();
    UiSpectrum_Init();
}

static void UiDriver_LoadMemData1(void)
{
    UiDriver_LoadMemData(1);
}

static void UiDriver_LoadMemData2(void)
{
    UiDriver_LoadMemData(2);
}

static void UiDriver_LoadMemData3(void)
{
    UiDriver_LoadMemData(3);
}

static void UiDriver_LoadMemData4(void)
{
    UiDriver_LoadMemData(4);
}

static void UiAction_ShowMems()
{
//  ts.mem_disp = true;
    uint8_t band = RadioManagement_GetBand(df.tune_old)->band_mode;
//    uint16_t freq = 0;
    uint32_t freq = 0;//df.tune_old;
    int8_t mode = 0;
    uint8_t sideband = 0;
    char txt_cell[2];
    char txt_freq[10] = "???";
    char* txt_mode = "???";
    char* txt_sideband = "?";
    int16_t filter = 0;
    const char *filter_names[2];
    uint8_t y_start = 146;
    uint8_t y_interval = 20;

    for(int mem_cell = 0; mem_cell<4; mem_cell++)
    {
        snprintf(txt_cell,4,"M%1u",mem_cell+1);
#ifndef SDR_AMBER_480_320
	#ifndef OVI40_MOD_480_320
        UiLcdHy28_PrintText(10,y_start+(mem_cell*y_interval), txt_cell,Yellow,Black,0);
	#else
        UiLcdHy28_PrintText(10,y_start+(mem_cell*y_interval), txt_cell,Yellow,Black,0);
	#endif
#else
        UiLcdHy28_PrintText(10,y_start+(mem_cell*y_interval), txt_cell,Yellow,Black,0);
#endif

        freq = mem_data[mem_cell][0][band];
        if ((freq != 0) && (band == RadioManagement_GetBand(freq)->band_mode) && (freq > 500000) && (freq < 55000000))
        {
            mode = (mem_data[mem_cell][1][band]>>8) & 0xFF;
            sideband = mem_data[mem_cell][1][band] & 0xFF;
            filter = (mem_data[mem_cell][1][band]>>16) & 0xFFFF;
            AudioFilter_GetNamesOfFilterPath(filter,filter_names);
            switch(mode)
            {
            case 0:
                //txt_mode = "   SSB";
                break;
            case 1:
                txt_mode = "    AM";
                break;
            case 2:
                txt_mode = "   SAM";
                break;
            case 3:
                txt_mode = "    CW";
                break;
            case 4:
                txt_mode = "    FM";
                break;
            case 5:
                txt_mode = "  RTTY";
                break;
            case 6:
                txt_mode = "  BPSK";
                break;
            case 7:
                txt_mode = "FreeDV";
                break;
#ifdef USE_TWO_CHANNEL_AUDIO
            case 8:
                txt_mode = "    IQ";
                break;
            case 9:
                txt_mode = " SSB-S";
                break;
#endif
            default:
                break;
            }

            switch(sideband)
            {
            case 0:
                txt_sideband = " ";
                break;
            case 1:
                if(mode == 0)
                {
                    txt_mode = "   USB";
                    txt_sideband = " ";
                }
                else
                {
                    txt_sideband = "U";
                }
                break;
            case 2:
                if(mode == 0)
                {
                    txt_mode = "   LSB";
                    txt_sideband = " ";
                }
                else
                {
                    txt_sideband = "L";
                }
                break;
                break;
            case 3:
                txt_sideband = "S";
                break;
            case 4:
                txt_sideband = "N";
                break;
            case 5:
                txt_sideband = "W";
                break;
            default:
                break;
            }

            sprintf(txt_freq,"%8ld",freq);
            uint8_t i = 0;
            uint8_t q = 0;
            char symb[2] = " ";
            for(i = 0; i<8; i++) // formatted output of freq
            {
                symb[0] = txt_freq[i];
#ifndef SDR_AMBER_480_320
	#ifndef OVI40_MOD_480_320
                if(i==2 || i==5)
                {
                    UiLcdHy28_PrintText(38+(q*8),y_start+(mem_cell*y_interval), ".",Yellow,Black,0);
                    q++;
                }
                UiLcdHy28_PrintText(38, y_start+(mem_cell*y_interval), symb,Yellow,Black,0);
                q++;
            }
//          UiLcdHy28_PrintText(38,y_start+(mem_cell*y_interval), txt_freq,Yellow,Black,0);
            UiLcdHy28_PrintText(128,y_start+(mem_cell*y_interval), txt_mode,Yellow,Black,0);
            UiLcdHy28_PrintText(188,y_start+(mem_cell*y_interval), txt_sideband,Yellow,Black,0);
	#else
            if(i==2 || i==5)
            {
                UiLcdHy28_PrintText(38+(q*8),y_start+(mem_cell*y_interval), ".",Yellow,Black,0);
                q++;
            }
            UiLcdHy28_PrintText(38+(q*8),y_start+(mem_cell*y_interval), symb,Yellow,Black,0);
            q++;
        }
//          UiLcdHy28_PrintText(38,y_start+(mem_cell*y_interval), txt_freq,Yellow,Black,0);
        UiLcdHy28_PrintText(128,y_start+(mem_cell*y_interval), txt_mode,Yellow,Black,0);
        UiLcdHy28_PrintText(188,y_start+(mem_cell*y_interval), txt_sideband,Yellow,Black,0);
	#endif
#else
            if(i==2 || i==5)
            {
                UiLcdHy28_PrintText(38+(q*8),y_start+(mem_cell*y_interval), ".",Yellow,Black,0);
                q++;
            }
            UiLcdHy28_PrintText(38+(q*8),y_start+(mem_cell*y_interval), symb,Yellow,Black,0);
            q++;
        }
//          UiLcdHy28_PrintText(38,y_start+(mem_cell*y_interval), txt_freq,Yellow,Black,0);
        UiLcdHy28_PrintText(128,y_start+(mem_cell*y_interval), txt_mode,Yellow,Black,0);
        UiLcdHy28_PrintText(188,y_start+(mem_cell*y_interval), txt_sideband,Yellow,Black,0);
#endif

            UiLcdHy28_PrintText(206+(strlen(filter_names[0])!=5?8:0),y_start+(mem_cell*y_interval), filter_names[0],Yellow,Black,0);
           UiLcdHy28_PrintText(262,y_start+(mem_cell*y_interval), filter_names[1],Yellow,Black,0);
        } else {
#ifndef SDR_AMBER_480_320
	#ifndef OVI40_MOD_480_320
            UiLcdHy28_PrintText(38, y_start+(mem_cell*y_interval), "empty",White,Black,0);
	#else
            UiLcdHy28_PrintText(38,y_start+(mem_cell*y_interval), "empty",White,Black,0);
	#endif
#else
            UiLcdHy28_PrintText(38,y_start+(mem_cell*y_interval), "empty",White,Black,0);
#endif
        }
    }
}

void UiDriver_DisplayFButton_F1MenuExit()
{
	char* cap;
	uint32_t color;
	if (ts.keyer_mode.active)
	{
		if (ts.keyer_mode.button_recording == KEYER_BUTTON_1)
		{
			cap = "REC";
			color = Red;
		}
		else
		{
			cap = (char*) ts.keyer_mode.cap[0];
			color = (ts.expflags1 & EXPFLAGS1_FAST_MACRO_1_2)? Yellow : White;
		}
	}
	else if(ts.ExtFMenuActive)
	{
		cap = "RIT 0";
		color = White;
	}
	else if(ts.XvtrFMenuActive)
	{
		cap = "XVTR 1";
		color = White;
	}
	else if(ts.MenuOfFMenusActive)
	{
		cap = "MACROS";
		color = Green;
	}
	else
	{
		if (!ts.menu_var_changed)
		{

			if (ts.menu_mode)
			{
				cap = "EXIT";
				color = Yellow;
			}
			else
			{
				cap = "MENU";
				color = White;
			}
		}
		else
		{
			cap = ts.menu_mode?"EXIT *":"MENU *";
			color = Orange;
		}
	}
	UiDriver_DrawFButtonLabel(1, cap, color);
}


static void UiDriver_DisplayFButton_F2SnapMeter(void)
{
	const char* cap;
	uint32_t color;
	if (ts.keyer_mode.active)
	{
		if (ts.keyer_mode.button_recording == KEYER_BUTTON_2)
		{
			cap = "REC";
			color = Red;
		}
		else
		{
			cap = (char *)ts.keyer_mode.cap[1];
            color = (ts.expflags1 & EXPFLAGS1_FAST_MACRO_1_2)? Yellow : White;
		}
	}
	else if(ts.ExtFMenuActive)
	{
		cap = "ZOOM-";
		color = White;
	}
	else if(ts.XvtrFMenuActive)
	{
		cap = "XVTR 2";
		color = White;
	}
    else if(ts.MenuOfFMenusActive)
    {
        cap = "EXT";
        color = Green;
    }
	else
	{

		cap = "SNAP";
		color = White;    // yes - indicate with color
//		color = White;
//		cap = "METER";
	}
	UiDriver_DrawFButtonLabel(2,cap,color);
}



static void UiDriver_FButton_F3MemSplit(void)
{
	const char* cap;
	uint32_t color;

	if (ts.keyer_mode.active)
	{
		if (ts.keyer_mode.button_recording == KEYER_BUTTON_3)
		{
			cap = "REC";
			color = Red;
		}
		else
		{
			cap = (char *)ts.keyer_mode.cap[2];
			color = White;
		}

	}
	else if(ts.ExtFMenuActive)
	{
		cap = "ZOOM+";
		color = White;
	}
	else if(ts.XvtrFMenuActive)
	{
		cap = "LST/OFF";
		color = Blue2;
	}
    else if(ts.MenuOfFMenusActive)
    {
        cap = "MEMS";
        color = Green;
    }
	else
	{
		if (ts.vfo_mem_flag)            // is it in VFO MEM mode now?
		{
			cap = "MEM";
//			color = White;    // yes - indicate with color
			color = Yellow;
			UiDriver_DrawFButtonLabel(1, "M1", White);
			UiDriver_DrawFButtonLabel(2, "M2", White);
			UiDriver_DrawFButtonLabel(4, "M3", White);
			UiDriver_DrawFButtonLabel(5, "M4", White);
		}
		else
		{
			color = is_splitmode() ?
					SPLIT_ACTIVE_COLOUR : SPLIT_INACTIVE_COLOUR;
			cap = "SPLIT";
			UiDriver_DisplayFButton_F1MenuExit();
			UiDriver_DisplayFButton_F2SnapMeter();
			UiDriver_FButton_F4ActiveVFO();
			UiDriver_FButton_F5Tune();
		}
	}
	UiDriver_DrawFButtonLabel(3, cap, color);
}


static inline void UiDriver_FButton_F4ActiveVFO()
{
	const char* cap;
	uint32_t color;
    if (ts.keyer_mode.active)
    {
        if (ts.keyer_mode.button_recording == KEYER_BUTTON_4)
        {
            cap = "REC";
            color = Red;
        }
        else
        {
            cap = (char *)ts.keyer_mode.cap[3];
            color = White;
        }

    }
	else if(ts.ExtFMenuActive)
	{
#ifdef SDR_AMBER
		cap = "RXIN-";
		color = White;
#else
		cap = "TXINP";
		color = White;
#endif
	}
    else if(ts.vfo_mem_flag)
    {
		cap = "M3";
		color = White;
    }
    else if(ts.XvtrFMenuActive)
    {
		cap = "XVTR 3";
		color = White;
    }
    else if(ts.MenuOfFMenusActive)
    {
        cap = "XVTR";
        color = Green;
    }
	else
	{
		cap = is_vfo_b() ? "VFO B" : "VFO A";
		color = White;
	}
	UiDriver_DrawFButtonLabel(4, cap, color);
}

static inline void UiDriver_FButton_F5Tune()
{
	const char* cap;
	uint32_t color = RadioManagement_IsTxDisabled() ? Grey1 : (ts.tune ? Red : White);

	if (ts.keyer_mode.active)
	{
		switch(ts.keyer_mode_tx)
		{
		case TXRX_U:
			cap = "TXRX U";
			break;
		case TXRX_B:
			cap = "TXRX B";
			break;
		case CYCLED:
			cap = "CYCLED";
			break;
		case BEACON:
			cap = "BEACON";
			break;
		}
		color = (ts.keyer_mode_tx == BEACON && ts.keyer_cycled_tx_active)? Red:  Blue2;
	}
	else if(ts.ExtFMenuActive)
	{
#ifdef SDR_AMBER
		cap = "RXIN+";
		color = White;
#else
		cap = "REST";
		color = White;
#endif
	}
    else if(ts.vfo_mem_flag)
    {
		cap = "M4";
		color = White;
    }
    else if(ts.XvtrFMenuActive)
    {
		cap = "XVTR 4";
		color = White;
    }
    else if(ts.MenuOfFMenusActive)
    {
        cap = "MAIN";
        color = Green;
    }
	else
	{
		cap = "TUNE";
	}
	UiDriver_DrawFButtonLabel(5, cap, color);
}

static inline void UiDriver_FButton_F6Illum()
{
#ifdef BOTTOM_BUTTON > 5
	const char* cap;
	uint32_t color = White;
	cap = "ILLUM";

	if (ts.expflags2 & EXPFLAGS2_AMBER_4INCH_F6_IND) {
		cap = "AT";
	} else {
		cap = "ILLUM";
	}
	UiDriver_DrawFButtonLabel(6, cap, color);
#endif
}

static inline void UiDriver_FButton_F7AT() {
#if BOTTOM_BUTTON > 6
	const char* cap;
	uint32_t color = White;

	if (ts.expflags2 & EXPFLAGS2_AMBER_4INCH_F7_IND) {
	    cap = "POWER";
	} else {
	    cap = "AT";
	}

	UiDriver_DrawFButtonLabel(7, cap, color);
#endif
}

void UiDriver_EncoderDisplaySimple(const uint8_t row, const uint8_t column, const char *label, uint8_t style, int32_t value) {
	char        temp[5];
	uint32_t    color;

	switch (style) {
	    case ENC_STATE_NORM:
        case ENC_STATE_TUNE_NORM:
            color = sd.txt_colour;
            break;

	    default:
	        color = sd.txt_colour_dark;
	        break;
	}

	snprintf(temp, sizeof(temp), "%4d", value);
	UiDriver_EncoderDisplay(row, column, label, style, temp, color);
}

void UiDriver_DisplayFreqLabels() {
    const char *txt = is_vfo_b() ? "B / A" : "A / B";

    UiLcdHy28_PrintTextCentered(
        ts.Layout->TUNE_SPLIT_MARKER_X,
        ts.Layout->TUNE_SPLIT_FREQ_Y_TX,
        20 * 8,
        txt,
        sd.txt_colour_dark, Black,
        0
    );
}

void UiDriver_DisplaySplitFreqLabels() {
#ifndef USE_DISP_480_320
	const char *split_rx, *split_tx;

	if (!(is_vfo_b())) {
		split_rx = "A: RX";
		split_tx = "B: TX";
	} else {
		split_rx = "B: RX";
		split_tx = "A: TX";
	}

	UiLcdHy28_PrintText(
	    ts.Layout->TUNE_SPLIT_MARKER_X - (SMALL_FONT_WIDTH * 5),
		ts.Layout->TUNE_FREQ.y,
		split_rx,
		sd.txt_colour, Black,
		0
	);

	UiLcdHy28_PrintText(
	    ts.Layout->TUNE_SPLIT_MARKER_X - (SMALL_FONT_WIDTH * 5),
		ts.Layout->TUNE_SPLIT_FREQ_Y_TX,
		split_tx,
		sd.txt_colour, Black,
		0
	);
#else
	const char *txt = is_vfo_b() ? "RX:B / TX:A" : "RX:A / TX:B";

	UiLcdHy28_PrintTextCentered(
	    ts.Layout->TUNE_SPLIT_MARKER_X,
	    ts.Layout->TUNE_SPLIT_FREQ_Y_TX,
	    20 * 8,
	    txt,
	    sd.txt_colour_dark, Black,
	    0
	);
#endif
}

void UiAction_CopyVfoAB() {
    VfoReg* vfo_store = &vfo[is_vfo_b()? VFO_A : VFO_B].band[ts.band->band_mode];

	vfo_store->dial_value = df.tune_new;
	vfo_store->decod_mode = ts.dmod_mode;                   // copy active VFO settings into other VFO
	vfo_store->digital_mode = ts.digital_mode;
	vfo_store->dial_delta = ts.iq_freq_delta;

	UiDriver_FrequencyUpdateLOandDisplay(true);

	if (ts.menu_mode == false) {
		UiSpectrum_Clear();          // clear display under spectrum scope
		UiLcdHy28_PrintText(80, 160, is_vfo_b() ? "VFO B -> VFO A" : "VFO A -> VFO B", Cyan, Black, 1);
		HAL_Delay(2000);
		UiSpectrum_Init();           // init spectrum scope
	}
}

void UiAction_ToggleVfoAB() {

	uint32_t old_dmod_mode = ts.dmod_mode;

	/**
	 * @warning - The two functions below call the same function
	 * -> RadioManagement_SetDemodMode() at the end,
	 * so some reorganization should be done to better handle switching modes,
	 * in a more central way...
	 */
	RadioManagement_ToggleVfoAB();
	UiDriver_SetDemodMode(ts.dmod_mode);

	UiDriver_FButton_F4ActiveVFO();

	// do frequency/display update

	if (is_splitmode()) {
		UiDriver_DisplaySplitFreqLabels();
#ifdef USE_DISP_480_320
	} else {
	    UiDriver_DisplayFreqLabels();
#endif
	}

	// Change decode mode if need to
	if (ts.dmod_mode != old_dmod_mode) {
		UiDriver_UpdateDisplayAfterParamChange();
	} else {
		UiDriver_FrequencyUpdateLOandDisplay(true);
	}
}

void UiDriver_SetSplitMode(bool mode_active) {
	if (mode_active) {
		ts.vfo_mem_mode |= VFO_MEM_MODE_SPLIT;      // turn on MSB to activate SPLIT
	} else {
		ts.vfo_mem_mode &= ~VFO_MEM_MODE_SPLIT;     // turn off MSB to turn off SPLIT
	}
	UiDriver_CreateMainFreqDisplay(false);
	UiDriver_FrequencyUpdateLOandDisplay(true);
}

/**
 * @brief: process hardcoded buttons click and hold
 */
void UiDriver_RefreshEncoderDisplay() {
	UiDriver_DisplayEncoderMode(ENC1);
	UiDriver_DisplayEncoderMode(ENC2);
	UiDriver_DisplayEncoderMode(ENC3);
	UiDriver_DisplayDSPMode();
}

/**
 * @brief This is THE function to call after changing operational parameters such as frequency or demod mode
 * It will make sure to update the display AND also tunes to a newly selected frequency if not already tuned to it.
 */
bool UiDriver_IsDemodModeChange() {
	bool retval = (ts.dmod_mode != ui_driver_state.dmod_mode);
	retval |= ts.dmod_mode == DEMOD_DIGI && ts.digital_mode != ui_driver_state.digital_mode;

	return retval;
}

/**
 * @brief cleans out mode specific ui elements before switching to next mode
 */
void UiDriver_ModeSpecificDisplayClear(uint8_t dmod_mode, uint8_t digital_mode) {
	switch(dmod_mode) {
	case DEMOD_CW:
		UiDriver_TextMsgClear();
		CwDecoder_WpmDisplayClearOrPrepare(false);
		UiSpectrum_InitCwSnapDisplay(false);
		break;
	case DEMOD_DIGI:
	{
		switch(digital_mode)
		{
#ifdef USE_FREEDV
		case DigitalMode_FreeDV:
			FreeDv_DisplayClear();
			break;
#endif
		case DigitalMode_RTTY:
		case DigitalMode_BPSK:
			UiDriver_TextMsgClear();
			UiSpectrum_InitCwSnapDisplay(false);
			break;
		default:
			break;
		}
	}
	break;
	case DEMOD_AM:
	case DEMOD_SAM:
		UiSpectrum_InitCwSnapDisplay(false);
	break;

	default:
		break;
	}
}

/**
 * @brief prepares mode specific ui elements run the mode
 */
void UiDriver_ModeSpecificDisplayPrepare(uint8_t dmod_mode, uint8_t digital_mode) {
	switch(dmod_mode)
	{
	case DEMOD_CW:
	    if (ts.cw_decoder_enable == true)
	    {
	        CwDecoder_WpmDisplayClearOrPrepare(true);
	    }
	    if(cw_decoder_config.snap_enable == true && ts.dmod_mode == DEMOD_CW)
	    {
	    	UiSpectrum_InitCwSnapDisplay(true);
	    }
		break;
	case DEMOD_DIGI:
	{
		switch(digital_mode)
		{
#ifdef USE_FREEDV
		case DigitalMode_FreeDV:
			FreeDv_DisplayPrepare();
			break;
#endif
		case DigitalMode_RTTY:
		case DigitalMode_BPSK:
			UiDriver_TextMsgClear();
		    if(cw_decoder_config.snap_enable == true)
		    {
		    	UiSpectrum_InitCwSnapDisplay(true);
		    }
			break;
		default:
			break;
		}
	}
	break;
	case DEMOD_AM:
	case DEMOD_SAM:
	    if(cw_decoder_config.snap_enable == true)
	    {
	    	UiSpectrum_InitCwSnapDisplay(true);
	    }
	break;

	default:
		break;
	}
}

void UiDriver_UpdateDemodSpecificDisplayAfterParamChange() {
    // clear display content specific for the old mode
    UiDriver_ModeSpecificDisplayClear(ui_driver_state.dmod_mode,ui_driver_state.digital_mode);
    // prepare mode specific UI elements used in the new mode
    UiDriver_ModeSpecificDisplayPrepare(ts.dmod_mode,ts.digital_mode);
    ui_driver_state.dmod_mode = ts.dmod_mode;
    ui_driver_state.digital_mode = ts.digital_mode;
}

static void UiDriver_MapColors() {
    UiMenu_MapColors(ts.box_colour, NULL, &sd.boxes_colour);
    UiMenu_MapColors(ts.txt_colour, NULL, &sd.txt_colour);

    sd.txt_colour_dark = UiDriver_DarkColor(sd.txt_colour);
}

void UiDriver_UpdateDisplayAfterParamChange() {
    // TODO Maybe we should split this, so that we clear BEFORE doing the general stuff
    // and prepare after, but for now it should work this way

    UiDriver_MapColors();

	if (UiDriver_IsDemodModeChange()) {
	    uint8_t mode = UiDriver_GetModeCode();

	    df.tuning_step  = tune_steps[df.selected_idx[mode]];
	    UiDriver_UpdateDemodSpecificDisplayAfterParamChange();
	}

	UiDriver_CreateSomeKindDisplay();
	UiDriver_CreateMeters();

    if (is_splitmode()) {
        UiDriver_DisplaySplitFreqLabels();
#ifdef USE_DISP_480_320
    } else {
        UiDriver_DisplayFreqLabels();
#endif
    }

	UiDriver_FrequencyUpdateLOandDisplay(false);   // update frequency display without checking encoder
	UiDriver_DisplayDemodMode();
	UiDriver_DisplayBandForFreq(df.tune_new, true);
	UiDriver_DisplayMemoryLabel();
	UiDriver_DisplayFilter();    // make certain that numerical on-screen bandwidth indicator is updated
	UiSpectrum_DisplayFilterBW();  // update on-screen filter bandwidth indicator (graphical)
	UiDriver_RefreshEncoderDisplay();

	if (ts.menu_mode) {
		UiMenu_RenderMenu(MENU_RENDER_ONLY);    // yes, update display when we change modes
	}

	UiVk_Redraw();			//virtual keypads call (refresh purpose)
    UiDriver_DisplayPowerLevel();
    UiDriver_DisplayFreqStepSize();
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverPressHoldStep
//* Object              : Select the step size for the press-and-hold of the step size button
//* Input Parameters    : 0=Decrease step size, 1=Increase step size
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void UiDriver_PressHoldStep(uchar is_up)
{
	ulong	minus_idx, plus_idx;
	uint8_t mode = UiDriver_GetModeCode();

	switch(df.selected_idx[mode])	 		// select appropriate "alternate" step size based on current step size
	{
	case T_STEP_1HZ_IDX:	// 1Hz step size
	case T_STEP_5HZ_IDX:	// 10Hz step size
		minus_idx = T_STEP_1HZ_IDX;		// use 1 Hz as small step size
		plus_idx = T_STEP_1KHZ_IDX;		// use 1 kHz as large step size
		break;
	case T_STEP_10HZ_IDX:	// 10Hz step size
		minus_idx = T_STEP_1HZ_IDX;		// use 1 Hz as small step size
		plus_idx = T_STEP_1KHZ_IDX;		// use 1 kHz as large step size
		break;
	case T_STEP_50HZ_IDX:	// 10Hz step size
		minus_idx = T_STEP_1HZ_IDX;		// use 1 Hz as small step size
		plus_idx = T_STEP_1KHZ_IDX;		// use 1 kHz as large step size
		break;
	case T_STEP_100HZ_IDX:	// 100Hz step size
		minus_idx = T_STEP_10HZ_IDX;		// use 10 Hz as small step size
		plus_idx = T_STEP_1KHZ_IDX;		// use 1 kHz as large step size
		break;
	case T_STEP_10KHZ_IDX:	// 10 kHz step size
	case T_STEP_100KHZ_IDX:	// 100 kHz step size
		minus_idx = T_STEP_100HZ_IDX;	// use 100 Hz as small step size
		plus_idx = T_STEP_100KHZ_IDX;	// use 100 kHz as large step size
		break;
	case T_STEP_1KHZ_IDX:	// 1 kHz step size
	default:
		minus_idx = T_STEP_10HZ_IDX;	// use 10 Hz as small step size
		plus_idx = T_STEP_10KHZ_IDX;	// use 10 kHz as large step size
		break;
	}

	if(!is_up)	 		// temporary decrease of step size
	{
		ts.tune_step = STEP_PRESS_MINUS;
		ts.tune_step_idx_holder = df.selected_idx[mode];
		if(df.selected_idx[mode])
			df.tuning_step	= tune_steps[minus_idx];
		df.selected_idx[mode] = minus_idx;
	}
	else	 			// temporary increase of step size
	{
		ts.tune_step = STEP_PRESS_PLUS;
		ts.tune_step_idx_holder = df.selected_idx[mode];
		df.tuning_step	= tune_steps[plus_idx];
		df.selected_idx[mode] = plus_idx;
	}
	//
	UiDriver_DisplayFreqStepSize();		// update display
}

void UiDriver_DisplayDemodMode() {
	char* txt = "???";
	uint16_t color= sd.txt_colour;

    switch(ts.dmod_mode) {
        case DEMOD_USB:
            txt = "USB";
            break;

        case DEMOD_LSB:
            txt = "LSB";
            break;

        case DEMOD_SAM:
            if (ads.sam_sideband == SAM_SIDEBAND_LSB) {
                txt = "SAM-L";
            } else if (ads.sam_sideband == SAM_SIDEBAND_USB) {
                txt = "SAM-U";
            }
    #ifdef USE_TWO_CHANNEL_AUDIO
            else if (ads.sam_sideband == SAM_SIDEBAND_STEREO) {
                txt = "SAM-S";
            }
    #endif
            else {
                txt = "SAM";
            }
            break;

        case DEMOD_AM:
            txt = "AM";
            break;

        case DEMOD_FM:
            txt = RadioManagement_FmDevIs5khz() ? "FM-W" : "FM-N";

            if (ts.txrx_mode == TRX_MODE_RX) {
                if (ads.fm_conf.squelched == false) {
                    // is audio not squelched?
                    if ((ads.fm_conf.subaudible_tone_detected) && (ts.fm_subaudible_tone_det_select)) {
                        // is tone decoding enabled AND a tone being detected?
                        color = Red2;	// Not squelched, passing audio - change color!
                    } else { 	        // tone decoder disabled - squelch only
                        color = Cyan;	// Not squelched, passing audio - change color, but different from tone
                    }
                }
            } else if (ts.txrx_mode == TRX_MODE_TX) {	 	// in transmit mode?
                if (ads.fm_conf.tone_burst_active) {	 		// yes - is tone burst active?
                    color = Yellow;	// Yes, make "FM" yellow
                }
            }
            break;

        case DEMOD_CW:
            txt = ts.cw_lsb ? "CW-L" : "CW-U";
            break;

        case DEMOD_DIGI:
            switch (ts.digital_mode) {
                case DigitalMode_RTTY:
                    txt = ts.digi_lsb ? "RT-L" : "RT-U";
                    break;

                case DigitalMode_BPSK:
                    txt = ts.digi_lsb ? "PSK-L" : "PSK-U";
                    break;

                default:
                    txt = ts.digi_lsb ? "DI-L" : "DI-U";
                }
            break;

    #ifdef USE_TWO_CHANNEL_AUDIO
        case DEMOD_IQ:
            txt = "IQ-S";
            break;

        case DEMOD_SSBSTEREO:
            txt = "SSB-S";
            break;

    #endif
        default:
            break;
    }

    UiLcdHy28_DrawEmptyRect(
        ts.Layout->DEMOD_MODE_MASK.x,
        ts.Layout->DEMOD_MODE_MASK.y,
        ts.Layout->DEMOD_MODE_MASK.h,
        ts.Layout->DEMOD_MODE_MASK.w,
        sd.boxes_colour
    );

    UiLcdHy28_PrintTextCentered(
            ts.Layout->DEMOD_MODE_MASK.x + 1,
            ts.Layout->DEMOD_MODE_MASK.y + 2,
            ts.Layout->DEMOD_MODE_MASK.w - 2,
            txt,
            color, Black,
            0
    );
}

/**
 * This function gives a visual indication of the selected step size for the tuning knob. It draws a line under the respective digit in the frequency.
 * This function is closely coupled to the code for displaying the frequency digits.
 */
void UiDriver_DisplayFreqStepSize() {
	static	bool	step_line = 0;	// used to indicate the presence of a step line

	const uint16_t font_width = is_splitmode()?SMALL_FONT_WIDTH:LARGE_FONT_WIDTH;
    const uint16_t x_pos = is_splitmode() ? ts.Layout->TUNE_SPLIT_FREQ_X : ts.Layout->TUNE_FREQ.x;
    const uint16_t x_right = x_pos + (9 * font_width);
	const uint32_t color = ts.tune_step ? Cyan : sd.txt_colour;		// is this a "Temporary" step size from press-and-hold?
	const uint32_t stepsize_background = (ts.flags1 & FLAGS1_DYN_TUNE_ENABLE) ? sd.boxes_colour : Black;

	if (step_line) {	 	// Remove underline indicating step size if one had been drawn
        const int32_t space_l = 3*(LARGE_FONT_WIDTH * 3 + LARGE_FONT_WIDTH/2); //3 digits plus a half width dot
        const int32_t space_s = 3*(SMALL_FONT_WIDTH * 3 + SMALL_FONT_WIDTH/2); //3 digits plus a half width dot

		UiLcdHy28_DrawStraightLineDouble(ts.Layout->TUNE_FREQ.x, (ts.Layout->TUNE_FREQ.y + 24), space_l, LCD_DIR_HORIZONTAL, Black);
		UiLcdHy28_DrawStraightLineDouble(ts.Layout->TUNE_SPLIT_FREQ_X, (ts.Layout->TUNE_FREQ.y + 24), space_s, LCD_DIR_HORIZONTAL, Black);
	}

	// Blank old step size
	// UiLcdHy28_DrawFullRect(POS_TUNE_STEP_X,POS_TUNE_STEP_Y-1,POS_TUNE_STEP_MASK_H,POS_TUNE_STEP_MASK_W,stepsize_background);

	char step_name[10];
	// I know the code below will not win the price for the most readable code
	// ever. But it does the job of display any freq step somewhat reasonable.
	// khz/Mhz only whole  khz/Mhz is shown, no fraction
	// showing fractions would require some more coding, which is not yet necessary
	const int32_t pow10 = log10f(df.tuning_step);
	const int32_t digit_group = pow10/3;
	const int32_t digit_idx = pow10%3;

	const char* stepUnitPrefix[] = { "Hz","k","M","G","T"};
	snprintf(step_name,10,"%d%s",(int)(df.tuning_step/exp10((digit_group)*3)), stepUnitPrefix[digit_group]);

	UiLcdHy28_PrintTextCentered(ts.Layout->TUNE_STEP.x,ts.Layout->TUNE_STEP.y,ts.Layout->TUNE_STEP.w,step_name,color,stepsize_background,0);

	if ((ts.freq_step_config & FREQ_STEP_SHOW_MARKER) && pow10 < MAX_DIGITS) {         // is frequency step marker line enabled?
	    const int32_t group_space = (font_width * 3) + font_width/2; //3 digits plus a half width dot
	    const uint32_t line_pos =  x_right -  digit_idx * font_width - (digit_group * group_space);

	    UiLcdHy28_DrawStraightLineDouble(line_pos, (ts.Layout->TUNE_FREQ.y + 24), font_width, LCD_DIR_HORIZONTAL, sd.txt_colour);
	    step_line = 1;	// indicate that a line under the step size had been drawn
	} else {	// marker line not enabled
	    step_line = 0;	// we don't need to erase "step size" marker line in the future
	}
}


typedef struct
{
	uint32_t start;
	uint32_t end;
	const char* name;
} BandGenInfo;

const BandGenInfo bandGenInfo[] =
{
		{150000, 285000, "LW" },
		{525000, 1605000, "MW" },
		{2300000, 2495000, "120m" },
		{3200000, 3400000, "90m" },
		{3900000, 4000000, "75m" },
		{4750000, 5060000, "60m" },
		{5950000, 6200000, "49m" },
		{7300000, 7350000, "41m" },
		{9400000, 9900000, "31m" },
		{11600000, 12100000, "25m" },
		{13570000, 13870000, "22m" },
		{15100000, 15800000, "19m" },
		{17480000, 17900000, "16m" },
		{18900000, 19020000, "15m" },
		{21450000, 21750000, "13m" },
        {25670000, 26959999, "11m" },
        {26960000, 27991250, "11m" },
		{ 0,  0,             "Gen" }
};

//*----------------------------------------------------------------------------

void UiDriver_ClearMemoryLabel() {
    UiLcdHy28_DrawFullRect(ts.Layout->MEMORYLABEL.x, ts.Layout->MEMORYLABEL.y, 8, SMALL_FONT_WIDTH * 5, Black);
}

void UiDriver_DisplayMemoryLabel() {
    char txt[12];

    if (ts.expflags1 & EXPFLAGS1_NO_SHOW_BNDMEM) {
        UiLcdHy28_DrawFullRect(ts.Layout->MEMORYLABEL.x, ts.Layout->MEMORYLABEL.y, 8, SMALL_FONT_WIDTH * 5, Black);
        return;
    }

	if (ts.band->band_mode < MAX_BAND_NUM && ts.cat_band_index == 255) {
#ifdef USE_MEMORY_MODE
	    // Enable all band memories, don't use band names
        snprintf(txt,12,"M%02d ", ts.band->band_mode);
#else
        // Each memory has its designated band, use that as band
        snprintf(txt, sizeof(txt), "B%s ", ts.band->name);
#endif
	}

	if (ts.cat_band_index != 255) {		// no band storage place active because of "CAT running in sandbox"
		snprintf(txt, sizeof(txt), " CAT ");
	}

	UiLcdHy28_PrintText(ts.Layout->MEMORYLABEL.x,  ts.Layout->MEMORYLABEL.y, txt, sd.txt_colour, Black, 4);
}

/**
 * Display the ham or broadcast band name of the currently selected band
 * @param band the band id, last band id is BAND_MODE_GEN, which is everything outside ham bands
 */
static void UiDriver_DisplayBand(const BandInfo* band) {

	if (band != NULL) {
	    const char* bandName;
	    bool print_bc_name = true;

		uint16_t col = sd.txt_colour; // default color for non-bc band

		// only if we are not in a ham band, we check the name of a broadcast band
		if (RadioManagement_IsGenericBand(band)) {
		    int idx;

			for (idx = 0; bandGenInfo[idx].start !=0; idx++) {
				if (df.tune_old >= bandGenInfo[idx].start && df.tune_old < bandGenInfo[idx].end) {
					break; // found match
				}
			}

			if (bandGenInfo[idx].start !=0) {
				// Print name of BC band in yellow, if frequency is within a broadcast band
				col = Yellow;
			}

			if  (bandGenInfo[idx].start == 26965000) {
				col = Blue;		// CB radio == blue
			}

			if (idx == ts.bc_band) {
				print_bc_name = false;
			}

			ts.bc_band = idx;

			bandName = bandGenInfo[idx].name;
		} else {
			print_bc_name = true;
			bandName = band->name;
			ts.bc_band = 0xff;
		}

		if (print_bc_name) {
			UiLcdHy28_DrawFullRect(
			    ts.Layout->BAND_MODE.x,
			    ts.Layout->BAND_MODE.y,
			    ts.Layout->BAND_MODE.h,
			    ts.Layout->BAND_MODE.w,
			    Black
			);

			UiLcdHy28_PrintTextRight(
			    ts.Layout->BAND_MODE.x + ts.Layout->BAND_MODE.w,
			    ts.Layout->BAND_MODE.y,
			    bandName,
			    col, Black,
			    0
			);
		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverInitMainFreqDisplay
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void UiDriver_CreateMainFreqDisplay(bool all_digits)
{
//	UiDriver_FButton_F3MemSplit();
    // TODO: Adjust ts.Layout->TUNE_FREQ.x to match 10 digits display approach, would simplify code
    const uint16_t font_width = LARGE_FONT_WIDTH;
    const uint16_t x_right = ts.Layout->TUNE_FREQ.x + (9* font_width);
    const int32_t group_space = (font_width * 3) + font_width/2; //3 digits plus a half width dot
    uint32_t box_width = 0;

    if (!(ts.xverter_mode & 0xf) && !all_digits) {// XVTR is OFF & not forse clear all digits
    	box_width = (2 * font_width) + (2 * group_space); // == 8 digits
    } else {
    	box_width =  font_width + (3 * group_space); // 3 x 3 digits in a group with a dot + 1 x single digit == 10 digits
    }
    UiLcdHy28_DrawFullRect(x_right - box_width,ts.Layout->TUNE_FREQ.y,24, box_width, Black);

    // clear frequency display area for large digits, which is also the max area for split

    UiDriver_FButton_F3MemSplit();

    if (is_splitmode()) {
		UiDriver_DisplaySplitFreqLabels();
#ifdef USE_DISP_480_320
	} else {
        UiDriver_DisplayFreqLabels();
#endif
	}

    UiDriver_DisplayFreqStepSize();
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverCreateFunctionButtons
//* Object              : function keys based on decoder mode
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void UiDriver_CreateFunctionButtons(bool full_repaint) {
	// Create bottom bar
	if (full_repaint) {
		for (int i = 0; i < BOTTOM_BUTTON; i++) {
			UiLcdHy28_DrawBottomButton(
			    (ts.Layout->BOTTOM_BAR.x + (ts.Layout->BOTTOM_BAR.w+1)*i),
			    (ts.Layout->BOTTOM_BAR.y - 4),
			    ts.Layout->BOTTOM_BAR.h,
			    ts.Layout->BOTTOM_BAR.w,
			    Grey
			 );
		}
	}

	// Button F1
	UiDriver_DisplayFButton_F1MenuExit();
	// Button F2
	UiDriver_DisplayFButton_F2SnapMeter();
	// Button F3
	UiDriver_FButton_F3MemSplit();
	// Button F4
	UiDriver_FButton_F4ActiveVFO();
	// Button F5
	UiDriver_FButton_F5Tune();

#if BOTTOM_BUTTON > 5
	UiDriver_FButton_F6Illum();
#endif

#if BOTTOM_BUTTON > 6
	UiDriver_FButton_F7AT();
#endif
}

void UiDriver_SetSpectrumMode(SpectrumMode_t mode) {
    ts.flags1 = (ts.flags1 & ~(FLAGS1_SCOPE_ENABLED | FLAGS1_WFALL_ENABLED)) | (mode << 7);
}

SpectrumMode_t UiDriver_GetSpectrumMode() {
    return (ts.flags1 & (FLAGS1_SCOPE_ENABLED | FLAGS1_WFALL_ENABLED))  >> 7;
}

// #define RGB(red,green,blue)(uint16_t)(((red>>3)<<11)|((green>>2)<<5)|(blue>> 3))

static uint32_t UiDriver_DarkColor(uint32_t color) {
    uint16_t r = (color >> 11);
    uint16_t g = (color >> 5) & 0b111111;
    uint16_t b = color & 0b11111;

    r = (r * 2) / 3;
    g = (g * 2) / 3;
    b = (b * 2) / 3;

    return (r << 11) | (g << 5) | b;
}

//
//*----------------------------------------------------------------------------
//* Function Name       : UiDriverDrawSMeter
//* Object              : draw the part of the S meter
//* Input Parameters    : uchar color
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void UiDriver_CreateDesktop(void) {
	// Backlight off - hide startup logo
	UiLcdHy28_BacklightEnable(false);

	// Clear display
	UiLcdHy28_LcdClear(Black);

	if (UiDriver_GetSpectrumMode() == SPECTRUM_BLANK) {
	    UiDriver_SetSpectrumMode(SPECTRUM_DUAL);
	}

	UiDriver_MapColors();

    UiDriver_CreateMainFreqDisplay(false);
    UiDriver_CreateFunctionButtons(true);

	UiSpectrum_Init();

	UiDriver_RefreshEncoderDisplay();
	UiDriver_DisplayPowerLevel();

	UiDriver_FrequencyUpdateLOandDisplay(true);

	ts.dsp.active =  DSP_OFF;

	UiDriver_UpdateDisplayAfterParamChange();

    UiDriver_CreateMeters();
    UiDriver_CreateSomeKindDisplay();

	// Backlight on - only when all is drawn
	UiLcdHy28_BacklightEnable(true);

#ifdef SDR_AMBER
	if(ts.expflags1 & EXPFLAGS1_SHOW_GREETING) {
	    UiDriver_ShowGreeting(0);
	    ts.expflags1 = ts.expflags1 & (~EXPFLAGS1_SHOW_GREETING); // show just one time
	}

    Board_AmberIOx8_Write(255);  // set all ports
    Board_IllumButt();
    Board_AmberIOx4_Write(3, 0); // configurate all ports as outputs
    ts.amber_io4_state = 0;
    Board_AmberIOx4_Write(1, 0); // reset all ports
#endif

    if(ts.expflags2 & EXPFLAGS2_DISABLE_TP) {
        ts.disabled_tp = true;
    }

#ifndef UI_BRD_MCHF
    MIC_bias_set();
#endif

	CW_Smooth_Settings();
}

#define BTM_PLUS (2)

static void UiDriver_DeleteMeters() {
	UiLcdHy28_DrawFullRect(
	    ts.Layout->SM_IND.x+1,
	    ts.Layout->SM_IND.y+1,
	    ts.Layout->SM_IND.h,
	    ts.Layout->SM_IND.w,
	    Black
	);
}

static void UiDriver_DrawPowerMeterLabels(void) {
    const uint16_t  y_pos = ts.Layout->SM_IND.y + 3 + BTM_PLUS;
    const uint16_t  x_pos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    const int32_t   maxW = ((mchf_pa.max_power > 5000) ? mchf_pa.max_power : 5000)  / 1000;
    const uint16_t  step = (ts.Layout->SM_IND.size + ts.Layout->SM_IND.space) * 3;

    // get the pwr increment in next integer number of 0.5W steps
    const float32_t PWR_INCR = (maxW % 5 == 0) ? (maxW/10.0) : (maxW/5+1)/2.0;

    // Labels

    UiLcdHy28_PrintText(
        x_pos - 12 ,
        y_pos,
        "P",
        White, Black, 4
    );

	// Draw s markers on middle white line
	for(int i = 0; i < 11; i++) {
		uint16_t    pwr_val = i * PWR_INCR;
		uchar       v_s;

		if (!(i % 2)) {
	        char    num[8];

	        snprintf(num, sizeof(num), i == 10 ? "%dW" : "%d", pwr_val);

		    const int dw = UiLcdHy28_TextWidth(num, 4);

			UiLcdHy28_PrintText(
			    (x_pos - dw/2 + i * step),
			    y_pos,
			    num,
			    White, Black, 4
			);

			v_s = 3;
		} else {
		    v_s = 5;
		}

		UiLcdHy28_DrawStraightLine(
		    (x_pos + i * step),
		    (y_pos + 15) - v_s,
		    v_s,
		    LCD_DIR_VERTICAL, White
		);
	}
}

static void UiDriver_DrawSMeterLabels(void) {
	uchar       v_s;
	char        num[8];
	uint16_t    xpos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    uint8_t     step = (ts.Layout->SM_IND.size + ts.Layout->SM_IND.space) * 2;

	// Labels

	UiLcdHy28_PrintText(
	   (xpos - 12),
	   (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
	   "S",
	   White, Black, 4
	 );

	// Draw s markers on top white line

	for (int i = 0; i < 10; i++) {
		// Draw s text, only odd numbers
		if (i % 2) {
	        snprintf(num, sizeof(num), "%d", i);

	        UiLcdHy28_PrintText(
			    (xpos - 4 + i * step),
			    (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
			    num,
			    White, Black,
			    4
			);
			v_s = 5;
		} else {
			v_s = 3;
		}

		// Lines
		UiLcdHy28_DrawStraightLine(
		    (xpos + i * step),
		    ((ts.Layout->SM_IND.y + 18 + BTM_PLUS) - v_s),
		    v_s,
		    LCD_DIR_VERTICAL, White
		);
	}

	// Draw s markers on top green line

	for (int i = 1; i < 4; i++) {
        snprintf(num, sizeof(num), i == 3 ? "%ddB" : "%d", i * 20);

        // Draw text
        UiLcdHy28_PrintText(
            ((xpos + step * 9) - 6 + i * step * 2),
            (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
            num,
            Green,Black,
            4
        );

        // Draw vert lines
        UiLcdHy28_DrawStraightLine(
           ((xpos + step * 9) + i * step * 2),
           (ts.Layout->SM_IND.y + 13 + BTM_PLUS),
           5, LCD_DIR_VERTICAL,
           Green
        );
	}
}

static void UiDriver_DrawSWRMeterLabels() {
    int         col = White;
    char        num[20];
    uchar       v_s = 5;
    uint16_t    xpos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    uint8_t     step = (ts.Layout->SM_IND.size + ts.Layout->SM_IND.space) * 2;

    // Label

    UiLcdHy28_PrintText(
        (xpos - 12),
        (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
        "SWR",
        Red2, Black, 4
     );

    // Draw S markers on middle white line
    for (int i = 0; i < 12; i++) {
        if (i > 6)
            col = Red;

        if (! (i % 2)) {
            if (i) {
                num[0] = i / 2 + 0x30;
                num[1] = 0;

                // Text
                UiLcdHy28_PrintText(
                    (xpos - 3 + i * step),
                    (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
                    num,
                    White, Black, 4
                );

                v_s = 3;
            }

            UiLcdHy28_DrawStraightLine(
               (xpos + i * step),
               (ts.Layout->SM_IND.y + 18 + BTM_PLUS - v_s),
               v_s,
               LCD_DIR_VERTICAL, col
            );
        }
    }
}

static void UiDriver_DrawALCMeterLabels() {
    int         col = White;
    char        num[20];
    uchar       v_s = 5;
    uint16_t    xpos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    uint8_t     step = (ts.Layout->SM_IND.size + ts.Layout->SM_IND.space) * 2;

    // Label

    UiLcdHy28_PrintText(
        (xpos - 12),
        (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
        "ALC",
        Yellow, Black, 4
    );

    // Draw markers on middle line

    for (int i = 0; i < 17; i++) {
        if (i > 6)
            col = Red;

        if (! (i % 2)) {
            if (i) {
                snprintf(num, 20, "%d", (i * 2));
                // Text
                UiLcdHy28_PrintText(
                    (xpos - 3 + i * step),
                    (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
                    num,
                    White, Black, 4
                 );

                v_s = 3;
            }

            UiLcdHy28_DrawStraightLine(
                (xpos + i * step),
                (ts.Layout->SM_IND.y + 18 - v_s + BTM_PLUS),
                v_s,
                LCD_DIR_VERTICAL, col
            );
        }
    }
}

static void UiDriver_DrawAudioMeterLabels() {
    char        num[20];
    int         col = White;
    uchar       v_s = 5;
    uint16_t    xpos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    uint8_t     step = (ts.Layout->SM_IND.size + ts.Layout->SM_IND.space) * 2;

    // Label

    UiLcdHy28_PrintText(
        (xpos - 12),
        (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
        "AUD",
        Cyan, Black, 4
     );

    // Draw markers on middle line
    for (int i = 0; i < 17; i++) {
        if (i > 10)
            col = Red;

        if (!(i%2)) {
            if (i) {
                snprintf(num, 20, "%d", (i*2) -20);
                // Text
                UiLcdHy28_PrintText(
                    (xpos - 3 + i * step),
                    (ts.Layout->SM_IND.y + 3 + BTM_PLUS),
                    num, White, Black,
                    4
                );

                v_s = 3;
            }

            UiLcdHy28_DrawStraightLine(
               (xpos + i * step),
               (ts.Layout->SM_IND.y + 18 + BTM_PLUS - v_s),
               v_s,
               LCD_DIR_VERTICAL, col
            );
        }
    }
}

static void UiDriver_CreateMeters() {
	UiLcdHy28_DrawEmptyRect(
	    ts.Layout->SM_IND.x,
	    ts.Layout->SM_IND.y,
	    ts.Layout->SM_IND.h,
	    ts.Layout->SM_IND.w + 2,
	    sd.boxes_colour
	);

	if (ts.txrx_mode == TRX_MODE_RX) {
		UiDriver_DrawSMeterLabels();

		UiDriver_UpdateMeterRX(34);
		UiDriver_UpdateMeterRX(0);
	} else {
	    switch (ts.tx_meter_mode) {
	        case METER_PWR:
	            UiDriver_DrawPowerMeterLabels();
	            break;

	        case METER_SWR:
	            UiDriver_DrawSWRMeterLabels();
	            break;

	        case METER_ALC:
                UiDriver_DrawALCMeterLabels();
	            break;

	        case METER_AUDIO:
                UiDriver_DrawAudioMeterLabels();
                break;
	    }
	    UiDriver_UpdateMeterTX(34, 34);
	    UiDriver_UpdateMeterTX(0, 34);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverUpdateTopMeterA
//* Object              : redraw indicator, same like upper implementation
//* Input Parameters    : but no hold
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
#define SMETER_MAX_LEVEL 33

enum {
	METER_RX = 0,
	METER_TX,
	METER_NUM
};

typedef struct MeterState_s {
	uint8_t last;
	uint8_t last_warn;
	uint8_t peak;
	uint32_t timer;
} MeterState;

static MeterState meters[METER_NUM];

static uint8_t SMmode;

/**
 * Displays a horizontal meter bar as dash segments.
 *
 * @param val value to display. Max value (100%) is SMETER_MAX_LEVEL, higher values are cut off.
 * @param warn At which value shall value is in red color. 0 to disable.
 * @param color_norm default, non-warning color
 * @param meterId index of the meter in the internal meter data storage.
 */
static void UiDriver_UpdateMeter(uchar val, uchar warn, uint32_t color_norm, uint8_t meterId) {
    assert(meterId < METER_NUM);

	uint8_t from_warn = 255;

	// limit meter
	if(val > SMETER_MAX_LEVEL)
		val = SMETER_MAX_LEVEL;

	if (warn == 0)
		warn = SMETER_MAX_LEVEL+1;    // never warn if warn == 0

    if (warn != meters[meterId].last_warn) {
        if (warn < meters[meterId].last_warn) {
            from_warn = warn;
        } else {
            from_warn = meters[meterId].last_warn;
        }
    }

    const uint16_t  ypos = ts.Layout->SM_IND.y + 20 + BTM_PLUS;
    const uint16_t  xpos = ts.Layout->SM_IND.x + 18 + ts.Layout->SM_IND.padding;
    const uint8_t   size = ts.Layout->SM_IND.size;
    const uint8_t   space = ts.Layout->SM_IND.space;

    // Peak indication
    if (ts.peak_ind_tune != 0 && meterId == 0) { // Draw just for S/PWR meter, if peak ind. is ON
        // reset p.i. on mode change
        uint8_t SMmode_new = (ts.txrx_mode == TRX_MODE_RX) ? 2 : 0;

        if (SMmode_new != SMmode) {
            meters[meterId].timer = ts.sysclock; //0;
            SMmode = SMmode_new;
        }

        if (val >= meters[meterId].peak) {
            meters[0].peak = val;
            meters[0].timer = ts.sysclock + (ts.peak_ind_tune * 25);
        }

        if (meters[0].timer <= ts.sysclock) {
            // Erase peak indicator
            UiLcdHy28_DrawStraightLineTriple(
                (xpos + meters[meterId].peak * (size + space)),
                ypos,
                size,
                LCD_DIR_HORIZONTAL,
                Grid
            );

            meters[meterId].peak = 0;
        } else {
            // Show peak indicator
            UiLcdHy28_DrawStraightLineTriple (
                (xpos + meters[meterId].peak * (size + space)),
                ypos,
                size,
                LCD_DIR_HORIZONTAL,
                color_norm
            );
        }
    }
    // The end of the peak indication block

    if (val != meters[meterId].last || from_warn != 255) {
        uint8_t from, to;

        // decide if we need to draw more boxes or delete some
        if (val > meters[meterId].last) {
            // we will draw more active boxes
            from = meters[meterId].last;
            to = val + 1;
        } else {
            from = val;
            to = meters[meterId].last + 1;
        }

        if (from_warn < from) {
            from = from_warn;
        }

        // Draw indicator
        // we never draw a zero, so we start from 1 min
        if (from == 0) {
            from = 1;
        }

        uint32_t col = color_norm;

        for (int i = from; i < to; i++) {
            if (i > val) {
                col = Grid; // switch to delete color
            }

            if ((i >= warn) && warn && col != Grid) { // is level above "warning" color? (is "warn" is zero, disable warning)
                col = Red2;
            }

            UiLcdHy28_DrawStraightLineTriple(
                (xpos + i * (size + space)),
                ypos,
                size,
                LCD_DIR_HORIZONTAL,
                col
             );
        }

        meters[meterId].last = val;
        meters[meterId].last_warn = warn;
    }
}


static void UiDriver_UpdateMeterRX(uchar val) {
	ulong clr;

	UiMenu_MapColors(ts.meter_colour_up, NULL, &clr);
	UiDriver_UpdateMeter(val, 0, clr, METER_RX);
}

/**
 * @brief updates the lower meter
 * @param val the value to display, max value is S_METER_MAX, min is 0, values will be limited to range
 * @param warn the level value from which the meter is going to show warning indication
 */
static void UiDriver_UpdateMeterTX(float val, uchar warn) {
	ulong clr;
	UiMenu_MapColors(ts.meter_colour_down, NULL, &clr);

	if (val < 0)
		val = 0;

	if (val > S_METER_MAX)
		val = S_METER_MAX;

	UiDriver_UpdateMeter(val, warn, clr, METER_TX);
}

// FIXME: Move to RadioManagement()
void UiDriver_InitBandSet()
{
    // TODO: Do this setting based on the detected RF board capabilities
    // set the enabled bands
    uint32_t min_osc = osc->getMinFrequency();
    uint32_t max_osc = osc->getMaxFrequency();

    // first we enabled all bands for rx based on the reported tuning frequency range
    // please note, that this may enabled bands not really usable
    // as other limitations of the hardware such as lpf/bpf limits
    // etc. may apply. For instance, the standard MCHF RF has RX BPF/LPF
    // limiting RX above 32 Mhz.

    // the PA itself has its own limits which are checked before transmitting
    // so we don't care here about these limits

    for(int i = 0; i < MAX_BANDS; i++)
    {
        const BandInfo* bi = RadioManagement_GetBandInfo(i);
        band_enabled[i] = ((bi->tune >= min_osc) && ((bi->size + bi->tune) <= max_osc));
    }
    // For Robin bagnall
//    band_enabled[BAND_MODE_4] = true;
//    band_enabled[BAND_MODE_6] = true;

    switch (ts.rf_board)
    {
    case FOUND_RF_BOARD_MCHF:
        // here you can enable or disable based on additional hardware capabilities
        // but this should only be used with care
        /*
        band_enabled[BAND_MODE_23] = false;
        band_enabled[BAND_MODE_70] = false;
        band_enabled[BAND_MODE_2] = false;
        band_enabled[BAND_MODE_4] = false;
        band_enabled[BAND_MODE_6] = false;
        band_enabled[BAND_MODE_630] = false;
        band_enabled[BAND_MODE_2200] = false;
        */
        break;
    case FOUND_RF_BOARD_OVI40:
        /*
        band_enabled[BAND_MODE_23] = false;
        band_enabled[BAND_MODE_70] = false;
        */
        break;
    }

	const char* test = Board_BootloaderVersion();
	char res = 0;
	for (int i=0; i<20;i++)			// find last character in bootloader string
	{
		if(test[i] == 0)
		{
			res = test[i-1];
			break;
		}
	}
	if(res == 0x61)					// if it is an "a" ==> DF8OE version, enable all bands
	{
  		for(int i = 0; i < MAX_BANDS; i++)
  		{
      		band_enabled[i] = true;
  		}
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverInitFrequency
//* Object              : set default values, some could be overwritten later
static void UiDriver_InitFrequency()
{
	// Clear band values array
	for(int i = 0; i < MAX_BANDS; i++)
	{
		vfo[VFO_A].band[i].dial_value = 0xFFFFFFFF;	// clear dial values
		vfo[VFO_A].band[i].decod_mode = DEMOD_USB; 	// clear decode mode
        vfo[VFO_A].band[i].digital_mode = DigitalMode_None;   // clear digital mode
		vfo[VFO_B].band[i].dial_value = 0xFFFFFFFF;  // clear dial values
		vfo[VFO_B].band[i].decod_mode = DEMOD_USB;   // clear decode mode
        vfo[VFO_B].band[i].digital_mode = DigitalMode_None;   // clear digital mode
        vfo[VFO_B].band[i].dial_delta = 0;
	}

	// Lower bands default to LSB mode
	// TODO: This needs to be checked, some even lower bands have higher numbers now
	for(int i = 0; i < 4; i++)
	{
		vfo[VFO_A].band[i].decod_mode = DEMOD_LSB;
		vfo[VFO_B].band[i].decod_mode = DEMOD_LSB;
	}

	// Init frequency publics(set diff values so update on LCD will be done)
	df.tune_old 	= 0;
	df.tune_new 	= 3500001;

	for (int i = 0; i < DEMOD_NUM_MODE; i++)
	    df.selected_idx[i] = T_STEP_1KHZ_IDX; 		// 1 Khz startup step

	df.tuning_step	= tune_steps[df.selected_idx[0]];
	df.temp_factor	= 0;
	df.temp_factor_changed = false;
	df.temp_enabled = 0;		// startup state of TCXO

	UiDriver_InitBandSet();
}

/**
 * @brief Checks in which band the current frequency lies and updates display only if changed
 *
 * @param freq frequency in Hz
 * @returns band index (0 - (MAX_BANDS-1))
 */

void UiDriver_DisplayBandForFreq(uint32_t freq, bool force) {
	// here we maintain our local state of the last band shown
    const BandInfo* band = RadioManagement_GetBand(freq);

    // yes, did the tx band actually change?
    // or are we in the generic band (i.e. outside tx bands)

	if (band != ts.band_effective || RadioManagement_IsGenericBand(band) || force) {
		UiDriver_DisplayBand(band);
		UiDriver_HandlePowerLevelChange(band, ts.power_level);
	}

	ts.band_effective = band;

    if (ts.lotx_dacs_present) {
        LO_TX_SUPR_DAC_GetBand(freq);
    }
}


/*
 * @brief Changes the tune frequency according to mode and other settings
 * @param dial_freq The desired dial frequency in Hz (not the tune frequency of the LO)
 * @returns true if the change was executed  (even if it is not tunable freq), false if the change is pending
 */
/*
 * @brief Check if a frequency is tunable
 * @returns SI570_OK, SI570_LARGE_STEP, SI570_TUNE_LIMITED if ok, SI570_TUNE_IMPOSSIBLE if not OK
 */
/*
 * @brief Used to update the individual vfo displays, not meant to be called directly except when changing LO
 * @brief parameters (in this case use (true,0)), use UiDriver_FrequencyUpdateLOandDisplay(full_update) instead
 *
 * @param force_update true = unconditionally update synthesizer EVEN IF frequency did not change
 * @param mode  =0 automatic, 1=force large, 2=force small, upper (RX), 3 = small, lower (TX)
 *
 * WARNING:  If called with "mode = 3", you must ALWAYS call again with "mode = 2" to reset internal variables.
 */
/*
 * @brief change LO freq to match df.tune_new freq according to mode without updating the ui
 *
 * @param trx_mode The mode which the frequency is being used for (TRX_MODE_TX/TRX_MODE_RX)
 */
void UiDriver_UpdateFrequency(bool force_update, enum UpdateFrequencyMode_t mode) {
	uint32_t		            dial_freq;
	Oscillator_ResultCodes_t    lo_result = OSC_OK;
	bool                        lo_change_not_pending = true;
    VfoReg*                     vfo_store = &vfo[is_vfo_b()? VFO_A : VFO_B].band[ts.band->band_mode];

    switch (mode) {
        case UFM_LARGE_B:
            dial_freq = vfo_store->dial_value;
            lo_result = ts.last_lo_result;
            break;

        case UFM_SMALL_TX:
            dial_freq = RadioManagement_GetTXDialFrequency();
            lo_result = RadioManagement_ValidateFrequencyForTX(dial_freq);
            break;

        case UFM_SMALL_RX:
            dial_freq = RadioManagement_GetRXDialFrequency();
            lo_result = ts.last_lo_result;
            break;

        default:
            dial_freq = df.tune_new;
            lo_change_not_pending = RadioManagement_ChangeFrequency(force_update, dial_freq, ts.txrx_mode);
            lo_result = ts.last_lo_result;
    }

	// ALL UI CODE BELOW
	{
		uint32_t clr;

		if (lo_change_not_pending) {
			if (mode != UFM_SMALL_TX && mode != UFM_LARGE_B) {
				UiDriver_DisplayBandForFreq(dial_freq, false);
				// check which band in which we are currently tuning and update the display

				UiDriver_UpdateLcdFreq(RadioManagement_GetRXDialFrequency(), sd.txt_colour, UFM_SECONDARY);
				// set mode parameter to UFM_SECONDARY to update secondary display (it shows real RX frequency if RIT is being used)
				// color argument is not being used by secondary display
			}

			switch(lo_result) {
			case OSC_TUNE_IMPOSSIBLE:
				clr = Orange; // Color in orange if there was a problem setting frequency
				break;
			case OSC_TUNE_LIMITED:
				clr = Yellow; // Color in yellow if there was a problem setting frequency exactly
				break;
			case OSC_LARGE_STEP:
			case OSC_OK:
				clr = sd.txt_colour;
				break;
			default:
				clr = Red; // a serious error happened, i.e. I2C not working etc.
			}
		} else {
			// we did not execute the change, so we show the freq in Blue.
			// this will turn into the appropriate color the moment the tuning
			// happens.
			// Use white in releases, many complained about the  Blue digits
			clr = sd.txt_colour; // Blue;
		}

		// Update frequency display
		UiDriver_UpdateLcdFreq(dial_freq, clr, mode);
	}
}

static void UiDriver_UpdateFreqDisplay(uint64_t dial_freq, uint32_t pos_x_loc, uint32_t pos_y_loc, uint16_t color, uint16_t bg_color, uint8_t digit_font) {
	uint8_t MAX_DIGITS_T = MAX_DIGITS; // 11

	if (!(ts.xverter_mode & 0xf)) {
	    MAX_DIGITS_T = 9;
	}

    uint8_t digits[MAX_DIGITS_T];
    uint8_t last_non_zero = 0;
    const uint8_t font_width = UiLcdHy28_TextWidth("0", digit_font);

    // Terminate string for digit
    char digit[2];
    digit[1] = 0;

    // calculate the digits
    uint64_t dial_freq_temp = dial_freq;

    for (uint32_t idx = 0; idx < MAX_DIGITS_T; idx++) {
        digits[idx] = dial_freq_temp % 10;
        dial_freq_temp /= 10;
        if (digits[idx] != 0) last_non_zero = idx;
    }

    const uint16_t group_space = 3* font_width + font_width/2;
    const uint16_t x_right = pos_x_loc + (9* font_width);

    for (uint32_t idx = 3; idx < MAX_DIGITS_T; idx+=3) {
        bool noshow = last_non_zero < idx;
        int digit_group = idx / 3; // we group every 3 digits

        digit[0] = '.';
        uint32_t dot_clr = noshow ? bg_color : color;

        if (digit_font != 5) {
            UiLcdHy28_PrintText(
                x_right - (digit_group * group_space) + font_width/2 + 1,
                pos_y_loc,
                digit,
                dot_clr, bg_color,
                digit_font
             );
        } else {
            UiLcdHy28_PrintText(
                x_right - (digit_group * group_space) + font_width,
                pos_y_loc,
                digit,
                dot_clr, bg_color,
                digit_font
             );
        }
    }

    for (uint32_t idx = 0; idx < MAX_DIGITS_T; idx++) {
        int digit_group = idx / 3; // we group every 3 digits
        int digit_idx = idx % 3;

        bool noshow = idx > last_non_zero;
        // don't show leading zeros, except for the 0th digits
        digit[0] = noshow ? '0' : 0x30 + (digits[idx] & 0x0F);
        uint32_t digit_clr = noshow ? bg_color : color;

        // Update segment
        UiLcdHy28_PrintText(
            x_right -  digit_idx * font_width - (digit_group * group_space),
            pos_y_loc,
            digit,
            digit_clr, bg_color,
            digit_font
        );
    }
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverUpdateLcdFreq
//* Object              : this function will split LCD freq display control
//* Object              : and update as it is 7 segments indicator
//* Input Parameters    : freq=freq (Hz), color=color, mode: 0 = auto, 1= force normal (large digits), 2= force upper, small, 3 = force lower, small, 4 = secondary display
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void UiDriver_UpdateLcdFreq(uint32_t dial_freq, uint16_t color, uint16_t mode) {
    uint16_t bg_color = Black;

	if (ts.frequency_lock) {
		// Frequency is locked - change color of display
		color = sd.txt_colour_dark;
	}

	if (mode == UFM_AUTOMATIC) {
	    mode = is_splitmode() ? UFM_SMALL_RX : UFM_LARGE_A;
		// in "split" mode?
		// yes - update upper, small digits (receive frequency)
		// no  - large, normal-sized digits
	}

    uint64_t disp_freq;

    // if in transverter mode, main display shows translated frequencies  in blue2
    // the secondary display will always show the real untranslated frequency

    if (RadioManagement_Transverter_IsEnabled() && mode != UFM_SECONDARY) {        // transverter mode active?
        uint8_t txrx_mode = (mode == UFM_LARGE_A) ?
                ts.txrx_mode
                :
                (mode == UFM_SMALL_TX ? TRX_MODE_TX : TRX_MODE_RX);

        disp_freq = RadioManagement_Transverter_GetFreq(dial_freq, txrx_mode);
        // scale by LO multiplier and add transverter frequency offset
        color = Blue2;
	} else {
	    disp_freq = dial_freq;
	}

	// Handle frequency display offset in "CW RX" modes
	if (ts.dmod_mode == DEMOD_CW) {	 		// In CW mode?
		switch(ts.cw_offset_mode) {
            case CW_OFFSET_LSB_RX:	// Yes - In an LSB mode with display offset?
            case CW_OFFSET_USB_RX:	// In a USB mode with display offset?
            case CW_OFFSET_AUTO_RX:	// in "auto" mode with display offset?
                if (ts.cw_lsb) {
                    disp_freq -= ts.cw_sidetone_freq;		// yes - LSB - lower display frequency by sidetone amount
                } else {
                    disp_freq += ts.cw_sidetone_freq;		// yes - USB - raise display frequency by sidetone amount
                }
                break;
            }
	}

    uint16_t       pos_y_loc;
    uint16_t       pos_x_loc;
    uint8_t        digit_font;

	switch (mode) {
        case UFM_SMALL_RX:
            digit_font = 0;
            pos_y_loc = ts.Layout->TUNE_FREQ.y;
            pos_x_loc = ts.Layout->TUNE_SPLIT_FREQ_X;
            break;
        case UFM_SMALL_TX:					// small digits in lower location
            digit_font = 0;
            pos_y_loc = ts.Layout->TUNE_SPLIT_FREQ_Y_TX;
            pos_x_loc = ts.Layout->TUNE_SPLIT_FREQ_X;
            break;
        case UFM_SECONDARY:
            digit_font = 4;
            bg_color = Black;
            pos_y_loc = ts.Layout->TUNE_SFREQ.y;
            pos_x_loc = ts.Layout->TUNE_SFREQ.x;
            break;
        case UFM_LARGE_B:
#ifdef USE_8bit_FONT
            digit_font=(ts.FreqDisplayFont ==0) ? 1 : 5;
#else
            digit_font = 1;
#endif
            pos_y_loc = ts.Layout->TUNE_FREQ.y;
            pos_x_loc = ts.Layout->TUNE_SPLIT_FREQ_X;
            color = sd.txt_colour_dark;
            break;
        case UFM_LARGE_A:
        default:			// default:  normal sized (large) digits
    #ifdef USE_8bit_FONT
            digit_font=(ts.FreqDisplayFont ==0) ? 1 : 5;
    #else
            digit_font = 1;
    #endif
            pos_y_loc = ts.Layout->TUNE_FREQ.y;
            pos_x_loc = ts.Layout->TUNE_FREQ.x;
	}

	// in SAM mode, never display any RIT etc., but
	// use small display for display of the carrier frequency that the PLL has locked to
	if (ts.dmod_mode == DEMOD_SAM && (mode == UFM_SMALL_RX || mode == UFM_SECONDARY)) {
		digit_font = 4;
		pos_y_loc = ts.Layout->TUNE_SFREQ.y;
		pos_x_loc = ts.Layout->TUNE_SFREQ.x;
		disp_freq += ads.carrier_freq_offset;
		color = Yellow;
	}

    UiDriver_UpdateFreqDisplay(disp_freq, pos_x_loc, pos_y_loc, color, bg_color, digit_font);
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverChangeTuningStep
//* Object              : Change tunning step
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
void UiDriver_ChangeTuningStep(uchar is_up) {
    const int8_t step = is_up ? +1 : -1;
    uint8_t mode = UiDriver_GetModeCode();
	int32_t idx = df.selected_idx[mode];
	int8_t idx_limit = T_STEP_MAX_STEPS;

	if ((!ts.xvtr_adjust_flag) && (!ts.xverter_mode)) {
		// are we NOT in "transverter adjust" or transverter mode *NOT* on?
		idx_limit = T_STEP_1MHZ_IDX;
	}

	idx = (idx + step + idx_limit) % idx_limit;

	// 9kHz step only on MW and LW, skip otherwise
	if (idx == T_STEP_9KHZ_IDX && ((df.tune_old) > 1600001)) {
			idx+= step;
	}

	df.tuning_step	= tune_steps[idx];
	df.selected_idx[mode] = idx;
	UiDriver_DisplayFreqStepSize();
}

static void UiDriver_ShowTxErrorMessages(void) {
    // if there is no power factor ( == no output power)
    // or effective bias is 0 (== no or very distorted output)
    // inform operator
//	const bool no_tx_power = ts.tx_power_factor == 0;
	const bool no_tx_power = ts.tx_power_factor == 0.0;
	const bool no_bias = (ts.dmod_mode != DEMOD_CW && ts.pa_bias == 0) || (ts.dmod_mode == DEMOD_CW && ts.pa_cw_bias == 0 && ts.pa_bias == 0);
	
    if (RadioManagement_IsTxDisabled() == false && (no_tx_power || no_bias))
    {
        UiDriver_DisplayMessageStart();
		int display_time = 3000;
		
        const uint16_t scope_middle_y = sd.Slayout->full.h/2+sd.Slayout->full.y;
        
		const char* txp = NULL;
		
		if (no_bias && no_tx_power) 
		{ 
			txp = "No TX power and no bias.\nCheck PA calibration!";
		} 
		else if (no_bias) 
		{
			txp = "PA BIAS is 0.\nAdjust Bias before TX!";
		} 
		else if (no_tx_power)
		{
			txp = "PA TX Power Factor is 0.\nAdjust TX power for band!";
		}
		
		if(ts.menu_mode)
		{
			// we assume tune is used for power calibration when in menu mode
			if (ts.tune)
			{
				// we disable display of error message if bias is set.
				if (no_bias == false)
				{
					txp = NULL;
				}
				else
				{
					txp = "PA BIAS is 0.\nAdjust Bias before TX!";
				}
			}
			else
			{
				// shorter display time 
				display_time = 1000;
			}
		}
    	
		if (display_time > 0 && txp != NULL)
		{
        	UiLcdHy28_PrintTextCentered(sd.Slayout->full.x, scope_middle_y-6, sd.Slayout->full.w,txp,Red,Black,0);

        	HAL_Delay(display_time);
		}
        UiDriver_DisplayMessageStop();
    }
    else if(ts.expflags1 & EXPFLAGS1_CLEAR_PAN_ON_TX)
    {
    	UiSpectrum_Clear();
//    	UiLcdHy28_PrintText(140, 160, "TX", Red, Black, 5);
    	UiLcdHy28_PrintTextCentered(sd.Slayout->full.x, 160, sd.Slayout->full.w,"[TX]",Red,Black,1);
    }
}

#ifdef SDR_AMBER
static void UiDriver_ShowGreeting(uint8_t variant)
{
    UiDriver_DisplayMessageStart();
    int display_time = 6000;
    const uint16_t scope_middle_y = sd.Slayout->full.h/2+sd.Slayout->full.y-18;
    const char* txp = NULL;

    if (variant == 0)
    {
        txp = "Hello, I'm Amber.\nNice to meet and work for you.\n***\nFrom Russia with love";
    }
    else if (variant == 1)
    {
        txp = "Hello, I'm Axis.\nNice to meet and work for you.\n***\nFrom Russia with love";
    }

    if (display_time > 0 && txp != NULL)
    {
        UiLcdHy28_PrintTextCentered(sd.Slayout->full.x, scope_middle_y-6, sd.Slayout->full.w,txp,Magenta,Black,0);

        HAL_Delay(display_time);
    }
    UiDriver_DisplayMessageStop();
}
#endif

/*----------------------------------------------------------------------------
 * @brief Scans buttons 0-16:  0-15 are normal buttons, 16 is power button, 17 touch
 * @param button_num - 0-BUTTON_NUM - 1
 * @returns true if button is pressed
 */

static bool UiDriver_IsButtonPressed(uint32_t button_num)
{
	// FIXME: This is fragile code, as it depends on being called multiple times in short periods (ms)
	// This works, since regularily the button matrix is queried.
	UiLcdHy28_TouchscreenDetectPress();
	return Keypad_IsKeyPressed(button_num);
}

static void UiDriver_WaitForButtonPressed(uint32_t button_num)
{
    while (true)
    {
        Keypad_Scan();
        if (UiDriver_IsButtonPressed(button_num))
        {
            break;
        }
        HAL_Delay(20);
    }
}

void UiDriver_KeyboardProcessOldClicks()
{
    static uchar press_hold_release_delay = 0;

    Keypad_Scan(); // update all key states

    // State machine - processing old click
    if(ks.button_processed == false)
    {
        // State machine - click or release(debounce filter)
        if(ks.button_pressed == false)
        {
            // Scan logical button inputs (the result of the HW keypad scan)
            // this algorithm favors the pressed button with the lowest number
            for(int i = 0; i < BUTTON_NUM; i++)
            {
                if(UiDriver_IsButtonPressed(i))
                {
                    // Change state to clicked
                    ks.button_id      = i;
                    ks.button_pressed = true;
                    ks.button_released    = 0;
                    ks.button_just_pressed    = 0;
                    ks.debounce_time  = 0;
                    ks.debounce_check_complete    = 0;
                    ks.press_hold         = 0;
                    //printf("button_pressed %02x\n\r",ks.button_id);
                    // Exit, we process just one click at a time
                    break;
                }
            }
        }
        else if((ks.debounce_time >= BUTTON_PRESS_DEBOUNCE) && (!ks.debounce_check_complete))
        {
            if(UiDriver_IsButtonPressed(ks.button_id))        // button still pressed?
            {
                ks.button_just_pressed = 1; // yes!
                ks.debounce_check_complete = 1; // indicate that the debounce check was completed
            }
            else
            {
                ks.button_pressed = false;          // debounce incomplete, button released - cancel detection
            }
        }
        else if((ks.debounce_time >= BUTTON_HOLD_TIME) && (!ks.press_hold))     // press-and-hold processing
        {
            ks.button_processed = 1;                      // indicate that a button was processed
            ks.button_just_pressed = 0;                   // clear this flag so that the release (below) won't be detected
            ks.press_hold = 1;
            press_hold_release_delay = PRESS_HOLD_RELEASE_DELAY_TIME; // Set up a bit of delay for when press-and-hold is released
        }
        else if(ks.press_hold && (!UiDriver_IsButtonPressed(ks.button_id)))     // was there a press-and-hold and the button is now released?
        {
            if(press_hold_release_delay)                  // press-and-hold delay expired?
            {
                press_hold_release_delay--;                 // no - continue counting down before cancelling "press-and-hold" mode
            }
            else                              // Press-and-hold mode time expired!
            {
                ks.button_pressed = 0;          // reset and exit press-and-hold mode, this to prevent extraneous button-presses when using multiple buttons
                ks.button_released = 0;
                ks.press_hold = 0;
                ks.button_just_pressed = 0;
            }
        }
        else if(!UiDriver_IsButtonPressed(ks.button_id) && (!ks.press_hold))        // button released and had been debounced?
        {
            // Change state from click to released, and processing flag on - if the button had been held down adequately
            ks.button_pressed     = 0;
            ks.button_released    = 1;
            ks.button_processed   = 1;
            ks.button_just_pressed = 0;
            //printf("button_released %02x\n\r",ks.button_id);
        }
        //
        // Handle press-and-hold tuning step adjustment
        //
        if((ts.tune_step != STEP_PRESS_OFF) && (!ks.press_hold))     // are we in press-and-hold step size mode and did the button get released?
        {
            uint8_t mode = UiDriver_GetModeCode();

            ts.tune_step = STEP_PRESS_OFF;                        // yes, cancel offset
            df.selected_idx[mode] = ts.tune_step_idx_holder;            // restore previous setting
            df.tuning_step    = tune_steps[df.selected_idx[mode]];
            UiDriver_DisplayFreqStepSize();
        }
    }
}



enum TRX_States_t
{
	TRX_STATE_TX_TO_RX,
	TRX_STATE_RX,
	TRX_STATE_RX_TO_TX,
	TRX_STATE_TX,
};

static void UiDriver_TxRxUiSwitch(enum TRX_States_t state) {
    if (state == TRX_STATE_RX_TO_TX) {
        UiDriver_DeleteMeters();
        UiDriver_CreateMeters();

        // force redisplay of Encoder boxes and values
        UiDriver_RefreshEncoderDisplay();

        // if there is a need to tell the operator something related to
        // the tx mode (such as PA not configured correctly) we do this now.
        UiDriver_ShowTxErrorMessages();
    } else if (state == TRX_STATE_TX_TO_RX) {
        UiDriver_DeleteMeters();
        UiDriver_CreateMeters();

        // force redisplay of Encoder boxes and values
        UiDriver_RefreshEncoderDisplay();
        if (ts.expflags1 & EXPFLAGS1_CLEAR_PAN_ON_TX) {
            if(!ts.menu_mode) {
                UiSpectrum_Init();
            }
        }
    }

    if ((ts.menu_mode)) {              // update menu when we are (or WERE) in MENU mode
		UiMenu_RenderMenu(MENU_RENDER_ONLY);
	}
}

/**
 * adjust volume and return to RX from TX and other time-related functions,
 * has to be called regularly
 */
static void UiDriver_TimeScheduler()
{
	static bool	 audio_spkr_volume_update_request = true;
	static bool  audio_spkr_delayed_unmute_active = false;

	static bool old_squelch = 0;	// used to detect change-of-state of squelch
	static bool old_tone_det = 0;	// used to detect change-of-state of tone decoder
	static bool old_tone_det_enable = 0;	// used to detect change-of-state of tone decoder enabling
	static bool old_burst_active = 0;		// used to detect state of change of tone burst generator
	static bool startup_done_flag = 0;
	static enum TRX_States_t last_state = TRX_STATE_RX; // we assume everything is
	enum TRX_States_t state;



	// let us figure out if we are in a stable state or if this
	// is the first run after a mode change
	if (ts.txrx_mode == TRX_MODE_TX)
	{
		if (last_state != TRX_STATE_TX)
		{
			state = TRX_STATE_RX_TO_TX;
		}
		else
		{
			state = TRX_STATE_TX;
		}
		last_state = TRX_STATE_TX;
	}
	else
	{
		if (last_state != TRX_STATE_RX)
		{
			state = TRX_STATE_TX_TO_RX;
		}
		else
		{
			state = TRX_STATE_RX;
		}
		last_state = TRX_STATE_RX;
	}


	/*** RX MODE ***/
	if(ts.txrx_mode == TRX_MODE_RX)
	{
		if (state == TRX_STATE_TX_TO_RX)
		{
			audio_spkr_delayed_unmute_active = true;
		}

		if(audio_spkr_delayed_unmute_active  && ts.audio_spkr_unmute_delay_count == 0)	 	// did timer hit zero
		{
			audio_spkr_delayed_unmute_active = false;
			audio_spkr_volume_update_request = true;
		}



		audio_spkr_volume_update_request |= ts.rx_gain[RX_AUDIO_SPKR].value != ts.rx_gain[RX_AUDIO_SPKR].value_old;


		if( audio_spkr_volume_update_request)	 	// in normal mode - calculate volume normally
		{

			ts.rx_gain[RX_AUDIO_SPKR].value_old = ts.rx_gain[RX_AUDIO_SPKR].value;
			ts.rx_gain[RX_AUDIO_SPKR].active_value = 1;		// software gain not active - set to unity
			if(ts.rx_gain[RX_AUDIO_SPKR].value <= CODEC_SPEAKER_MAX_VOLUME)  				// Note:  Gain > 16 adjusted in audio_driver.c via software
			{
				Codec_VolumeSpkr((ts.rx_gain[RX_AUDIO_SPKR].value));

			}
			else  	// are we in the "software amplification" range?
			{
				Codec_VolumeSpkr(CODEC_SPEAKER_MAX_VOLUME);		// set to fixed "maximum" gain
				ts.rx_gain[RX_AUDIO_SPKR].active_value = (((float32_t)ts.rx_gain[RX_AUDIO_SPKR].value)/2.5)-5.35;	// to float
			}

			audio_spkr_volume_update_request = false;
		}

		// update the on-screen indicator of squelch/tone detection (the "FM" mode text) if there is a change of state of squelch/tone detection
		if((old_squelch != ads.fm_conf.squelched)
				|| (old_tone_det != ads.fm_conf.subaudible_tone_detected)
				|| (old_tone_det_enable != (bool)ts.fm_subaudible_tone_det_select))       // did the squelch or tone detect state just change?
		{

			UiDriver_DisplayDemodMode();                           // yes - update on-screen indicator to show that squelch is open/closed
			old_squelch = ads.fm_conf.squelched;
			old_tone_det = ads.fm_conf.subaudible_tone_detected;
			old_tone_det_enable = (bool)ts.fm_subaudible_tone_det_select;
		}
	}

	/*** TX MODE ONLY ***/
	if(ts.txrx_mode == TRX_MODE_TX)
	{

		if((state == TRX_STATE_RX_TO_TX))
		{
			// we just switched to TX
			if((ts.dmod_mode != DEMOD_CW))        // did we just enter TX mode in voice mode?
			{
				ads.alc_val = 1;    // re-init AGC value
				ads.peak_audio = 0; // clear peak reading of audio meter
			}
		}


		// Has the timing for the tone burst expired?
		if(ts.sysclock > ts.fm_tone_burst_timing)
		{
			ads.fm_conf.tone_burst_active = 0;               // yes, turn the tone off
		}

		if(ads.fm_conf.tone_burst_active != old_burst_active)       // did the squelch or tone detect state just change?
		{
			UiDriver_DisplayDemodMode();                           // yes - update on-screen indicator to show that tone burst is on/off
			old_burst_active = ads.fm_conf.tone_burst_active;
		}
	}

	/*** TX+RX STATE CHANGE ONLY ***/
	// if we do change modes, some visuals need an update
	if(state == TRX_STATE_RX_TO_TX || state == TRX_STATE_TX_TO_RX)
	{
		// now update display according to the changed state
		UiDriver_TxRxUiSwitch(state);
	}

	/*** ALWAYS ***/
	UiDriver_LcdBlankingProcessTimer();

	// This delays the start-up of the DSP for several seconds to minimize the likelihood that the LMS function will get "jammed"
	// and stop working.

	/*** ONCE AFTER STARTUP DELAY ***/
	if( startup_done_flag == false && (ts.sysclock > DSP_STARTUP_DELAY))       // has it been long enough after startup?
	{
		startup_done_flag = true;                  // set flag so that we do this only once

		UiDriver_DisplayDSPMode();

		audio_spkr_volume_update_request = 1;      // set unmute flag to force audio to be un-muted - just in case it starts up muted!
		Codec_MuteDAC(false);                      // make sure that audio is un-muted
	}
}


/*
 * Tells you which SSB Demod Mode is the preferred one for a given frequency in Hertz
 */
//*----------------------------------------------------------------------------
//* Function Name       : UiDriverChangeDemodMode
//* Object              : change demodulator mode
//* Input Parameters    : "noskip", if TRUE, disabled modes are to be included
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------

/**
 * This function is responsible for make the changes to the UI layout
 * as required for a give new mode, such as enabling the right set of encoder boxes etc.
 */
typedef struct
{
	int16_t encoder_modes[3];
} encoder_mode_store_t;

void UiDriver_SetDemodMode(uint8_t new_mode)
{
	RadioManagement_SetDemodMode(new_mode);

	DigiModes_TxBufferReset();

	switch(ts.dmod_mode) {
	    case DEMOD_DIGI:
	        switch(ts.digital_mode) {
	            case DigitalMode_RTTY:
	                DigiModes_Set_BufferConsumer( RTTY );
	                break;

	            case DigitalMode_BPSK:
	                DigiModes_Set_BufferConsumer( BPSK );
	                break;
	        }
	        break;

	    case DEMOD_CW:
	        DigiModes_Set_BufferConsumer( CW );
	        break;

	    default:
	        break;
	}
	UiDriver_UpdateDisplayAfterParamChange();
}

static void UiDriver_ChangeToNextDemodMode(bool select_alternative_mode)
{
	uint8_t new_mode = ts.dmod_mode;	// copy to local, so IRQ is not affected
	if (select_alternative_mode)
	{
		new_mode = RadioManagement_NextAlternativeDemodMode(new_mode);
	}
	else
	{
		new_mode = RadioManagement_NextNormalDemodMode(new_mode);
	}

	// TODO: We call this always, since we may have switched sidebands or the digital mode
	// if we would remember that, we would decide if to call this.
	UiDriver_SetDemodMode(new_mode);
}

/**
 * @brief band memory switch. Sets VFO to use the values of specified band memory. Does not store "old" values.
 *
 * @param vfo_sel	which VFO A/B to use
 * @param new_band_index
 */
void UiDriver_SelectBandMemory(uint16_t vfo_sel, uint8_t new_band_index)
{
	if (ts.expflags2 & EXPFLAGS2_TEMP_MEM_BW_MOD_BAND) // enabled temp. remember BW on mode & band
	{
		UiDriver_StoreBwData();
	}

		// TODO: There is a strong similarity to code in UiDriverProcessFunctionKeyClick around line 2053
		df.tune_new = vfo[vfo_sel].band[new_band_index].dial_value;	// Load value from VFO
		ts.iq_freq_delta = vfo[vfo_sel].band[new_band_index].dial_delta;

		bool new_lsb = RadioManagement_CalculateCWSidebandMode();

		uint16_t new_dmod_mode = vfo[vfo_sel].band[new_band_index].decod_mode;
		uint16_t new_digital_mode = vfo[vfo_sel].band[new_band_index].digital_mode;

		bool isNewDigitalMode = ts.digital_mode != new_digital_mode && new_dmod_mode == DEMOD_DIGI;


		// we need to mute here since changing bands may cause audible click/pops
		RadioManagement_MuteTemporarilyRxAudio();

		ts.digital_mode = new_digital_mode;

		if(ts.dmod_mode != new_dmod_mode || (new_dmod_mode == DEMOD_CW && ts.cw_lsb != new_lsb) || isNewDigitalMode)
		{
			// Update mode
			ts.cw_lsb = new_lsb;
			RadioManagement_SetDemodMode(new_dmod_mode);
		}

		// Finally update public flag by setting the
		// appropriate bandInfo
		ts.band = RadioManagement_GetBandInfo(new_band_index);

		if (ts.expflags2 & EXPFLAGS2_TEMP_MEM_BW_MOD_BAND) // enabled temp. remember BW on mode & band
		{
			UiDriver_LoadBwData();
		}

		UiDriver_UpdateDisplayAfterParamChange();    // because mode/filter may have changed
		UiVk_Redraw();		//virtual keypads call (refresh purpose)
}

/**
 * @brief initiate band change.
 * @param is_up select the next higher band, otherwise go to the next lower band
 */
static void UiDriver_ChangeBand(bool is_up)
{

	// Do not allow band change during TX
	if(ts.txrx_mode != TRX_MODE_TX)
	{
//		if (ts.expflags2 & EXPFLAGS2_TEMP_MEM_BW_MOD_BAND) // enabled temp. remember BW on mode & band
//		{
//			UiDriver_StoreBwData();
//		}

		vfo_name_t vfo_sel = get_active_vfo();

		uint8_t curr_band_index = ts.band->band_mode; // index in band table of currently selected band


		// Save old band values
		if(curr_band_index < (MAX_BANDS) && ts.cat_band_index == 255)
		{
			// Save dial, but only if we are not in "CAT mode"
			vfo[vfo_sel].band[curr_band_index].dial_value = df.tune_new;
			vfo[vfo_sel].band[curr_band_index].decod_mode = ts.dmod_mode;
			vfo[vfo_sel].band[curr_band_index].digital_mode = ts.digital_mode;
			vfo[vfo_sel].band[curr_band_index].dial_delta = ts.iq_freq_delta;
		}
		else
		{
			ts.cat_band_index = 255;
		}

		uint8_t   new_band_index = curr_band_index;     // index of the new selected band
		// in case of no other band enabled, we stay in this band

		// we start checking the index following (is_up) or preceding (!is_up) the current one
		// until we reach an enabled band
		for (int idx  = 1; idx <= MAX_BANDS; idx++)
		{
		    uint32_t test_idx = (curr_band_index + ((is_up == true) ? idx : (MAX_BANDS-idx)))% MAX_BANDS;
#ifndef USE_MEMORY_MODE
		    if (band_enabled[test_idx])
#endif
		    {
		        new_band_index = test_idx;
		        break; // we found the first enabled band following the current one
		    }
		}

		UiDriver_SelectBandMemory(vfo_sel, new_band_index);
	}
}


/**
 * @brief Read out the changes in the frequency encoder and initiate frequency change by setting a global variable.
 *
 * @returns true if a frequency change was detected and a new tuning frequency was set in a global variable.
 */
static bool UiDriver_CheckFrequencyEncoder()
{
	int 		pot_diff;
	bool		retval = false;
	int		enc_multiplier;
	static float 	enc_speed_avg = 0.0;  //keeps the averaged encoder speed
	int		delta_t, enc_speed;

	pot_diff = UiDriverEncoderRead(ENCFREQ);


	if (pot_diff != 0)
	{
		delta_t = ts.audio_int_counter;  // get ticker difference since last enc. change
		ts.audio_int_counter = 0;		 //reset tick counter

		UiDriver_LcdBlankingStartTimer();	// calculate/process LCD blanking timing

	}
	if (pot_diff != 0 &&
			ts.txrx_mode == TRX_MODE_RX
			&& ks.button_just_pressed == false
			&& (ts.frequency_lock == false || ts.iq_freq_mode == FREQ_IQ_CONV_SLIDE))
	{
		// allow tuning only if in rx mode and allow freq lock only in RX Slide mode,

#ifdef FAST_FREQ_ENC
	    delta_t *= 0.2; // pooling interval 2 ms instead of 10 ms
	    if (delta_t > 60)
#else
		if (delta_t > 300)
#endif
		{
			enc_speed_avg = 0;    //when leaving speedy turning set avg_speed to 0
		}

		enc_speed = div(4000,delta_t).quot*pot_diff;  // app. 4000 tics per second -> calc. enc. speed.

		if (enc_speed > 500)
		{
			enc_speed = 500;    //limit calculated enc. speed
		}
		if (enc_speed < -500)
		{
			enc_speed = -500;
		}

		enc_speed_avg = 0.1*enc_speed + 0.9*enc_speed_avg; // averaging to smooth encoder speed

		enc_multiplier = 1; //set standard speed

		if (ts.flags1 & FLAGS1_DYN_TUNE_ENABLE)   // check if dynamic tuning has been activated by touchscreen
		{
		    if (!(ts.expflags1 & EXPFLAGS1_SMOOTH_DYNAMIC_TUNE))        // Smooth dynamic tune is OFF
			{
				if ((enc_speed_avg > 80) || (enc_speed_avg < (-80)))
				{
					enc_multiplier = 10;    // turning medium speed -> increase speed by 10
				}

				if ((enc_speed_avg > 160) || (enc_speed_avg < (-160)))
				{
					enc_multiplier = 40;    //turning fast speed -> increase speed by 100
				}

				if ((enc_speed_avg > 300) || (enc_speed_avg < (-300)))
				{
					enc_multiplier = 100;    //turning fast speed -> increase speed by 100
				}
			}
            else
            {
                if      ((enc_speed_avg > 350) || (enc_speed_avg < (-350)))
                {
                    enc_multiplier = 100;    // turning medium speed -> increase speed by 100
                }
                else if ((enc_speed_avg > 250) || (enc_speed_avg < (-250)))
                {
                    enc_multiplier =  50;    //turning fast speed -> increase speed by 50
                }
                else if ((enc_speed_avg > 180) || (enc_speed_avg < (-180)))
                {
                    enc_multiplier =  12;    //turning fast speed -> increase speed by 12
                }
                else if ((enc_speed_avg >  90) || (enc_speed_avg < (- 90)))
                {
                    enc_multiplier =   6;    //turning fast speed -> increase speed by 6
                }
                else if ((enc_speed_avg >  45) || (enc_speed_avg < (- 45)))
                {
                    enc_multiplier =   3;    //turning fast speed -> increase speed by 3
                }
                else if ((enc_speed_avg >  30) || (enc_speed_avg < (- 30)))
                {
                    enc_multiplier =   2;    //turning fast speed -> increase speed by 2
                }
			}

			if ((df.tuning_step == 10000) && (enc_multiplier > 10))
			{
				enc_multiplier = 10;    //limit speed to 100000kHz/step
			}
			if ((df.tuning_step == 100000) && (enc_multiplier > 1))
			{
				enc_multiplier = 1;    //limit speed to 100000kHz/step
			}
		}


		// Finally convert to frequency incr/decr

		int32_t tune_step = df.tuning_step * enc_multiplier;

		if (ts.iq_freq_mode == FREQ_IQ_CONV_SLIDE) {
		    int32_t    freq_delta = ts.iq_freq_delta;
		    int32_t    max = 18000 / (1 << sd.magnify);

		    if (ts.frequency_lock) {
		        if (pot_diff>0) {
                    freq_delta += tune_step;

                    if (freq_delta < max) {
                        ts.iq_freq_delta = freq_delta;
                        df.tune_old += tune_step;
                    }
		        } else {
                    freq_delta -= tune_step;

                    if (freq_delta > -max) {
                        ts.iq_freq_delta = freq_delta;
                        df.tune_old -= tune_step;
                    }
		        }
		    } else {
                if (pot_diff>0) {
                    freq_delta -= tune_step;

                    if (freq_delta < -max) {
                        freq_delta = -max;
                    }
                    df.tune_new -= df.tune_new % df.tuning_step;
                    df.tune_new += tune_step;
                } else {
                    freq_delta += tune_step;

                    if (freq_delta > max) {
                        freq_delta = max;
                    }
                    df.tune_new -= df.tune_new % df.tuning_step;
                    df.tune_new -= tune_step;
                }

                ts.iq_freq_delta = freq_delta;
		    }
		} else {
            if(pot_diff>0)
            {
                df.tune_new += tune_step;
                //itoa(enc_speed,num,6);
                //UiSpectrumClearDisplay();			// clear display under spectrum scope
                //UiLcdHy28_PrintText(110,156,num,Cyan,Black,0);
            }
            else
            {
                df.tune_new -= tune_step;
            }

            if (enc_multiplier != 1)
            {
                df.tune_new = enc_multiplier*df.tuning_step * div((df.tune_new),enc_multiplier*df.tuning_step).quot;    // keep last digit to zero
            }
		}

		retval = true;
	}
	return retval;
}


static void UiDriver_RotateNormalEncoder(int8_t pot_diff, uint8_t enc) {
    int8_t  pot_diff_step = (pot_diff < 0) ? -1 : 1;
    const   BandInfo* band = RadioManagement_GetBand(df.tune_old);
    uint8_t mode = ts.enc_mode[enc][ts.enc_state[enc] & ENC_MODE_MASK];

    // used for notch and peak
    float32_t MAX_FREQ = 5000.0;

    if (ts.filters_p->sample_rate_dec == RX_DECIMATION_RATE_24KHZ) {
        MAX_FREQ = 10000.0;
    } else if (ts.filters_p->sample_rate_dec == RX_DECIMATION_RATE_12KHZ) {
        MAX_FREQ = 5000.0;
    }

    if (filter_path_change && enc == ENC3) {
        AudioFilter_NextApplicableFilterPath(PATH_ALL_APPLICABLE | (pot_diff < 0 ? PATH_DOWN:PATH_UP), AudioFilter_GetFilterModeFromDemodMode(ts.dmod_mode), ts.filter_path);
        AudioDriver_SetProcessingChain(ts.dmod_mode, false);
        UiDriver_UpdateDisplayAfterParamChange();
        return;
    }

    switch(mode) {
        case ENC_MODE_AUDIO_GAIN:
            ts.rx_gain[RX_AUDIO_SPKR].value = change_and_limit_uint(ts.rx_gain[RX_AUDIO_SPKR].value,pot_diff_step,0,ts.rx_gain[RX_AUDIO_SPKR].max);
            UiDriver_DisplayAfGain(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_RF_GAIN:
            agc_wdsp_conf.thresh = change_and_limit_int(agc_wdsp_conf.thresh,pot_diff_step,-20,120);
            AudioDriver_AgcWdsp_Set();
            UiDriver_DisplayRfGain(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_SQL:
            ts.fm_sql_threshold = change_and_limit_uint(ts.fm_sql_threshold,pot_diff_step,0,FM_SQUELCH_MAX);
            UiDriver_DisplaySQL(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_RIT:
            if (ts.txrx_mode == TRX_MODE_RX) {
                int16_t old_rit_value = ts.rit_value;

                ts.rit_value = change_and_limit_int(ts.rit_value,pot_diff_step,MIN_RIT_VALUE,MAX_RIT_VALUE);
                ts.dial_moved = ts.rit_value != old_rit_value;

                UiDriver_DisplayRit(enc, ENC_STATE_NORM);
                UiDriver_FrequencyUpdateLOandDisplay(false);
            }
            break;

        case ENC_MODE_PWR_GAIN:
            if (ts.band_index != 17) { // not GEN-band
                ts.pwr_adj[ts.power_level == PA_LEVEL_FULL?ADJ_FULL_POWER:ADJ_REF_PWR][band->band_mode] = change_and_limit_uint(ts.pwr_adj[ts.power_level == PA_LEVEL_FULL?ADJ_FULL_POWER:ADJ_REF_PWR][band->band_mode],pot_diff_step,3,RadioManagement_IsPowerFactorReduce(df.tune_old)?TX_POWER_FACTOR_MAX:TX_POWER_FACTOR_MAX/4);
            } else { // GEN-band
                if((ts.power_level == PA_LEVEL_FULL) && RadioManagement_CBFullPwrEnabled()) {
                    ts.power_scale_gen_full = change_and_limit_uint(ts.power_scale_gen_full,pot_diff_step,3,250);
                } else {
                    ts.power_scale_gen = change_and_limit_uint(ts.power_scale_gen,pot_diff_step,3,200);
                }
            }

            RadioManagement_SetPowerLevel(band, ts.power_level);
            UiDriver_DisplayPWRGain(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_RTTY_SPEED:
            rtty_ctrl_config.speed_idx = change_and_limit_int(rtty_ctrl_config.speed_idx,pot_diff_step,0,RTTY_SPEED_NUM-1);
            Rtty_Modem_Init(ts.samp_rate);
            UiDriver_DisplayRttySpeed(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_ST_GAIN:
            ts.cw_sidetone_gain = change_and_limit_uint(ts.cw_sidetone_gain,pot_diff_step,0,SIDETONE_MAX_GAIN);

            // we only set a side tone if it would have an effect in the current mode
            // TODO: Should we even disable changes or at least display the box differently if  side tone is not support in currently
            // active mode?
            if (RadioManagement_UsesTxSidetone()) {
                Codec_TxSidetoneSetgain(ts.txrx_mode);
            }
            UiDriver_DisplaySidetoneGain(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_CMP_LEVEL:
            ts.tx_comp_level = change_and_limit_int(ts.tx_comp_level,pot_diff_step,TX_AUDIO_COMPRESSION_MIN,TX_AUDIO_COMPRESSION_MAX);
            AudioManagement_CalcTxCompLevel();                  // calculate values for selection compression level
            UiDriver_DisplayCmpLevel(enc, ENC_STATE_NORM);      // update on-screen display
            break;

        case ENC_MODE_BASS_GAIN:
            ts.dsp.bass_gain = change_and_limit_int(ts.dsp.bass_gain,pot_diff_step,MIN_BASS,MAX_BASS);
            AudioDriver_SetProcessingChain(ts.dmod_mode, false);
            UiDriver_DisplayBas(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_TREBLE_GAIN:
            ts.dsp.treble_gain = change_and_limit_int(ts.dsp.treble_gain,pot_diff_step,MIN_TREBLE,MAX_TREBLE);
            AudioDriver_SetProcessingChain(ts.dmod_mode, false);
            UiDriver_DisplayTreble(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_RTTY_SHIFT:
            rtty_ctrl_config.shift_idx = change_and_limit_int(rtty_ctrl_config.shift_idx,pot_diff_step,0,RTTY_SHIFT_NUM-1);
            Rtty_Modem_Init(ts.samp_rate);
            UiDriver_DisplayRttyShift(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_CW_SPEED:
            ts.cw_keyer_speed = change_and_limit_int(ts.cw_keyer_speed, pot_diff_step, CW_KEYER_SPEED_MIN, CW_KEYER_SPEED_MAX);
            CwGen_SetSpeed();
            ts.cw_keyer_speed_bak = ts.cw_keyer_speed;
            UiDriver_DisplayKeyerSpeed(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_PSK_SPEED:
            psk_ctrl_config.speed_idx = change_and_limit_int(psk_ctrl_config.speed_idx,pot_diff_step,0,PSK_SPEED_NUM-1);
            UiDriver_TextMsgClear();
            Psk_Modem_Init(ts.samp_rate);
            UiDriver_DisplayPskSpeed(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_NB:
            // Signal processor setting
            // this is AGC setting OR noise blanker setting
            ts.dsp.nb_setting = change_and_limit_uint(ts.dsp.nb_setting,pot_diff_step,0,MAX_NB_SETTING);
            UiDriver_DisplayNoiseBlanker(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_AGC:
            agc_wdsp_conf.mode = change_and_limit_uint(agc_wdsp_conf.mode, pot_diff_step, 0, 5);
            agc_wdsp_conf.switch_mode = 1; // set flag, so that mode switching really takes place in AGC_prep
            AudioDriver_AgcWdsp_Set();
            UiDriver_DisplayAGC(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_NR:
            ts.dsp.nr_strength = change_and_limit_uint(
                    ts.dsp.nr_strength,
                    pot_diff_step * (ts.dsp.nr_strength >= 190 || ts.dsp.nr_strength <= 10 ? 1 : DSP_NR_STRENGTH_STEP),
                    DSP_NR_STRENGTH_MIN, DSP_NR_STRENGTH_MAX
            );

            if(ts.dsp.nr_strength == 189) {
                ts.dsp.nr_strength = 185;
            }

            if(ts.dsp.nr_strength == 11) {
                ts.dsp.nr_strength = 15;
            }

            // this causes considerable noise
            //AudioDriver_SetRxAudioProcessing(ts.dmod_mode, false);
            // we do this instead
            nr_params.alpha = 0.799 + ((float32_t)ts.dsp.nr_strength / 1000.0);
            UiDriver_DisplayNR(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_NOTCH_F:
            ts.dsp.notch_frequency = ts.dsp.notch_frequency + 5.0 * pot_diff_step;

            if(ts.dsp.notch_frequency > MAX_FREQ) {
                ts.dsp.notch_frequency = MAX_FREQ;
            }

            if(ts.dsp.notch_frequency < MIN_PEAK_NOTCH_FREQ) {
                ts.dsp.notch_frequency = MIN_PEAK_NOTCH_FREQ;
            }

            AudioDriver_SetProcessingChain(ts.dmod_mode, false);
            UiDriver_DisplayNotch(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_PEAK_F:
            ts.dsp.peak_frequency = ts.dsp.peak_frequency + 5.0 * pot_diff_step;

            if(ts.dsp.peak_frequency > MAX_FREQ) {
                ts.dsp.peak_frequency = MAX_FREQ;
            }

            if(ts.dsp.peak_frequency < MIN_PEAK_NOTCH_FREQ) {
                ts.dsp.peak_frequency = MIN_PEAK_NOTCH_FREQ;
            }

            AudioDriver_SetProcessingChain(ts.dmod_mode, false);
            UiDriver_DisplayPeak(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_INPUT_GAIN:
            ts.tx_gain[ts.tx_audio_source] = change_and_limit_int(
                ts.tx_gain[ts.tx_audio_source],
                pot_diff_step,
                (ts.tx_audio_source == TX_AUDIO_MIC) ? MIC_GAIN_MIN : LINE_GAIN_MIN,
                (ts.tx_audio_source == TX_AUDIO_MIC) ? MIC_GAIN_MAX : LINE_GAIN_MAX
            );

            if (ts.tx_audio_source == TX_AUDIO_MIC) {
                Codec_SwitchMicTxRxMode(ts.txrx_mode);
            }
            UiDriver_DisplayInGain(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_INPUT:
            ts.tx_audio_source = change_and_limit_uint(
                ts.tx_audio_source,
                pot_diff_step,
                0, TX_AUDIO_NUM-1
            );

            UiDriver_DisplayIn(enc, ENC_STATE_NORM);
            break;

        case ENC_MODE_METER:
            ts.tx_meter_mode = change_and_limit_uint(
                ts.tx_meter_mode,
                pot_diff_step,
                0, METER_MAX-1
            );

            UiDriver_DeleteMeters();
            UiDriver_CreateMeters();
            UiDriver_DisplayMeter(enc, ENC_STATE_NORM);
            break;

#ifdef SDR_AMBER
        case ENC_MODE_PRE:
            ts.amber_input_state = change_and_limit_uint(
                ts.amber_input_state,
                pot_diff_step,
                0, 3
            );
            RadioManagement_MuteTemporarilyRxAudio();
            Board_Amber_InputStateSet(ts.amber_input_state);
            UiDriver_DisplayPre(enc, ENC_STATE_NORM);
            break;
#endif

        default:
            break;
    }
}

static void UiDriver_RotateTuneEncoder(int8_t pot_diff, uint8_t enc) {
    uint8_t state = ts.enc_state[enc] & ENC_MODE_MASK;

    ts.enc_mode[enc][state] = change_and_limit_uint(
        ts.enc_mode[enc][state],
        pot_diff < 0 ? -1 : 1,
        0, ENC_NUM_MODES-1
    );

    UiDriver_DisplayEncoderMode(enc);
}

static void UiDriver_RotateCarouselEncoder(int8_t pot_diff, uint8_t enc) {
    int8_t  state = (ts.enc_state[enc] & ENC_MODE_MASK);

    state = change_and_limit_uint(
        state,
        pot_diff < 0 ? -1 : 1,
        0, ENC_STATE_NUM-1
    );

    ts.enc_state[enc] = (ENC_STATE_CAROUSEL << ENC_STATE_BITS) | state;
    UiDriver_DisplayEncoderMode(enc);
}

static void UiDriver_RotateEncoder(int8_t pot_diff, uint8_t enc) {
    uint8_t style = ts.enc_state[enc] >> ENC_STATE_BITS;

    switch (style) {
        case ENC_STATE_NORM:
            UiDriver_RotateNormalEncoder(pot_diff, enc);
            break;

        case ENC_STATE_CAROUSEL:
            UiDriver_RotateCarouselEncoder(pot_diff, enc);
            break;

        case ENC_STATE_TUNE_NORM:
        case ENC_STATE_TUNE_CAROUSEL:
            UiDriver_RotateTuneEncoder(pot_diff, enc);
            break;

        default:
            break;
    }
}

static void UiDriver_RotateEncoderOne(void)
{
	int32_t pot_diff = UiDriverEncoderRead(ENC1);

	if (pot_diff) {
		UiDriver_LcdBlankingStartTimer();
		UiDriver_RotateEncoder(pot_diff, ENC1);
	}
}

static void UiDriver_RotateEncoderTwo(void)
{
	int32_t pot_diff = UiDriverEncoderRead(ENC2);

	if (pot_diff != 0) {
		UiDriver_LcdBlankingStartTimer();

		if (ts.menu_mode) {
			UiMenu_RenderChangeItem(pot_diff);
		} else {
	        UiDriver_RotateEncoder(pot_diff, ENC2);
		}
	}
}

static void UiDriver_RotateEncoderThree(void)
{
	int32_t 	pot_diff = UiDriverEncoderRead(ENC3);

	if (pot_diff != 0) {
		UiDriver_LcdBlankingStartTimer();

		if (ts.menu_mode) {
			UiMenu_RenderChangeItemValue(pot_diff);
		} else {
	        UiDriver_RotateEncoder(pot_diff, ENC3);
		}
	}
}

static void UiDriver_DisplayEncoderMode(uint8_t enc) {
    uint8_t enc_mode = ts.enc_mode[enc][ts.enc_state[enc] & ENC_MODE_MASK];
    uint8_t style = ts.enc_state[enc] >> ENC_STATE_BITS;

	switch (enc_mode) {
	    case ENC_MODE_AUDIO_GAIN:
	        UiDriver_DisplayAfGain(enc, style);
	        break;

	    case ENC_MODE_RF_GAIN:
            UiDriver_DisplayRfGain(enc, style);
            break;

	    case ENC_MODE_SQL:
            UiDriver_DisplaySQL(enc, style);
            break;

	    case ENC_MODE_RIT:
            UiDriver_DisplayRit(enc, style);
            break;

	    case ENC_MODE_PWR_GAIN:
	        UiDriver_DisplayPWRGain(enc, style);
	        break;

	    case ENC_MODE_RTTY_SPEED:
            UiDriver_DisplayRttySpeed(enc, style);
            break;

	    case ENC_MODE_ST_GAIN:
            UiDriver_DisplaySidetoneGain(enc, style);
            break;

	    case ENC_MODE_CMP_LEVEL:
            UiDriver_DisplayCmpLevel(enc, style);
            break;

	    case ENC_MODE_BASS_GAIN:
            UiDriver_DisplayBas(enc, style);
            break;

	    case ENC_MODE_TREBLE_GAIN:
            UiDriver_DisplayTreble(enc, style);
            break;

	    case ENC_MODE_RTTY_SHIFT:
            UiDriver_DisplayRttyShift(enc, style);
            break;

        case ENC_MODE_CW_SPEED:
            UiDriver_DisplayKeyerSpeed(enc, style);
            break;

        case ENC_MODE_PSK_SPEED:
            UiDriver_DisplayPskSpeed(enc, style);
            break;

        case ENC_MODE_NB:
            UiDriver_DisplayNoiseBlanker(enc, style);
            break;

        case ENC_MODE_NR:
            UiDriver_DisplayNR(enc, style);
            break;

        case ENC_MODE_AGC:
            UiDriver_DisplayAGC(enc, style);
            break;

        case ENC_MODE_NOTCH_F:
            UiDriver_DisplayNotch(enc, style);
            break;

        case ENC_MODE_PEAK_F:
            UiDriver_DisplayPeak(enc, style);
            break;

	    case ENC_MODE_INPUT_GAIN:
	        UiDriver_DisplayInGain(enc, style);
	        break;

	    case ENC_MODE_INPUT:
            UiDriver_DisplayIn(enc, style);
            break;

	    case ENC_MODE_METER:
            UiDriver_DisplayMeter(enc, style);
            break;

#ifdef SDR_AMBER
        case ENC_MODE_PRE:
            UiDriver_DisplayPre(enc, style);
            break;
#endif

	    default:
	        break;
	}
}

static void UiDriver_PressEncoder(uint8_t enc) {
    if (ts.menu_mode) return;

    uint8_t state = ts.enc_state[enc] >> ENC_STATE_BITS;
    uint8_t mode = ts.enc_state[enc] & ENC_MODE_MASK;

    switch (state) {
        case ENC_STATE_NORM:
            if (ts.expflags2 & EXPFLAGS2_ENC_CAROUSEL) {
                state = ENC_STATE_CAROUSEL;
            } else {
                ts.enc_state[enc] = (ts.enc_state[enc] + 1) % ENC_STATE_NUM;
                UiDriver_DisplayEncoderMode(enc);

                return;
            }
            break;

        case ENC_STATE_CAROUSEL:
            state = ENC_STATE_NORM;
            break;

        default:
            return;
    }

    ts.enc_state[enc] = (state << ENC_STATE_BITS) | mode;
    UiDriver_DisplayEncoderMode(enc);
}

static void UiDriver_PressEncoderOne(void) {
    UiDriver_PressEncoder(ENC1);
}

static void UiDriver_PressEncoderTwo(void) {
    UiDriver_PressEncoder(ENC2);
}

static void UiDriver_PressEncoderThree(void) {
    UiDriver_PressEncoder(ENC3);
}

/**
 * @brief Displays audio speaker volume
 */
static void UiDriver_DisplayAfGain(uint8_t enc, uint8_t style)
{
	UiDriver_EncoderDisplaySimple(0, enc, "AFG", style, ts.rx_gain[RX_AUDIO_SPKR].value);
}

/**
 * @brief Displays power volume
 */
static void UiDriver_DisplayPWRGain(uint8_t enc, uint8_t style)
{
	if (ts.band_index != 17) // not GEN-band
	{
		const BandInfo* band = RadioManagement_GetBand(df.tune_old);
		UiDriver_EncoderDisplaySimple(0, enc, "PWR", style, ts.pwr_adj[ts.power_level == PA_LEVEL_FULL?ADJ_FULL_POWER:ADJ_REF_PWR][band->band_mode]);
	}
	else // GEN-band
	{
		UiDriver_EncoderDisplaySimple(0, enc, "PWR", style, ((ts.power_level == PA_LEVEL_FULL && RadioManagement_CBFullPwrEnabled())? ts.power_scale_gen_full : ts.power_scale_gen));
	}
}

/**
 * @brief Display CW Sidetone gain (used during CW TX or training)
 */
static void UiDriver_DisplaySidetoneGain(uint8_t enc, uint8_t style)
{
	UiDriver_EncoderDisplaySimple(0, enc, "STG", style, ts.cw_sidetone_gain);
}

/**
 * @brief Display TX Compressor Level
 */
static void UiDriver_DisplayCmpLevel(uint8_t enc, uint8_t style)
{
	ushort 	color = sd.txt_colour;
	char	temp[5];
	const char* outs;

	if (ts.tx_comp_level == TX_AUDIO_COMPRESSION_MIN)
	{
		outs ="OFF";
	}
	else if(ts.tx_comp_level < TX_AUDIO_COMPRESSION_MAX)	 	// 	display numbers for all but the highest value
	{
		snprintf(temp,5,"  %02d",ts.tx_comp_level);
		outs = temp;
	}
	else
	{
		color = Yellow; // Custom value - use yellow
		outs ="CUS";
	}

	UiDriver_EncoderDisplay(0, enc, "CMP" , style, outs, color);
}

uint32_t UiDriver_GetActiveDSPFunctions()
{
	return ts.dsp.active & (DSP_NOTCH_ENABLE|DSP_NR_ENABLE|DSP_ANR_ENABLE|DSP_MNOTCH_ENABLE|DSP_MPEAK_ENABLE);
}

static void UiDriver_DisplayDSPMode() {
    uint32_t    color = sd.txt_colour;
	char*       txt;

	switch (ts.dsp.mode) {
        case DSP_SWITCH_OFF:
            color = sd.txt_colour_dark;
            txt = "DSP OFF";
            break;

        case DSP_SWITCH_NR:
            txt = "NR";
            break;

        case DSP_SWITCH_ANR:
            txt = "A-NR";
            break;

        case DSP_SWITCH_NOTCH:
            txt = "A-NOTCH";
            break;

        case DSP_SWITCH_NR_AND_NOTCH:
            txt = "NR+NOTCH";
            break;

        case DSP_SWITCH_NOTCH_MANUAL:
            txt = "NOTCH";
            break;

        case DSP_SWITCH_PEAK_FILTER:
            txt = "PEAK";
            break;

        default:
            txt = "???";
            break;
    }

	UiLcdHy28_DrawEmptyRect(
        ts.Layout->DSP_IND.x,
        ts.Layout->DSP_IND.y,
        ts.Layout->DSP_IND.h,
        ts.Layout->DSP_IND.w,
        sd.boxes_colour
    );

    UiLcdHy28_PrintTextCentered(
            ts.Layout->DSP_IND.x + 1,
            ts.Layout->DSP_IND.y + 2,
            ts.Layout->DSP_IND.w - 2,
            txt,
            color, Black,
            0
    );

}

static UiDriver_DisplayStateAGC() {
    uint16_t color = sd.txt_colour_dark;

    if (agc_wdsp_conf.action == 1) {
        color = sd.txt_colour;
    }

    UiLcdHy28_DrawEmptyRect(
        ts.Layout->AGC_MASK.x,
        ts.Layout->AGC_MASK.y,
        ts.Layout->AGC_MASK.h,
        ts.Layout->AGC_MASK.w,
        sd.boxes_colour
    );

    UiLcdHy28_PrintTextCentered(
            ts.Layout->AGC_MASK.x + 1,
            ts.Layout->AGC_MASK.y + 2,
            ts.Layout->AGC_MASK.w - 2,
            "AGC",
            color, Black,
            0
    );
}

static void UiDriver_DisplayNotch(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "NTCH", style, ts.dsp.notch_frequency);
}

static void UiDriver_DisplayPeak(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "PEAK", style, ts.dsp.peak_frequency);
}

static void UiDriver_DisplayKeyerSpeed(uint8_t enc, uint8_t style) {
	UiDriver_EncoderDisplaySimple(0, enc, "WPM", style, ts.cw_keyer_speed);
}

static void UiDriver_DisplayRttySpeed(uint8_t enc, uint8_t style) {
	UiDriver_EncoderDisplay(0, enc, "BD", style, rtty_speeds[rtty_ctrl_config.speed_idx].label, sd.txt_colour);
}

static void UiDriver_DisplayPskSpeed(uint8_t enc, uint8_t style) {
	UiDriver_EncoderDisplay(0, enc, "PSK", style, psk_speeds[psk_ctrl_config.speed_idx].label, sd.txt_colour);
}

static void UiDriver_DisplayRttyShift(uint8_t enc, uint8_t style) {
	UiDriver_EncoderDisplay(0, enc, "SFT", style, rtty_shifts[rtty_ctrl_config.shift_idx].label, sd.txt_colour);
}

static void UiDriver_DisplayInGain(uint8_t enc, uint8_t style) {
	const char* txt;
	char  txt_buf[5];

	bool gain_external_control = false;
	// if true, gain is controlled externally and ENC3 encoder does not do anything.

	switch (ts.tx_audio_source) {
	case TX_AUDIO_MIC:
		txt = "MIC";
		break;
	case TX_AUDIO_LINEIN_L:									// Line gain
		txt = "L>L";
		break;
	case TX_AUDIO_LINEIN_R:									// Line gain
		txt = "L>R";
		break;
	case TX_AUDIO_DIG:										// Line gain
		txt = "DIG";
		gain_external_control = true;
		break;
	case TX_AUDIO_DIGIQ:
		txt = "DIQ";
		gain_external_control = true;
		break;
	default:
		txt = "???";
	}

	if (gain_external_control == true) {
		snprintf(txt_buf, sizeof(txt_buf), " EXT");
	} else {
		snprintf(txt_buf,sizeof(txt_buf), "%4d", ts.tx_gain[ts.tx_audio_source]);
	}

	UiDriver_EncoderDisplay(0, enc, txt, style, txt_buf, sd.txt_colour);
}

static void UiDriver_DisplayIn(uint8_t enc, uint8_t style) {
    const char* txt;

    switch (ts.tx_audio_source) {
        case TX_AUDIO_MIC:
            txt = "MIC";
            break;
        case TX_AUDIO_LINEIN_L:
            txt = "L>L";
            break;
        case TX_AUDIO_LINEIN_R:
            txt = "L>R";
            break;
        case TX_AUDIO_DIG:
            txt = "DIG";
            break;
        case TX_AUDIO_DIGIQ:
            txt = "DIQ";
            break;
        default:
            txt = "???";
    }

    UiDriver_EncoderDisplay(0, enc, "IN", style, txt, sd.txt_colour);
}

static void UiDriver_DisplayMeter(uint8_t enc, uint8_t style) {
    const char* txt;

    switch (ts.tx_meter_mode) {
        case METER_SWR:
            txt = "SWR";
            break;

        case METER_AUDIO:
            txt = "AUD";
            break;

        case METER_ALC:
            txt = "ALC";
            break;

        case METER_PWR:
            txt = "PWR";
            break;

        default:
            txt = "???";
    }

    UiDriver_EncoderDisplay(0, enc, "MTR", style, txt, sd.txt_colour);
}

#ifdef SDR_AMBER
static void UiDriver_DisplayPre(uint8_t enc, uint8_t style) {
    const char* txt;

    switch (ts.amber_input_state) {
        case 0:
            txt = "  ON";
            break;

        case 1:
            txt = " OFF";
            break;

        case 2:
            txt = " -12";
            break;

        case 3:
            txt = " -24";
            break;

        default:
            txt = "???";
    }

    UiDriver_EncoderDisplay(0, enc, "PRE", style, txt, sd.txt_colour);
}
#endif

static void UiDriver_DisplayRfGain(uint8_t enc, uint8_t style) {
	UiDriver_EncoderDisplaySimple(0, enc, "RFG", style, agc_wdsp_conf.thresh);
}

static void UiDriver_DisplaySQL(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "SQL", style, ts.fm_sql_threshold);
}

uint32_t UiDriver_GetNBColor()
{
    uint32_t retval;

    if(ts.dsp.nb_setting >= NB_WARNING3_SETTING)
        retval = Red;        // above this value, make it red
    else if(ts.dsp.nb_setting >= NB_WARNING2_SETTING)
        retval = Orange;     // above this value, make it orange
    else if(ts.dsp.nb_setting >= NB_WARNING1_SETTING)
        retval = Yellow;     // above this value, make it yellow
    else
        retval = sd.txt_colour;

    return retval;
}

static void UiDriver_DisplayNoiseBlanker(uint8_t enc, uint8_t style) {
    uint32_t    color;
    char        temp[5];
    const char  *val_txt;
    int32_t     value = ts.dsp.nb_setting;

    color = UiDriver_GetNBColor();

    if (value) {
        snprintf(temp, 5, "%4ld", value);
        val_txt = temp;
    } else {
        val_txt = " OFF";
    }

    UiDriver_EncoderDisplay(0, enc, "NB", style, val_txt, color);
}

static void UiDriver_DisplayAGC(uint8_t enc, uint8_t style) {
    const char  *val;

    switch (agc_wdsp_conf.mode) {
        case 0:
            val = " vLO";
            break;
        case 1:
            val = " LON";
            break;
        case 2:
            val = " SLO";
            break;
        case 3:
            val = " MED";
            break;
        case 4:
            val = " FAS";
            break;
        case 5:
            val = " OFF";
            break;
        default:
            val = " ???";
            break;
    }

    UiDriver_EncoderDisplay(0, enc, "AGC", style, val, sd.txt_colour);
}

#define NOTCH_DELTA_Y (2*ENC_ROW_H)

static void UiDriver_DisplayBas(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "BAS", style, ts.dsp.bass_gain);
}

static void UiDriver_DisplayTreble(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "TRB", style, ts.dsp.treble_gain);
}

static void UiDriver_DisplayNR(uint8_t enc, uint8_t style) {
    UiDriver_EncoderDisplaySimple(0, enc, "NR", style, ts.dsp.nr_strength);
}

//
//*----------------------------------------------------------------------------
//* Function Name       : UiDriverChangeRit
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void UiDriver_DisplayRit(uint8_t enc, uint8_t style)
{
	char	temp[5];

	snprintf(temp, 5, ts.rit_value ? "%+4i" : "%4i", ts.rit_value);
	UiDriver_EncoderDisplay(0, enc, "RIT", style, temp, sd.txt_colour);
}

/**
 * Converts a power value in mW into useful null-terminated string
 * @param txt char array of at least 5 characters
 * @param txt_len length of txt array
 * @param power_mW power value, if 0 result is "FULL"
 */
void UiDriver_Power2String(char* txt, size_t txt_len, uint32_t power_mW) {
    if (power_mW == 0 ) {
        snprintf(txt, txt_len, "FULL");
    } else if (power_mW < 100) {
        snprintf(txt, txt_len, "%ldmW", power_mW);
    } else if (power_mW < 1000) {
        snprintf(txt, txt_len, "0.%ldW", power_mW / 100);
    } else {
        snprintf(txt, txt_len, "%ldW", power_mW / 1000);
    }
}

static void UiDriver_DisplayPowerLevel() {
    char txt[5];

    UiDriver_Power2String(txt, sizeof(txt), ts.power);

    uint16_t color = sd.txt_colour; // normal operation

    if (RadioManagement_IsTxDisabled()) {
        // we'll not transmit, power is irrelevant
        color = sd.txt_colour_dark;
    } else if (ts.tx_power_factor == 0) {
        // no output at all will be generate with power factor 0
        // probably not calibrated PA
        color = Red;
    } else if (ts.power_modified) {
        // transmit p, power is irrelevant
        color = Orange;
    }

	UiLcdHy28_DrawEmptyRect(
        ts.Layout->PW_IND.x,
        ts.Layout->PW_IND.y,
        ts.Layout->PW_IND.h,
        ts.Layout->PW_IND.w,
        sd.boxes_colour
    );

    UiLcdHy28_PrintTextCentered(
            ts.Layout->PW_IND.x + 1,
            ts.Layout->PW_IND.y + 2,
            ts.Layout->PW_IND.w - 2,
            txt,
            color, Black,
            0
    );
}

static void UiDriver_DisplayDbm(void) {
    bool display_something = false;
    static long oldVal=99999;
    static uint8_t dBmShown=0;
    char txt[12];

    if (ts.txrx_mode == TRX_MODE_RX) {
        long val;
        const char* unit_label;

        switch(ts.display_dbm) {
            case DISPLAY_S_METER_DBM:
                display_something = true;
                val = sm.dbm;
                unit_label = "dBm   ";

                break;
            case DISPLAY_S_METER_DBMHZ:
                display_something = true;
                val = sm.dbmhz;
                unit_label = "dBm/Hz";
                break;
        }

        if ((display_something == true) && (val != oldVal)) {
            snprintf(txt, sizeof(txt), "%4ld", val);

            UiLcdHy28_PrintText(
                ts.Layout->DisplayDbm.x,
                ts.Layout->DisplayDbm.y,
                txt,
                sd.txt_colour, Black,
                4
            );

            UiLcdHy28_PrintText(
                ts.Layout->DisplayDbm.x + SMALL_FONT_WIDTH * 4,
                ts.Layout->DisplayDbm.y,
                unit_label,
                sd.txt_colour, Black,
                4
            );

            oldVal = val;     //this will prevent from useless redrawing the same
            dBmShown = 1;     //for indicate that dms are shown and erase function may it clear when needed
        }
    }

    // clear the display since we are not showing dBm or dBm/Hz or we are in TX mode
    if ((display_something == false) && (dBmShown == 1)) {
   		UiLcdHy28_DrawFullRect(ts.Layout->DisplayDbm.x, ts.Layout->DisplayDbm.y, 8, 75, Black);

   		dBmShown=0;     //just to indicate that dbm is erased
        oldVal=99999;   //some value that will enforce refresh when user enable display dbm
    }
}

/**
 * Handles calculation of SMeter values and also handles the input gain adjustment to keep input inside good range
 */
void RadioManagement_HandleIqGainAndSMeter()
{
    if(ts.txrx_mode == TRX_MODE_RX)
    {
        RadioManagement_HandleRxIQSignalCodecGain();

        // lowpass IIR filter
        // Wheatley 2011: two averagers with two time constants
        // IIR filter with one element analog to 1st order RC filter
        // but uses two different time constants (ALPHA = 1 - e^(-T/Tau)) depending on
        // whether the signal is increasing (attack) or decreasing (decay)
        // we scale Alpha values by 100 so that we have a usable range for configuring the speed of attack and decay in menu items (from 1 to 100)

        const float32_t alpha_scale_inv = 0.01;
        float32_t attackAlpha = sm.config.alphaSplit.AttackAlpha *alpha_scale_inv;

        float32_t decayAlpha = sm.config.alphaSplit.DecayAlpha *alpha_scale_inv;
        sm.AttackAvedbm =   (1.0 - attackAlpha) * sm.AttackAvedbm   + attackAlpha * sm.dbm_cur;
        sm.DecayAvedbm =    (1.0 - decayAlpha)  * sm.DecayAvedbm    + decayAlpha  * sm.dbm_cur;
        sm.AttackAvedbmhz = (1.0 - attackAlpha) * sm.AttackAvedbmhz + attackAlpha * sm.dbmhz_cur;
        sm.DecayAvedbmhz =  (1.0 - decayAlpha)  * sm.DecayAvedbmhz  + decayAlpha  * sm.dbmhz_cur;

        if (sm.AttackAvedbm > sm.DecayAvedbm)
        { // if attack average is larger then it must be an increasing signal
            sm.dbm = sm.AttackAvedbm; // use attack average value for output
            sm.DecayAvedbm = sm.AttackAvedbm; // set decay average to attack average value for next time
        }
        else
        { // signal is decreasing, so use decay average value
            sm.dbm = sm.DecayAvedbm;
        }

        if (sm.AttackAvedbmhz > sm.DecayAvedbmhz)
        { // if attack average is larger then it must be an increasing signal
            sm.dbmhz = sm.AttackAvedbmhz; // use attack average value for output
            sm.DecayAvedbmhz = sm.AttackAvedbmhz; // set decay average to attack average value for next time
        }
        else
        { // signal is decreasing, so use decay average value
            sm.dbmhz = sm.DecayAvedbmhz;
        }
    }
}

static void UiDriver_HandleSMeter(void)
{

	// Only in RX mode
	if(ts.txrx_mode == TRX_MODE_RX)
	{
		// This makes a portion of the S-meter go red if A/D clipping occurs
		//
		{
			static bool         clip_indicate = 0;

			const float *S_Meter_Cal_Ptr = S_Meter_Cal_dbm;
			uint32_t s_count = 0;

			// find corresponding signal level
			for (
					s_count = 1;
					(sm.dbm >= S_Meter_Cal_Ptr[s_count]) && (s_count < S_Meter_Cal_Size);
					s_count++)
			{
				// nothing to do here
			}

			sm.s_count = s_count; // preserve value for CAT level reading

			// make sure that the S meter always reads something!
			UiDriver_UpdateMeterRX((s_count>0) ? s_count : 1);
            UiDriver_DisplayDbm();
		}
	}
}



/**
 *
 * Power, SWR, ALC and Audio indicator handling
 */
static void UiDriver_HandleTXMeters(void) {
	// Only in TX mode

	if (ts.txrx_mode != TRX_MODE_TX) {
		swrm.vswr_dampened = 0;		// reset averaged readings when not in TX mode
		swrm.fwd_pwr_avg = -1;
		swrm.rev_pwr_avg = -1;

        swrm.p_curr   = 0;
        swrm.fwd_calc = 0;
        swrm.rev_calc = 0;
        swrm.high_vswr_detected = false;

	} else {
		static uint8_t    old_power_level = 99;

		if (swrm.high_vswr_detected == true) {
			Board_RedLed(LED_STATE_TOGGLE);
		}

		// display FWD, REV power, in milliwatts - used for calibration - IF ENABLED
		if (swrm.pwr_meter_disp) {
			if((swrm.fwd_pwr_avg < 0) || (ts.power_level != old_power_level)) { 	// initialize with current value if it was zero (e.g. reset) or power level changed
				swrm.fwd_pwr_avg = swrm.fwd_pwr;
			} else {
				swrm.fwd_pwr_avg = (swrm.fwd_pwr_avg * (1-PWR_DAMPENING_FACTOR)) + swrm.fwd_pwr * PWR_DAMPENING_FACTOR;	// apply IIR smoothing to forward power reading
			}

			if((swrm.rev_pwr_avg < 0) || (ts.power_level != old_power_level)) { 	// initialize with current value if it was zero (e.g. reset) or power level changed
				swrm.rev_pwr_avg = swrm.rev_pwr;
			} else {
				swrm.rev_pwr_avg = (swrm.rev_pwr_avg * (1-PWR_DAMPENING_FACTOR)) + swrm.rev_pwr * PWR_DAMPENING_FACTOR; // apply IIR smoothing to reverse power reading
			}

			old_power_level = ts.power_level;		// update power level change detector
		}

        char txt[16];
        const char* txp = NULL;

        if (swrm.pwr_meter_disp) {
            snprintf(txt,16, "%5d,%5d", (int)(swrm.fwd_pwr_avg*1000), (int)(swrm.rev_pwr_avg*1000));		// scale to display power in milliwatts
            txp = txt;
            swrm.pwr_meter_was_disp = 1;	 // indicate the power meter WAS displayed
        } else if(swrm.pwr_meter_was_disp) { // had the numerical display been enabled - and it is now disabled?
            txp = "           ";             // yes - overwrite location of numerical power meter display to blank it
            swrm.pwr_meter_was_disp = 0;	 // clear flag so we don't do this again
        }

        if (txp != NULL) {
            UiLcdHy28_PrintText(ts.Layout->PWR_NUM_IND.x, ts.Layout->PWR_NUM_IND.y,txp,Green,Black,0);
        }

		// Do selectable meter readings
		float   btm_mtr_val = 0.0;
		uint32_t btm_mtr_red_level = 13;

		switch (ts.tx_meter_mode) {
		    case METER_PWR:
		        btm_mtr_val = swrm.fwd_pwr * 3;
		        btm_mtr_red_level = 0;
		        break;

		    case METER_SWR:
		        if(swrm.fwd_pwr >= SWR_MIN_CALC_POWER) {	 		// is the forward power high enough for valid VSWR calculation?
		            // (Do nothing/freeze old data if below this power level)

		            if(swrm.vswr_dampened < 1) {	// initialize averaging if this is the first time (e.g. VSWR <1 = just returned from RX)
		                swrm.vswr_dampened = swrm.vswr;
		            } else {
		                swrm.vswr_dampened = swrm.vswr_dampened * (1 - VSWR_DAMPENING_FACTOR) + swrm.vswr * VSWR_DAMPENING_FACTOR;
		            }

		            btm_mtr_val = swrm.vswr_dampened * 4;		// yes - four dots per unit of VSWR
		        }
		        break;

		    case METER_ALC:
		        btm_mtr_val = ads.alc_val;		    // get TX ALC value
		        btm_mtr_val *= btm_mtr_val;		    // square the value
		        btm_mtr_val = log10f(btm_mtr_val);	// get the log10
		        btm_mtr_val *= -10;		            // convert it to DeciBels and switch sign and then scale it for the meter
		        break;

		    case METER_AUDIO:
		        btm_mtr_val = ads.peak_audio/10000; // get a copy of the peak TX audio (maximum reference = 30000)
		        ads.peak_audio = 0;					// reset the peak detect
		        btm_mtr_val *= btm_mtr_val;			// square the value
		        btm_mtr_val = log10f(btm_mtr_val);	// get the log10
		        btm_mtr_val *= 10;					// convert to DeciBels and scale for the meter
		        btm_mtr_val += 11;					// offset for meter
		        btm_mtr_red_level = 22;	            // setting the "red" threshold
		        break;
		}

		UiDriver_UpdateMeterTX(btm_mtr_val, btm_mtr_red_level);   // update the meter, setting the "red" threshold
	}
}

static void UiDriver_CreateSomeKindDisplay() {
    UiLcdHy28_DrawEmptyRect(
        ts.Layout->INFO_BAR.x,
        ts.Layout->INFO_BAR.y,
        ts.Layout->INFO_BAR.h,
        ts.Layout->INFO_BAR.w,
        sd.boxes_colour);

    UiLcdHy28_DrawEmptyRect(
        ts.Layout->MID_BAR.x,
        ts.Layout->MID_BAR.y,
        ts.Layout->MID_BAR.h,
        ts.Layout->MID_BAR.w,
        sd.boxes_colour);

    UiLcdHy28_DrawEmptyRect(
        ts.Layout->TUNE_BOX.x,
        ts.Layout->TUNE_BOX.y,
        ts.Layout->TUNE_BOX.h,
        ts.Layout->TUNE_BOX.w,
        sd.boxes_colour);
}

static bool UiDriver_SaveConfiguration()
{
	bool savedConfiguration = true;

	const uint16_t scope_middle_y = sd.Slayout->full.h/2+sd.Slayout->full.y;

	const char* txp;
	uint16_t txc;

	switch (ts.configstore_in_use)
	{
	case CONFIGSTORE_IN_USE_FLASH:
		txp = "Saving settings to Flash Memory";
		break;
	case CONFIGSTORE_IN_USE_I2C:
		txp = "Saving settings to I2C EEPROM";
		break;
	default:
		txp = "Detected problems: Not saving";
		savedConfiguration = false;
	}
	UiLcdHy28_PrintTextCentered(sd.Slayout->full.x, scope_middle_y-6, sd.Slayout->full.w,txp,Blue,Black,0);

	if (savedConfiguration)
	{
		// save settings
		if (UiConfiguration_SaveEepromValues() == 0)
		{
			txp = "Saving settings finished";
			txc = Green;
		}
		else
		{
			txp = "Saving settings failed";
			txc = Red;
			savedConfiguration = false;
		}
		UiLcdHy28_PrintTextCentered(sd.Slayout->full.x, scope_middle_y+6, sd.Slayout->full.w,txp,txc,Black,0);
	}
	return savedConfiguration;
}


/*
 * @brief displays the visual information that power down is being executed and saves EEPROM if requested
 */
static void UiDriver_PowerDownCleanup(bool saveConfiguration)
{
	const char* txp;
	// Power off all - high to disable main regulator

	ts.powering_down = 1;   // indicate that we should be powering down

	UiSpectrum_Clear();   // clear display under spectrum scope

	// hardware based mute
	Codec_MuteDAC(true);  // mute audio when powering down

	txp = " ";
#ifndef SDR_AMBER_480_320
	UiLcdHy28_PrintTextCentered(60,148,240,txp,Blue2,Black,0);
	UiLcdHy28_PrintTextCentered(60,156,240,"Powering off...",Blue2,Black,0);
	UiLcdHy28_PrintTextCentered(60,168,240,txp,Blue2,Black,0);
#else
	UiLcdHy28_PrintTextCentered(120,148,240,txp,Blue2,Black,0);
	UiLcdHy28_PrintTextCentered(120,156,240,"Powering off...",Blue2,Black,0);
	UiLcdHy28_PrintTextCentered(120,168,240,txp,Blue2,Black,0);
#endif

	if (saveConfiguration)
	{
		UiDriver_SaveConfiguration();
	}
	else
	{
#ifndef SDR_AMBER_480_320
	#ifndef OVI40_MOD_480_320
		UiLcdHy28_PrintTextCentered(60,176,260,"...without saving settings...",Blue,Black,0);
	#else
		UiLcdHy28_PrintTextCentered(110,176,260,"...without saving settings...",Blue,Black,0);
	#endif
#else
		UiLcdHy28_PrintTextCentered(110,176,260,"...without saving settings...",Blue,Black,0);
#endif
	}


	if(saveConfiguration)
	{
		UiConfiguration_SaveEepromValues();     // save EEPROM values
	}

	HAL_Delay(3000);
}



/*
 * @brief Display external voltage
 */
static void UiDriver_DisplayVoltage() {
	uint32_t low_power_threshold = ((ts.low_power_config & LOW_POWER_THRESHOLD_MASK) + LOW_POWER_THRESHOLD_OFFSET) * 10;
	uint32_t col = sd.txt_colour;

	if (pwmt.voltage < low_power_threshold + 50) {
		col = Red;
	} else if (pwmt.voltage < low_power_threshold + 100) {
		col = Orange;
	} else if (pwmt.voltage < low_power_threshold + 150) {
		col = Yellow;
	}

	static uint8_t voltage_blink = 0;

	// in case of low power shutdown coming, we let the voltage blink with 1hz

	if (pwmt.undervoltage_detected == true && voltage_blink < 1 ) {
		col = Black;
	}

	voltage_blink++;

	if (voltage_blink == 2) {
		voltage_blink = 0;
	}

	char txt[9];
	snprintf(txt, sizeof(txt), "%2ld.%02ldV", pwmt.voltage / 100, pwmt.voltage % 100);

	UiLcdHy28_PrintText(
	    ts.Layout->PWR_IND.x,
	    ts.Layout->PWR_IND.y,
	    txt,
	    col, Black,
	    4
	);
}

/**
 * @brief Measures Voltage and controls undervoltage detection
 * @returns true if display update is required, false if not
 */
static bool UiDriver_HandleVoltage(void)
{
	bool retval = false;
	// if this is set to true, we should update the display because something relevant for the user happened.

	// Collect samples
	if(pwmt.p_curr < POWER_SAMPLES_CNT)
	{
		// Add to accumulator
		pwmt.pwr_aver = pwmt.pwr_aver + HAL_ADC_GetValue(&hadc1);
		pwmt.p_curr++;
	}
	else
	{

		// Get average
		uint32_t val_p  = ((pwmt.pwr_aver/POWER_SAMPLES_CNT) * (ts.voltmeter_calibrate + 900))/2500;

		// Reset accumulator
		pwmt.p_curr     = 0;
		pwmt.pwr_aver   = 0;


		retval = pwmt.voltage != val_p;

		pwmt.voltage = val_p;


		uint32_t low_power_threshold = ((ts.low_power_config & LOW_POWER_THRESHOLD_MASK) + LOW_POWER_THRESHOLD_OFFSET) * 10;
		bool low_power_shutdown_enabled = (ts.low_power_config & LOW_POWER_ENABLE_MASK) == LOW_POWER_ENABLE;

		if (low_power_shutdown_enabled && (val_p < low_power_threshold ))
		{
			// okay, voltage is too low, we should indicate
			pwmt.undervoltage_detected = true;
			retval = true;

			if (ts.txrx_mode == TRX_MODE_RX)
			{
				if (ts.sysclock > ts.low_power_shutdown_time )         // only allow power-off in RX mode
				{
					UiDriver_PowerDownCleanup(true);
				}
			}
			else
			{
				ts.low_power_shutdown_time = ts.sysclock + LOW_POWER_SHUTDOWN_DELAY_TIME;
				// in tx mode, we extend the waiting time during the transmit, so that we don't switch off
				// right after a transmit but let the battery some time to "regenerate"
			}
		}
		else
		{
			if (pwmt.undervoltage_detected == true)
			{
				retval = true;
				pwmt.undervoltage_detected = false;
				Board_GreenLed(LED_STATE_ON);
			}
			ts.low_power_shutdown_time = ts.sysclock + LOW_POWER_SHUTDOWN_DELAY_TIME;
		}
	}

	return retval;
}

/**
 * @brief display measured temperature and current state of TCXO
 * @param temp in tenth of degrees Celsius (10 == 1 degree C)
 */
static void UiDriver_DisplayTemperature(int temp) {
	static int last_disp_temp = -100;
	uint32_t clr =  RadioManagement_TcxoGetMode() ==TCXO_ON ? Blue:Red;

	if (temp != last_disp_temp) {
		char out[10];
		char* txt_ptr;

		if ((temp < 0) || (temp > 1000)) {  // is the temperature out of range?
			txt_ptr = "RANGE!";
		} else {
			last_disp_temp = temp;

			int32_t ttemp = last_disp_temp;
			char    unit = 'C';

			if (RadioManagement_TcxoIsFahrenheit()) {
				ttemp = ((ttemp * 9) / 5) + 320;			// multiply by 1.8 and add 32 degrees
				unit = 'F';
			}

			snprintf(out, sizeof(out), "%3ld.%1ld%c", ttemp / 10, ttemp % 10, unit);
			txt_ptr = out;
		}

		UiLcdHy28_PrintText(
		    ts.Layout->TEMP_IND.x,
		    ts.Layout->TEMP_IND.y,
		    txt_ptr,
		    sd.txt_colour, Black,
		    4
		);
	}
}

//*----------------------------------------------------------------------------
//* Function Name       : UiDriverHandleLoTemperature
//* Object              : display LO temperature and compensate drift
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
static void UiDriver_HandleLoTemperature()
{
	if (SoftTcxo_HandleLoTemperatureDrift())
	{
		UiDriver_DisplayTemperature(lo.temp/1000); // precision is 0.1 represent by lowest digit
	}
}



//*----------------------------------------------------------------------------
//* Function Name       : UiDriverEditMode
//* Object              :
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//*----------------------------------------------------------------------------
/*static void UiDriverEditMode()
{
	char symb[2];

	// Is edit mode ?
	if(kbs.set_mode != 1)
		return;

	// Key pressed
	if(kbs.last_char == 0)
		return;

	//printf("key = %02x ",kbs.last_char);

	// Handle CR
	if(kbs.last_char == 0x0a)
	{
		kbs.edit_item_id++;
		if(kbs.edit_item_id == 3)
			kbs.edit_item_id = 0;

		// Switch items
		switch(kbs.edit_item_id)
		{
			case 0:
			{
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y +  0),"Call:  ",White,Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 15),"Loc:   ",Grey, Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 30),"Power: ",Grey, Black,0);
				break;
			}

			case 1:
			{
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y +  0),"Call:  ",Grey,Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 15),"Loc:   ",White, Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 30),"Power: ",Grey, Black,0);
				break;
			}

			case 2:
			{
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y +  0),"Call:  ",Grey,Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 15),"Loc:   ",Grey, Black,0);
				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 5),(POS_SPECTRUM_IND_Y + 30),"Power: ",White, Black,0);
				break;
			}

			default:
				break;
		}

		// Reset hor ptr
		kbs.edit_item_hor = 0;
	}
	else
	{
		symb[0] = kbs.last_char;
		symb[1] = 0;

		// Print items
		switch(kbs.edit_item_id)
		{
			case 0:
			{
				// Add to buffer
				kbs.item_0[kbs.edit_item_hor] = kbs.last_char;

				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 80 + (kbs.edit_item_hor*12)),(POS_SPECTRUM_IND_Y +  0),symb,Grey,Black,0);
				break;
			}

			case 1:
			{
				// Add to buffer
				kbs.item_1[kbs.edit_item_hor] = kbs.last_char;

				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 80 + (kbs.edit_item_hor*12)),(POS_SPECTRUM_IND_Y + 15),symb,Grey, Black,0);
				break;
			}

			case 2:
			{
				// Add to buffer
				kbs.item_2[kbs.edit_item_hor] = kbs.last_char;

				UiLcdHy28_PrintText((POS_SPECTRUM_IND_X + 80 + (kbs.edit_item_hor*12)),(POS_SPECTRUM_IND_Y + 30),symb,Grey, Black,0);
				break;
			}

			default:
				break;
		}

		// Move cursor right
		kbs.edit_item_hor++;
		if(kbs.edit_item_hor == 10)
			kbs.edit_item_hor = 0;
	}

	// Clear public
	kbs.last_char = 0;
}*/

typedef enum
{
	CONFIG_DEFAULTS_KEEP = 0,
	CONFIG_DEFAULTS_LOAD_FREQ,
	CONFIG_DEFAULTS_LOAD_ALL
} CONFIG_DEFAULTS;


static void UiDriver_WaitForBandMAndBandPorPWR(void)
{
    while((((UiDriver_IsButtonPressed(BUTTON_BNDM_PRESSED)) && (UiDriver_IsButtonPressed(BUTTON_BNDP_PRESSED))) == false) && UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED) == false)
    {
        HAL_Delay(20);
        Keypad_Scan();
    }
}


/*
 * @brief Handles the loading of the configuration at startup (including the load of defaults if requested)
 * @returns false if it is a normal startup, true if defaults have been loaded
 */
static bool UiDriver_LoadSavedConfigurationAtStartup()
{

	bool retval = false;
	CONFIG_DEFAULTS load_mode = CONFIG_DEFAULTS_KEEP;

	if (UiDriver_IsButtonPressed(BUTTON_F1_PRESSED) && UiDriver_IsButtonPressed(BUTTON_F3_PRESSED) && UiDriver_IsButtonPressed(BUTTON_F5_PRESSED))
	{
		load_mode = CONFIG_DEFAULTS_LOAD_ALL;
	}
	else if (UiDriver_IsButtonPressed(BUTTON_F2_PRESSED) && UiDriver_IsButtonPressed(BUTTON_F4_PRESSED))
	{
		load_mode = CONFIG_DEFAULTS_LOAD_FREQ;
	}

	if(load_mode != CONFIG_DEFAULTS_KEEP)
	{
		// let us make sure, the user knows what he/she is doing
		// in case of change of mindes, do normal configuration load

		uint32_t clr_fg = White, clr_bg = Black;
		const char* top_line = "";

		switch (load_mode)
		{
		case CONFIG_DEFAULTS_LOAD_ALL:
			clr_bg = Red;
			clr_fg = White;
			top_line = "ALL DEFAULTS";
			break;
		case CONFIG_DEFAULTS_LOAD_FREQ:
			clr_bg = Yellow;
			clr_fg = Black;
			top_line = "FREQ/MODE DEFAULTS";
			break;
		default:
			break;
		}


		UiLcdHy28_LcdClear(clr_bg);							// clear the screen
		// now do all of the warnings, blah, blah...
		UiLcdHy28_PrintTextCentered(2,05, 316, top_line,clr_fg,clr_bg,1);
		UiLcdHy28_PrintTextCentered(2,35, 316, "-> LOAD REQUEST <-",clr_fg,clr_bg,1);

		UiLcdHy28_PrintTextCentered(2,70, 316,
				"If you don't want to do this\n"
				"press POWER button to start normally.",clr_fg,clr_bg,0);

		UiLcdHy28_PrintTextCentered(2,120, 316,
				"If you want to load default settings\n"
				"press and hold BAND+ AND BAND-.\n"
				"Settings will be saved at POWEROFF",clr_fg,clr_bg,0);

		// On screen delay									// delay a bit...
		HAL_Delay(5000);

		// add this for emphasis
		UiLcdHy28_PrintTextCentered(2,195, 316,
				"Press BAND+ and BAND-\n"
				"to confirm loading",clr_fg,clr_bg,0);

		UiDriver_WaitForBandMAndBandPorPWR();

		const char* txp;

		if(UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED))
		{
			clr_bg = Black;							// clear the screen
			clr_fg = White;
			txp = "...performing normal start...";

			load_mode = CONFIG_DEFAULTS_KEEP;
			retval = false;
		}
		else
		{
			txp = "...loading defaults in progress...";
			// call function to load values - default instead of EEPROM
			retval = true;
			ts.menu_var_changed = true;
		}
		UiLcdHy28_LcdClear(clr_bg);                         // clear the screen
		UiLcdHy28_PrintTextCentered(2,108,316,txp,clr_fg,clr_bg,0);
		HAL_Delay(5000);
	}

	bool load_freq_mode_defaults = false;
	bool load_eeprom_defaults = false;
	switch (load_mode)
	{
	case CONFIG_DEFAULTS_LOAD_ALL:
		load_eeprom_defaults = true;                           // yes, set flag to indicate that defaults will be loaded instead of those from EEPROM
		break;
	case CONFIG_DEFAULTS_LOAD_FREQ:
		load_freq_mode_defaults = true;
		break;
	default:
		break;
	}

	UiConfiguration_LoadEepromValues(load_freq_mode_defaults, load_eeprom_defaults);

	return retval;
}

//*----------------------------------------------------------------------------
//* Function Name       : UiCheckForPressedKey
//* Object              : Used for testing keys on the front panel
//* Input Parameters    :
//* Output Parameters   :
//* Functions called    :
//* Comments            : If a button (other than "POWER") is held during power-up a screen is displayed that shows which key (if any)
//  Comments            : is being pressed.  If multiple keys are being pressed, only the one with the highest precedence is displayed.  The order of decreasing
//  Comments            : precedence is:  M2, G3, G2, BNDM, G4, M3, STEPM, STEPP, M1, M3, F1, F2, F4, BNDP, F5, G1 and POWER.  [KA7OEI October, 2015]
//*----------------------------------------------------------------------------
static void UiDriver_KeyTestScreen()
{
	ushort p_o_state = 0, rb_state = 0, new_state = 0;
	uint32_t poweroffCount = 0, rebootCount = 0;

	// int direction;

	uint32_t keyScanState = 0;

    Keypad_Scan(); // read and map the keys to their logical buttons
    // not all keys may have a button or some keys may go to the same button

	if (UiDriver_IsButtonPressed(TOUCHSCREEN_ACTIVE))
	{
	    UiLcdHy28_TouchscreenHasProcessableCoordinates();
	    // touchscreen was pressed:
	    // wait a little and see if touch is still pressed
	    // some TRX seem to see touchscreen events even if not pressed
	    HAL_Delay(500);
	}

	Keypad_Scan();

    keyScanState = Keypad_KeyStates();	// remember which one was pressed

	if(Keypad_IsAnyKeyPressed()) {			// at least one button was pressed

        char txt_buf[40];
        const char* txt;
        uint32_t encoderCount = 0;
        const uint32_t clr_fg = White;
        const uint32_t clr_bg = Blue;

		UiLcdHy28_LcdClear(Blue);							// clear the screen
        UiLcdHy28_PrintTextCentered(2,05,ts.Layout->Size.x-4,"INPUT TEST SCREEN",clr_fg,clr_bg,1);

		snprintf(txt_buf,40,"Keys Initial: %08lx",keyScanState);
		UiLcdHy28_PrintTextCentered(0,30,ts.Layout->Size.x,txt_buf,White,Blue,0);

		UiLcdHy28_PrintTextCentered(0,70,ts.Layout->Size.x,"press & hold POWER button to poweroff\npress & hold BAND- button to reboot",White,Blue,0);

		for(;;)	 		// get stuck here for test duration
		{
			uint32_t idxFirstPressedButton = 99;		// load with flag value
			uint32_t numOfPressedButtons = 0;

			// we slow down the loop a bit so that our wait counters take some time to go down.
            HAL_Delay(10);

	        Keypad_Scan();
	        // read and map the hw keys to their logical buttons
	        // not all keys may have a button or some keys may go to the same button


	        uint32_t newKeyScanState = Keypad_KeyStates();   // check which hw keys are pressed

	        if (newKeyScanState != keyScanState)
	        {
	            keyScanState = newKeyScanState;
	            snprintf(txt_buf,40,"Keys Current: %08lx",keyScanState);
	            UiLcdHy28_PrintTextCentered(0,45,ts.Layout->Size.x,txt_buf,White,Blue,0);
	        }


	        //  now find out which buttons are pressed (logical buttons, not hw keys)
			for(int buttonIdx = 0; buttonIdx < BUTTON_NUM ; buttonIdx++)
			{
				// scan all buttons
				if(UiDriver_IsButtonPressed(buttonIdx))
				{
					// is this button pressed?
					numOfPressedButtons++;
					if(idxFirstPressedButton == 99)						// is this the first button pressed?
					{
						idxFirstPressedButton = buttonIdx;						// save button number
					}
				}
			}

			if(idxFirstPressedButton == BUTTON_BNDM_PRESSED && new_state == 0)	// delay if BANDM was used to enter button test mode
			{
				rebootCount = 0;
				new_state = 1;
			}


			// now find out if an encoder was moved (we detect this by seeing a encoder value != 0)
			int32_t encoderDirection;

			uint32_t encoderIdx;
			for(encoderIdx = 0; encoderIdx < ENC_MAX; encoderIdx++)
			{
				encoderDirection = UiDriverEncoderRead(encoderIdx);
				if(encoderDirection != 0)
				{
					encoderCount = 200;
					break;
				}
			}

			if(encoderIdx != ENC_MAX)
			{
				snprintf(txt_buf,40," Encoder %ld <%s>", encoderIdx+1, encoderDirection>0 ? "right":"left");		// building string for encoders
				idxFirstPressedButton = BUTTON_NUM+encoderIdx;					// add encoders behind buttons;
			}

			if (idxFirstPressedButton < BUTTON_NUM)
			{
				txt = Keypad_GetLabelOfButton( idxFirstPressedButton );
			}
			else
			{
				txt = NULL;
			}
			switch(idxFirstPressedButton)	 				// decode keyPin to text
			{
			case	BUTTON_PWR_PRESSED:
				if(poweroffCount > 75)
				{
					txt = "powering off...";
					p_o_state = 1;
				}
				poweroffCount++;
				break;
			case	BUTTON_BNDM_PRESSED:
				if(rebootCount > 75)
				{
					txt = "rebooting...";
					rb_state = 1;
				}
				rebootCount++;
				break;
			case	TOUCHSCREEN_ACTIVE:

				if (UiLcdHy28_TouchscreenHasProcessableCoordinates())
				{

					snprintf(txt_buf,40,"x/y: %04d/%04d x/y raw: %04x/%04x",ts.tp->hr_x,ts.tp->hr_y,ts.tp->xraw,ts.tp->yraw);	//show touched coordinates
					UiLcdHy28_PrintTextCentered(2,216,ts.Layout->Size.x-4,txt_buf,White,Blue,0);           // identify button on screen
					UiLcdHy28_DrawColorPoint(ts.tp->hr_x,ts.tp->hr_y,White);

					txt = "Touch";
				}
				else
				{
					if (mchf_touchscreen.present)
					{
						txt = "Touch (no coord.)";
					}
					else
					{
						txt = "Touch (no cntrlr)";
					}
				}
				break;
			case	BUTTON_NUM+ENC1:							// handle encoder event
			case	BUTTON_NUM+ENC2:
			case	BUTTON_NUM+ENC3:
            case    BUTTON_NUM+ENCFREQ:
			    txt = txt_buf;
			    break;
			default:
				if (txt == NULL)
				{
					if(encoderCount == 0)
					{
						txt = "<no key>";				// no keyPin pressed
					}
					else
					{
						encoderCount--;
					}
					poweroffCount = 0;
					rebootCount = 0;
				}
			}

			if(txt != NULL)
			{
				UiLcdHy28_PrintTextCentered(0,120,ts.Layout->Size.x,txt,White,Blue,1);			// identify button on screen
			}

			snprintf(txt_buf,40, "# of buttons pressed: %ld  ", numOfPressedButtons);
			UiLcdHy28_PrintTextCentered(0,160,ts.Layout->Size.x,txt_buf,White,Blue,0);			// show number of buttons pressed on screen

			if(ts.tp->present)			// show translation of touchscreen if present
			{
				txt = "Touch Coordinates:";
			}
			else
			{
				txt = "Touch Controller not present";
			}

			UiLcdHy28_PrintTextCentered(0,200,ts.Layout->Size.x,txt,White,Blue,0);

			if(p_o_state == 1)
			{
                if(idxFirstPressedButton != BUTTON_PWR_PRESSED)
                {
                    Board_Powerdown();
                }
				// never reached
			}
			if(rb_state == 1)
			{
				if(idxFirstPressedButton != BUTTON_BNDM_PRESSED)
				{
					Board_Reboot();
				}
			}
		}
	}
}
//cross size definitions, must be odd
#define CrossSizeH 11
#define CrossSizeV 11
static void DrawCross(int16_t* coord,uint16_t color)
{
	UiLcdHy28_DrawStraightLine(coord[0]-(CrossSizeH/2), coord[1],CrossSizeH,        LCD_DIR_HORIZONTAL,color);
	UiLcdHy28_DrawStraightLine(coord[0], coord[1]-(CrossSizeV/2),CrossSizeV,        LCD_DIR_VERTICAL,color);
}


/*
 * @brief Touchscreen Calibration function
 * @returns false if it is a normal startup, true if touchscreen has been calibrated
 */

#define ARM_MATH_MATRIX_CHECK
#define Touch_ShowTestscreen

static void UiDriver_TouchscreenCalibrationRun(void)
{
    UiLcdHy28_TouchscreenReadCoordinates();
    ts.tp->state = TP_DATASETS_NONE;
    uint16_t MAX_X=ts.Layout->Size.x; uint16_t MAX_Y=ts.Layout->Size.y;

    int16_t cross[5][4] =
    {
            {      20,      20,0,0},
            {MAX_X-20,      20,0,0},
            {      20,MAX_Y-20,0,0},
            {MAX_X-20,MAX_Y-20,0,0},
            { MAX_X/2, MAX_Y/2,0,0},
    };

    //reset calibration coefficients before acquiring points
    for(int16_t m=0; m<6; m++)
    {
        ts.tp->cal[m]=0;
    }

    ts.tp->cal[0]=65536;
    ts.tp->cal[4]=65536;

    for (int16_t idx = 0; idx < 5; idx++)
    {
        UiDriver_DoCrossCheck(cross[idx]);
    }

    //calibration algorithm based on publication:
    //"Calibration in touch-screen systems" Texas Instruments
    //Analog Applications Journal 3Q 2007

    /*//test vectors
    int16_t cross[0][4] = {     128,     384,1698,2258};
    int16_t cross[1][4] = {      64,     192, 767,1149};
    int16_t cross[2][4] = {     192,     192,2807,1327};
    int16_t cross[3][4] = {     192,     576,2629,3367};
    int16_t cross[4][4] = {      64,     576, 588,3189};*/

    //matrices field definitions
    float mA[3*5];
    float mAT[3*5];
    float mATAinv[3*3];
    float mbuff[3*3];
    float mcom[3*5];
    float mX[5];
    float mY[5];
    float mABC[3];
    float mDEF[3];

    //matrix data init
    for (int m=0; m < 5; m++)
    {
        mA[3*m+0]=cross[m][2];
        mA[3*m+1]=cross[m][3];
        mA[3*m+2]=1.0;
        mX[m]= cross[m][0];
        mY[m]= cross[m][1];
    }

    //create matrices instances
    arm_matrix_instance_f32 m_A,m_AT,m_ATAinv,m_X,m_Y,m_ABC,m_DEF,m_buff,m_com;

    //init of matrices
    arm_mat_init_f32(&m_A,5,3,mA);
    arm_mat_init_f32(&m_AT,3,5,mAT);
    arm_mat_init_f32(&m_ATAinv,3,3,mATAinv);
    arm_mat_init_f32(&m_X,5,1,mX);
    arm_mat_init_f32(&m_Y,5,1,mY);
    arm_mat_init_f32(&m_ABC,3,1,mABC);
    arm_mat_init_f32(&m_DEF,3,1,mDEF);
    arm_mat_init_f32(&m_buff,3,3,mbuff);
    arm_mat_init_f32(&m_com,3,5,mcom);

    //real computation
    arm_mat_trans_f32(&m_A,&m_AT);           //A^T           size 5x3 -> 3x5
    arm_mat_mult_f32(&m_AT,&m_A,&m_buff);        //A^T x A   size 3x5 * 5x3 -> 3x3
    arm_mat_inverse_f32(&m_buff,&m_ATAinv);  //(A^T x A)^-1  size 3x3
    arm_mat_mult_f32(&m_ATAinv,&m_AT,&m_com);//(A^T x A)^-1 x A^T   m_com is common matrix for estimating coefficients for X and Y      size 3x3 * 3x5 -> 3x5

    arm_mat_mult_f32(&m_com,&m_X,&m_ABC);   //calculating the coefficients for X data    size 3x5 * 5x1  -> 3x1
    arm_mat_mult_f32(&m_com,&m_Y,&m_DEF);   //calculating the coefficients for Y data    size 3x5 * 5x1  -> 3x1

    //store cal parameters
    for (int m=0; m < 3; m++)
    {
        ts.tp->cal[m]=mABC[m]*65536;
        ts.tp->cal[m+3]=mDEF[m]*65536;
    }
}

static bool UiDriver_TouchscreenCalibration()
{
	bool retval = false;
	uint16_t MAX_X=ts.Layout->Size.x; uint16_t MAX_Y=ts.Layout->Size.y;

    bool run_calibration = false;

    const uint32_t clr_bg = Black;
    const uint32_t clr_fg = White;

    Keypad_Scan();

    //if (UiDriver_IsButtonPressed(TOUCHSCREEN_ACTIVE) && UiDriver_IsButtonPressed(BUTTON_F5_PRESSED))
    if (UiDriver_IsButtonPressed(TOUCHSCREEN_ACTIVE))
    {
        //wait for a moment to filter out some unwanted spikes
        HAL_Delay(500);
        Keypad_Scan();

        if(UiDriver_IsButtonPressed(TOUCHSCREEN_ACTIVE))
        {

            UiLcdHy28_LcdClear(clr_bg);

            if (ts.tp->present)
            {
                // now do all of the warnings, blah, blah...
                UiLcdHy28_PrintTextCentered(2,05,MAX_X-4,"TOUCH CALIBRATION",clr_fg,clr_bg,1);
                UiLcdHy28_PrintTextCentered(2, 70, MAX_X-4, "If you don't want to do this\n"
                        "press POWER button to start normally.\n"
                        " Settings will be saved at POWEROFF"
                        ,clr_fg,clr_bg,0);

                // delay a bit...
                HAL_Delay(3000);

                // add this for emphasis
                UiLcdHy28_PrintTextCentered(2, 195, MAX_X-4, "Press BAND+ and BAND-\n"
                        "to start calibration",clr_fg,clr_bg,0);

                UiDriver_WaitForBandMAndBandPorPWR();

                if (UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED))
                {
                    UiLcdHy28_LcdClear(Black);							// clear the screen
                    UiLcdHy28_PrintTextCentered(2,108,MAX_X-4,"      ...performing normal start...",White,Black,0);
                    HAL_Delay(3000);
                }
                else
                {
                    run_calibration = true;
                }
            }
            else
            {
                UiLcdHy28_PrintTextCentered(2,05,MAX_X-4,"TOUCHSCREEN ERROR",clr_fg,clr_bg,1);
                UiLcdHy28_PrintTextCentered(2, 70, MAX_X-4, "A touchscreen press was detected\n"
                        "but no touchscreen controller found\n"
                        "Calibration cannot be executed!"
                        ,clr_fg,clr_bg,0);
                // delay a bit...
                HAL_Delay(3000);
            }
		}
	}

	if (run_calibration)
	{
	    UiLcdHy28_LcdClear(clr_bg);
	    UiLcdHy28_PrintTextCentered(2,70, MAX_X-4,
	            "On the next screen crosses will appear.\n"
	            "Touch as exact as you can on the middle\n"
	            "of each cross. After three valid\n"
	            "samples position of cross changes.\n"
	            "Repeat until the five test positions\n"
	            "are finished.",clr_fg,clr_bg,0);

	    UiLcdHy28_PrintTextCentered(2,195,MAX_X-4,"Touch at any position to start.",clr_fg,clr_bg,0);

 	    UiDriver_WaitForButtonPressed(TOUCHSCREEN_ACTIVE);

	    UiLcdHy28_LcdClear(clr_bg);
	    UiLcdHy28_PrintTextCentered(2,100,MAX_X-4,"Wait one moment please...",Yellow,clr_bg,0);
	    HAL_Delay(1000);

	    UiDriver_TouchscreenCalibrationRun();

	    UiLcdHy28_LcdClear(clr_bg);

#ifdef Touch_ShowTestscreen
	    UiLcdHy28_PrintTextCentered(2, 195, MAX_X-4, "Press BAND+ and BAND-\n"
	            "to run drawing on screen\n"
	            "or POWER to boot",clr_fg,clr_bg,0);

	    UiDriver_WaitForBandMAndBandPorPWR();

	    if (UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED))
	    {
	        UiLcdHy28_LcdClear(Black);                          // clear the screen
	        UiLcdHy28_PrintTextCentered(2,108,MAX_X-4,"      ...performing normal start...",White,Black,0);
	    }
	    else
        {
            UiLcdHy28_LcdClear(clr_bg);
            UiLcdHy28_PrintTextCentered(2, MAX_Y/2-8, MAX_X-4, "Test screen.\n"
                    "You can draw by pressing the screen.\n"
                    "Press Power to boot",clr_fg,clr_bg,0);
            while(1)
            {
                do
                {
                    HAL_Delay(10);
                    Keypad_Scan();
                } while (UiDriver_IsButtonPressed(TOUCHSCREEN_ACTIVE) == false && UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED) == false);

                if(UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED) == true)
                {
                    UiLcdHy28_LcdClear(Black);                          // clear the screen
                    UiLcdHy28_PrintTextCentered(2,108,MAX_X-4,"      ...performing normal start...",White,Black,0);
                    break;
                }

                if (UiLcdHy28_TouchscreenHasProcessableCoordinates())
                {
                    //          *xt_corr += (ts.tp->hr_x - cross[0]);
                    //          *yt_corr += (ts.tp->hr_y - cross[1]);
                    UiLcdHy28_DrawColorPoint(ts.tp->hr_x,ts.tp->hr_y,White);
                }

            }
        }
#endif
	    HAL_Delay(2000);
	    retval = true;
	    ts.menu_var_changed = true;
	}
	return retval;
}

#define CrossCheckCount 3
void UiDriver_DoCrossCheck(int16_t cross[])
{
	uint16_t MAX_X=ts.Layout->Size.x;
	uint32_t clr_fg, clr_bg;
	clr_bg = Black;
	clr_fg = White;

	UiLcdHy28_LcdClear(clr_bg);
	DrawCross(cross,clr_fg);

	char txt_buf[40];
	uchar datavalid = 0, samples = 0;

	int16_t* xt_corr=&cross[2];
	int16_t* yt_corr=&cross[3];

	*xt_corr=0;
	*yt_corr=0;

	do
	{
	    UiDriver_WaitForButtonPressed(TOUCHSCREEN_ACTIVE);

		if (UiLcdHy28_TouchscreenHasProcessableCoordinates())
		{
			//if(abs(ts.tp->hr_x - cross[0]) < MaxTouchError && abs(ts.tp->hr_y - cross[1]) < MaxTouchError)
			//{
				datavalid++;
				*xt_corr += ts.tp->hr_x;
				*yt_corr += ts.tp->hr_y;
				clr_fg = Green;
				snprintf(txt_buf,40,"Try (%d) error: x = %+d / y = %+d",datavalid,ts.tp->hr_x-cross[0],ts.tp->hr_y-cross[1]);	//show misajustments
			/*}
			else
			{
				clr_fg = Red;
				snprintf(txt_buf,40,"Try (%d) BIG error: x = %+d / y = %+d",samples,ts.tp->hr_x-cross[0],ts.tp->hr_y-cross[1]);	//show misajustments
			}*/
			samples++;
			UiLcdHy28_PrintTextCentered(2,70,MAX_X-4,txt_buf,clr_fg,clr_bg,0);

			snprintf(txt_buf,40,"RAW: x = %+d / y = %+d",ts.tp->xraw,ts.tp->yraw);	//show misajustments
			UiLcdHy28_PrintTextCentered(2,85,MAX_X-4,txt_buf,clr_fg,clr_bg,0);
			ts.tp->state = TP_DATASETS_PROCESSED;
		}
	}
	while(datavalid < CrossCheckCount);

	UiLcdHy28_PrintTextCentered(2,100,MAX_X-4,"Wait one moment please...",Yellow,clr_bg,0);

	*xt_corr/=CrossCheckCount; //average the data
	*yt_corr/=CrossCheckCount;

	HAL_Delay(2000);
}


static uint16_t startUpScreen_nextLineY;
static bool startUpError = false;

/**
 * @brief use this method to report initialization problems on splash screen, may only be used during splash screen presence (!)
 *
 * @param isProblem if set to true, the problem is reported on screen, otherwise nothing is done
 * @param txt pointer to a text string characterizing the problem detected
 *
 */
void UiDriver_StartupScreen_LogIfProblem(bool isProblem, const char* txt)
{
	if (isProblem)
	{
		startUpScreen_nextLineY = UiLcdHy28_PrintTextCentered(ts.Layout->StartUpScreen_START.x,startUpScreen_nextLineY,320,txt,Black,Red3,0);
		startUpError = true;
	}
}

static uint16_t fw_version_number_major = 0;    // save new F/W version
static uint16_t fw_version_number_release = 0;
static uint16_t fw_version_number_minor = 0;

/**
 * @returns true if the firmware version is different from version in loaded configuration settings.
 */
static bool UiDriver_FirmwareVersionCheck(void)
{

	fw_version_number_major = atoi(UHSDR_VER_MAJOR);    // save new F/W version
	fw_version_number_release = atoi(UHSDR_VER_RELEASE);
	fw_version_number_minor = atoi(UHSDR_VER_MINOR);

	return ((ts.version_number_major != fw_version_number_major) || (ts.version_number_release != fw_version_number_release) || (ts.version_number_minor != fw_version_number_minor));        // Yes - check for new version
}
/**
 * @brief basically does nothing but settiSng the firmware number of configuration to number of running fw
 */
static void UiDriver_FirmwareVersionUpdateConfig(void)
{

	if (UiDriver_FirmwareVersionCheck())
	{
		ts.version_number_major = fw_version_number_major;    // save new F/W version
		ts.version_number_release = fw_version_number_release;
		ts.version_number_minor = fw_version_number_minor;

	}
}


/**
 * @brief show initial splash screen, we run AFTER the configuration storage has been initialized!
 * @param hold_time how long the screen to be shown before proceeding (in ms)
 */
void UiDriver_StartUpScreenInit()
{
	char   tx[100];
	uint32_t clr;
	// Clear all
	UiLcdHy28_LcdClear(Black);
	uint16_t nextY = ts.Layout->StartUpScreen_START.y;
	uint16_t posX  = ts.Layout->StartUpScreen_START.x;
#ifndef SDR_AMBER
	snprintf(tx,100,"%s",DEVICE_STRING);
#else
	snprintf(tx,100,"%s","AMBER Transceiver");
#endif
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY, 320, tx, Cyan, Black, 1);

#ifdef TRX_HW_LIC
	snprintf(tx,100,"Hardware License: %s",TRX_HW_LIC);
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY + 3, 320, tx, White,Black, 0);
#endif
#ifdef TRX_HW_CREATOR
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY, 320, TRX_HW_CREATOR, White,Black, 0);
#endif
#ifdef SDR_AMBER
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY, 320, "UB8JDC Korenev Yuri, ub8jdc@mail.ru", White,Black, 0);
#endif

	snprintf(tx,100,"%s%s","UHSDR Vers. ",UiMenu_GetSystemInfo(&clr,INFO_FW_VERSION));
//	nextY = UiLcdHy28_PrintTextCentered(ts.Layout->StartUpScreen_START.x, nextY + 8, 320, tx, Yellow, Black, 1);
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY + 8, 320, tx, Yellow, Black, 0);
	nextY = UiLcdHy28_PrintTextCentered(posX, nextY + 3, 320, "Firmware License: " UHSDR_LICENCE "\n" UHSDR_REPO, White, Black, 0);

	// show important error status
	startUpScreen_nextLineY = nextY + 8; // reset y coord to first line of error messages

	UiLcdHy28_BacklightEnable(true);

}

void UiDriver_StartUpScreenFinish() {
	const char* txp;
	char   tx[100];
	uint32_t clr, fg_clr;
	uint32_t hold_time;

	bool osc_present_problem = osc->isPresent() == false;

	UiDriver_StartupScreen_LogIfProblem(osc_present_problem, "Local Oscillator NOT Detected!");

	if (!Si5351a_IsPresent() && RadioManagement_TcxoIsEnabled()) {
		UiDriver_StartupScreen_LogIfProblem(lo.sensor_present == false, "MCP9801 Temp Sensor NOT Detected!");
	}

	if(ts.configstore_in_use == CONFIGSTORE_IN_USE_ERROR) {
#ifdef USE_CONFIGSTORAGE_FLASH
	    UiDriver_StartupScreen_LogIfProblem(ts.ee_init_stat != HAL_OK, "Config Flash Error");
#endif
	    if (SerialEEPROM_eepromTypeDescs[ts.ser_eeprom_type].size == 0) {
	        snprintf(tx,100,"Config EEPROM: %s", SerialEEPROM_eepromTypeDescs[ts.ser_eeprom_type].name);
	        UiDriver_StartupScreen_LogIfProblem(true, tx);
	    }
	}

	if (!Si5351a_IsPresent()) {
	    UiDriver_StartupScreen_LogIfProblem(
	      (HAL_ADC_GetValue(&hadc2) > MAX_VSWR_MOD_VALUE) && (HAL_ADC_GetValue(&hadc3) > MAX_VSWR_MOD_VALUE),
	      "SWR Bridge resistor mod NOT completed!"
	    );
	}

	// we report this problem only if we are theoretically able to transmit
	// and tx was not disabled such as in a RX only device
    if (RadioManagement_IsTxDisabled() == false && osc_present_problem == false) {
        bool pa_bias_problem = ts.pa_bias == 0;

        UiDriver_StartupScreen_LogIfProblem(
            pa_bias_problem,
            "PA Bias is 0, TX not possible");
    }

	if (UiDriver_FirmwareVersionCheck()) {
		hold_time = 10000; // 10s
		txp = "Firmware change detected!\nPlease review settings!";
		startUpScreen_nextLineY = UiLcdHy28_PrintTextCentered(ts.Layout->StartUpScreen_START.x,startUpScreen_nextLineY + 10,320,txp,White,Black,0);

		UiDriver_FirmwareVersionUpdateConfig();
	}

	if (startUpError == true) {
		hold_time = 15000; // 15s
		txp = "Boot Delay because of Errors or Warnings";
		clr = Red3;
		fg_clr = Black;
	} else {
		hold_time = 3000; // 3s
		txp = "...starting up normally...";
		clr =  Black;
		fg_clr = Green;
	}

	UiLcdHy28_PrintTextCentered(
	    ts.Layout->StartUpScreen_START.x,
	    startUpScreen_nextLineY + 10,
	    320,
	    txp,
	    fg_clr, clr,
	    0
	 );

	HAL_Delay(hold_time);

#ifdef USE_DISP_480_320
	ts.Layout = &LcdLayouts[LcdLayout_480x320];
#else
    ts.Layout = &LcdLayouts[LcdLayout_320x240];
#endif

	if (!(ts.expflags1 & EXPFLAGS1_TUNE_HELPER_DEFENABLE)) {
		cw_decoder_config.snap_enable = true;
	}

	UiDriver_CreateDesktop();
	AudioDriver_SetFade(1.0f / 10000.0f);
}

// UiAction_... are typically small functions to execute a specific ui function initiate by a key press or touch event
// they take no argument and return nothing. If possible, try to keep the function atomic and independent from the
// key or touch region it is assigned to. I.e. it is better not to implement 2 functions based on menu mode or not here
// this logic should done separately so that the resulting action is reusable in different activation scenarios (touch or key or even CAT)
// If execution of an action is not applicable all the times it is necessary to check if the function should check that
// by itself and become a "No operation" or even issues a message, or if this is to be implicitly handled by the use of the
// function only in certain modes of operation through the modes tables.

// TODO: Make Atomic
static void UiAction_ChangeSnap(void) {
	sc.snap = 1;
}

/**
 *
 * @param dsp_mode a valid dsp mode id
 * @return true if dsp_mode is currently available, false otherwise
 */
static bool UiDriver_IsDspModePermitted(dsp_mode_t dsp_mode) {
    bool neg_retval = (dsp_mode >= DSP_SWITCH_MAX);

    // prevent NR AND NOTCH or NOTCH, when in CW
    neg_retval |= ts.dmod_mode == DEMOD_CW && ( dsp_mode == DSP_SWITCH_NR_AND_NOTCH || dsp_mode == DSP_SWITCH_NOTCH);

    // prevent NR AND NOTCH, when in AM and decimation rate equals 2 --> high CPU load)
    neg_retval |= (dsp_mode == DSP_SWITCH_NR_AND_NOTCH) && (ts.dmod_mode == DEMOD_AM) && (ads.decimated_freq >= 24000);

    // prevent using a mode not enabled in the dsp mode selection (i.e. user configured it to be not used, although available)
    //neg_retval |= (ts.dsp.mode_mask & (1<<dsp_mode)) == 0;

    // not forbidden, so return true;
    return neg_retval == false;
}

/**
 * Request a dsp mode to be activated. If the request mode is not valid, try to find the "next" valid one. In worst case this
 * is "DSP OFF" which is always permitted.
 * @param new_dsp_mode request new dsp mode, can be any value, too large values cause starting at lowest mode
 */
void UiDriver_UpdateDSPmode(uint8_t new_dsp_mode) {
	//loop for detection of first possible DSP function to switch it on if others are disabled/not allowed

    for (int i=0; i < DSP_SWITCH_MAX; i++) {
		if (new_dsp_mode >= DSP_SWITCH_MAX) {
		    new_dsp_mode = DSP_SWITCH_OFF; // flip round
		}

		if (UiDriver_IsDspModePermitted(new_dsp_mode)) {
		    ts.dsp.mode = new_dsp_mode;
		    break;
		} else {
		    // try next mode
		    new_dsp_mode++;
		}
	}

	switch (ts.dsp.mode) {
        case DSP_SWITCH_OFF:
            ts.dsp.active =  DSP_OFF;
            break;

        case DSP_SWITCH_NR:
            ts.dsp.active =  DSP_NR_ENABLE;
            break;

        case DSP_SWITCH_ANR:
            ts.dsp.active =  DSP_ANR_ENABLE;
            break;

        case DSP_SWITCH_NOTCH:
            ts.dsp.active =  DSP_NOTCH_ENABLE;
            break;

        case DSP_SWITCH_NR_AND_NOTCH:
            ts.dsp.active =  DSP_NOTCH_ENABLE | DSP_NR_ENABLE;
            break;

        case DSP_SWITCH_NOTCH_MANUAL:
            ts.dsp.active =  DSP_MNOTCH_ENABLE;
            break;

        case DSP_SWITCH_PEAK_FILTER:
            ts.dsp.active =  DSP_MPEAK_ENABLE;
            break;

        default:
            break;
	}

	ts.dsp.active_toggle = ts.dsp.active;  // save update in "toggle" variable
	// reset DSP NR coefficients
	AudioDriver_SetProcessingChain(ts.dmod_mode, true);        // update DSP/filter settings
	UiDriver_DisplayDSPMode();
}

static void UiAction_ChangeToNextDspMode(void) {
	if(ts.dmod_mode != DEMOD_FM) {	  // allow selection/change of DSP only if NOT in FM
		//
		// I think we should alter this to use a counter
		// What do we want to switch here:
		// NR ON/OFF		ts.dsp.active |= DSP_NR_ENABLE;	 // 	ts.dsp.active &= ~DSP_NR_ENABLE;
	    // ANR ON/OFF		ts.dsp.active |= DSP_ANR_ENABLE;
		// NOTCH ON/OFF		ts.dsp.active |= DSP_NOTCH_ENABLE; // 	ts.dsp.active &= ~DSP_NOTCH_ENABLE;
		// Manual Notch		ts.dsp.active |= DSP_MNOTCH_ENABLE
		// BASS				ts.bass // always "ON", gain ranges from -20 to +20 dB, "OFF" = 0dB
		// TREBLE			ts.treble // always "ON", gain ranges from -20 to +20 dB, "OFF" = 0dB

		UiDriver_UpdateDSPmode(ts.dsp.mode + 1);
	}
}

void UiAction_ChangeSpectrumSize() {
	ts.menu_var_changed = 1;

    if (ts.spectrum_size == SPECTRUM_BIG) {
        ts.spectrum_size = SPECTRUM_NORMAL;
    } else {
        ts.spectrum_size = SPECTRUM_BIG;
    }

    UiDriver_SpectrumChangeLayoutParameters();
}

void UiAction_ChangeSpectrumZoomLevelDown() {
	if (sd.magnify > MAGNIFY_MIN) {
	    decr_wrap_uint8(&sd.magnify, MAGNIFY_MIN, MAGNIFY_MAX);
	    UiDriver_SpectrumChangeLayoutParameters();
	}
}

void UiAction_ChangeSpectrumZoomLevelUp() {
	if (sd.magnify < MAGNIFY_MAX) {
	    incr_wrap_uint8(&sd.magnify, MAGNIFY_MIN, MAGNIFY_MAX);
	    UiDriver_SpectrumChangeLayoutParameters();
	}
}

void UiAction_ChangeFrequencyToNextKhz() {
	df.tune_new = floor(df.tune_new / 1000) * 1000;	// set last three digits to "0"
	UiDriver_FrequencyUpdateLOandDisplay(true);
}

void UiAction_ToggleWaterfallScopeDisplay() {
    SpectrumMode_t temp = UiDriver_GetSpectrumMode();

    if (temp != SPECTRUM_BLANK) {
        // we want range 0 - 2 instead of the normal 1 - 3
        temp--;
    }

    temp++;
    temp%=3;
    temp++;
    UiDriver_SetSpectrumMode(temp);
    UiSpectrum_ResetSpectrum();
    UiSpectrum_Init();   // init spectrum display
}

void UiAction_ChangeDemodMode() {
	if ((!ts.tune) && (ts.txrx_mode == TRX_MODE_RX)) {	 	// do NOT allow mode change in TUNE mode or transmit mode
        UiDriver_ChangeToNextDemodMode(0);
        UiDriver_HandlePowerLevelChange(ts.band_effective, ts.power_level);
	}
}

void UiAction_ChangeDemodModeToAlternativeMode() {
	if((!ts.tune) && (ts.txrx_mode == TRX_MODE_RX)) {	 	// do NOT allow mode change in TUNE mode or transmit mode
		UiDriver_ChangeToNextDemodMode(1);
	}
}

void UiAction_ChangePowerLevel() {
    uint8_t pl = ts.power_level;

    incr_wrap_uint8(&pl, 0, mchf_power_levelsInfo.count - 1);
	UiDriver_HandlePowerLevelChange(ts.band_effective, pl);
}

void UiAction_ChangePowerLevelToFull() {
	UiDriver_HandlePowerLevelChange(ts.band_effective, 0);
}

// TODO: Decide if we really want to switch
// order like for the normal buttons
void UiAction_ChangeBandDownOrUp() {
//	UiDriver_HandleBandButtons(BUTTON_BNDM);
	if(ts.expflags1 & EXPFLAGS1_BANDCH_JUMP_SWAP) {
	    bool buttondirSwap = (ts.flags1 & FLAGS1_SWAP_BAND_BTN)?true:false;
	    if (!buttondirSwap) { df.tune_new -= 48000 / (1 << sd.magnify); }
	    else { df.tune_new += 48000 / (1 << sd.magnify); }
	} else {
		UiDriver_HandleBandButtons(BUTTON_BNDM);
	}
}

void UiAction_ChangeBandUpOrDown() {
	if (ts.expflags1 & EXPFLAGS1_BANDCH_JUMP_SWAP) {
	    bool buttondirSwap = (ts.flags1 & FLAGS1_SWAP_BAND_BTN)?true:false;

	    if (!buttondirSwap) {
	        df.tune_new += 48000 / (1 << sd.magnify);
	    } else {
	        df.tune_new -= 48000 / (1 << sd.magnify);
	    }
	} else {
		UiDriver_HandleBandButtons(BUTTON_BNDP);
	}
}

static void UiAction_SaveConfigurationToMemory(void) {
	if(ts.txrx_mode == TRX_MODE_RX) {	 				// only allow EEPROM write in receive mode
	    if (ts.menu_var_changed != 0) {
            UiDriver_DisplayMessageStart();
            UiDriver_SaveConfiguration();
            HAL_Delay(3000);
            UiDriver_DisplayMessageStop();

            ts.menu_var_changed = 0;                    // clear "EEPROM SAVE IS NECESSARY" indicators
            UiDriver_DisplayFButton_F1MenuExit();
        }
	}
}

static void UiDriver_DrawGraticule_Rect(bool show) {
	uint16_t pos_y = sd.Slayout->graticule.y;

	if (show) {
		UiLcdHy28_PrintText(
		    sd.Slayout->graticule.x+5,
		    pos_y+4,
		    "CHOOSE NEW POSITION",
		    White, Black,
		    4
		);

		UiLcdHy28_PrintText(
		    sd.Slayout->graticule.x+sd.Slayout->graticule.w-65,
		    pos_y+4,
		    "| OK |",
		    Green, Black,
		    4
		);

		UiLcdHy28_PrintText(
		    sd.Slayout->graticule.x+sd.Slayout->graticule.w-20,
		    pos_y+4, "X",
		    Orange, Black,
		    4
		);

		UiLcdHy28_DrawEmptyRect(
		    sd.Slayout->graticule.x+1,
		    pos_y+1,
		    sd.Slayout->graticule.h-3,
		    sd.Slayout->graticule.w-3,
		    White
		);
	} else {
		UiLcdHy28_DrawFullRect(
		   sd.Slayout->graticule.x+1,
		   pos_y+1,
		   sd.Slayout->graticule.h-2,
		   sd.Slayout->graticule.w-2,
		   Black
		);
	}
}

/*
 * Special actions for long pressed spectrum/waterfall area
 */
void UiAction_CheckSpectrumTouchActions() {
	if (UiDriver_CheckTouchRegion(&sd.Slayout->graticule) && ts.txrx_mode == TRX_MODE_RX) {
		if (ts.SpectrumResize_flag == 0) {
			UiSpectrum_Clear();
			UiDriver_DrawGraticule_Rect(true);		//draw new graticule control

			UiLcdHy28_DrawEmptyRect(
			    sd.Slayout->full.x,
			    sd.Slayout->full.y,
			    sd.Slayout->full.h-1,
			    sd.Slayout->full.w-1,
			    White
			);

			ts.SpectrumResize_flag=1;
			return;
		}
	}
}

void UiAction_ZoomResetToOne()
{
	sd.magnify = MAGNIFY_MIN;
	UiDriver_SpectrumChangeLayoutParameters();
}

//SpectrumVirtualKeys_flag

void UiAction_ChangeFrequencyByTouch()
{
	if (ts.frequency_lock == false)
	{
		int step = 500;				// adjust to 500Hz

		if(sd.magnify == 3)
		{
			step = 100;					// adjust to 100Hz
		}
		if(sd.magnify == 4)
		{
			step = 10;					// adjust to 10Hz
		}
		if(sd.magnify == 5)
		{
			step = 1;					// adjust to 1Hz
		}
		if(ts.dmod_mode == DEMOD_AM || ts.dmod_mode == DEMOD_SAM)
		{
			step = 5000;				// adjust to 5KHz
		}

		//int16_t line =sd.marker_pos[0] + UiSpectrum_GetSpectrumStartX();
		int16_t line =sd.marker_pos[0] + sd.Slayout->scope.x;

		/*int16_t line =sd.rx_carrier_pos;
		if(ts.dmod_mode == DEMOD_CW)
		{
			line=sd.marker_pos[0];
		}*/

		//uint32_t tunediff = ((IQ_SAMPLE_RATE/slayout.scope.w)/(1 << sd.magnify))*(ts.tp->hr_x-line);
		int32_t tunediff = sd.hz_per_pixel*(ts.tp->hr_x-line);
		df.tune_new = lround((df.tune_new + tunediff)/step) * step;
		UiDriver_FrequencyUpdateLOandDisplay(true);
	}
}

void UiAction_ChangeDigitalMode() {
	incr_wrap_uint8(&ts.digital_mode, 0, DigitalMode_Num_Modes - 1);
	UiDriver_ToggleDigitalMode();
}

void UiAction_ChangeDynamicTuning()
{
	if (!(ts.flags1 & FLAGS1_DYN_TUNE_ENABLE))			// is it off??
	{
		ts.flags1 |= FLAGS1_DYN_TUNE_ENABLE;	// then turn it on
	}
	else
	{
		ts.flags1 &= ~FLAGS1_DYN_TUNE_ENABLE;	// then turn it off
	}

	UiDriver_DisplayFreqStepSize();
}

void UiAction_ChangeRXInputStateDown()
{
#ifdef SDR_AMBER
	if (ts.amber_input_state < 3)
	{
		ts.amber_input_state ++;
		if(ts.amber_io8_present)
		{
		    Board_Amber_InputStateSet(ts.amber_input_state);
		}
	}
#endif
}

void UiAction_ChangeRXInputStateUp()
{
#ifdef SDR_AMBER
	if (ts.amber_input_state > 0)
	{
		ts.amber_input_state --;
        if(ts.amber_io8_present)
        {
            Board_Amber_InputStateSet(ts.amber_input_state);
        }
	}
#endif
}

void UiAction_ChangeDebugInfoDisplay()
{
	UiDriver_DebugInfo_DisplayEnable(!ts.show_debug_info);
}

static void UiAction_ChangeFilterBW(void)
{
	if(!ts.tune)
	{
		if (filter_path_change == true)
		{
			filter_path_change = false;
		}
		else
		{
			if (ts.expflags2 & EXPFLAGS2_TEMP_MEM_BW_MOD_BAND) // enabled temp. remember BW on mode & band
			{
				UiDriver_StoreBwData();
			}

			AudioFilter_NextApplicableFilterPath(PATH_USE_RULES,AudioFilter_GetFilterModeFromDemodMode(ts.dmod_mode),ts.filter_path);
			// we store the new filter in the current active filter location
			AudioDriver_SetProcessingChain(ts.dmod_mode, false);
			// we activate it (in fact the last used one, which is the newly selected one);
		}
		// Change filter
		UiDriver_UpdateDisplayAfterParamChange();		// re-init for change of filter including display updates
		// FIXME UiDriver_DisplayEncoderThreeMode();
	}
}

static void UiAction_ChangeTuningStepDownOrUp(void)
{
	UiDriver_ChangeTuningStep(ts.freq_step_config & FREQ_STEP_SWAP_BTN? true: false);
}

static void UiAction_ChangeTuningStepUpOrDown(void)
{
	UiDriver_ChangeTuningStep(ts.freq_step_config & FREQ_STEP_SWAP_BTN? false: true);
}

static void UiAction_ChangeBacklightBrightness(void)
{
	incr_wrap_uint8(&ts.lcd_backlight_brightness,LCD_DIMMING_LEVEL_MIN,LCD_DIMMING_LEVEL_MAX);
}

static void UiAction_ToggleTxDisable(void)
{
	if(ts.txrx_mode == TRX_MODE_RX)			// do NOT allow mode change in TUNE mode or transmit mode
	{
		if( RadioManagement_IsTxDisabledBy(TX_DISABLE_USER) == false)
		{
			ts.tx_disable |= TX_DISABLE_USER;
		}
		else
		{
			ts.tx_disable &= ~TX_DISABLE_USER;
		}
		UiDriver_FButton_F5Tune();
		UiDriver_DisplayPowerLevel();
	}
}

static void UiAction_ToggleVfoMem(void)
{
	RadioManagement_ToggleVfoMem();
	UiDriver_FButton_F3MemSplit();
	if(ts.mem_disp)
	{
	    ts.mem_disp = false;
	    UiSpectrum_Init();
	}
}

static void UiAction_GoToMainFmenu(void)
{
	if(ts.ExtFMenuActive)
	{
		ts.ExtFMenuActive = false;
		UiDriver_CreateFunctionButtons(false);
	}
}

// TODO: Split into separate actions and a composition
static void UiAction_ChangeRxFilterOrFmToneBurst(void)
{
	if((!ts.tune) && (ts.txrx_mode == TRX_MODE_RX))	  // only allow in receive mode and when NOT in FM
	{
		filter_path_change = true;
		UiDriver_DisplayFilter();
	}
	else if((ts.txrx_mode == TRX_MODE_TX) && (ts.dmod_mode == DEMOD_FM))
	{
		if(ts.fm_tone_burst_mode != FM_TONE_BURST_OFF)	 	// is tone burst mode enabled?
		{
			ads.fm_conf.tone_burst_active = 1;					// activate the tone burst
			ts.fm_tone_burst_timing = ts.sysclock + FM_TONE_BURST_DURATION;	// set the duration/timing of the tone burst
		}
	}
}

static void UiAction_ToggleNoiseblanker(void)
{
	ts.dsp.active ^= DSP_NB_ENABLE;	// toggle whether or not DSP or NB is to be displayed
	UiDriver_DisplayEncoderTwoMode();
}

static void UiAction_ToggleTuneMode(void)
{
	ts.tune = RadioManagement_Tune(!ts.tune);
	UiDriver_DisplayPowerLevel();           // tuning may change power level temporarily
	UiDriver_FButton_F5Tune();
}

static void UiAction_PlayKeyerBtnN(int8_t n)
{
	uint8_t* pmacro;
	uint16_t c = 0;

	if (ts.keyer_mode.button_recording == KEYER_BUTTON_NONE)
	{
		pmacro = ( uint8_t* )ts.keyer_mode.macro[n];
		if (*pmacro != '\0') // If there is a macro
		{
		    if((ts.expflags1 & EXPFLAGS1_FAST_MACRO_1_2) && ((n == KEYER_BUTTON_1) || (n == KEYER_BUTTON_2)))
		    {
		        ts.cw_keyer_speed_bak = ts.cw_keyer_speed;
		        ts.cw_keyer_speed = CW_KEYER_SPEED_MAX;
		        CwGen_SetSpeed();
		    }
		    if(ts.keyer_mode_tx == CYCLED || ts.keyer_mode_tx == BEACON)
		    {
                DigiModes_TxBufferReset();
		    }
			while (*pmacro != '\0')
			{
				DigiModes_TxBufferPutChar( *pmacro++, UI );
			}
//			if (ts.buffered_tx)
			if (ts.keyer_mode_tx == TXRX_B)
			{
			    DigiModes_TxBufferPutChar( 0x04, UI );
			    // we put an EOT in the buffer, which tells the respective consumer
			    // to return to receive after transmitting all characters before this
			}

			if ((ts.dmod_mode == DEMOD_CW
			        && ts.cw_keyer_mode != CW_KEYER_MODE_STRAIGHT)
			        || is_demod_psk()
			        || is_demod_rtty())
			{
                if(ts.keyer_mode_tx == CYCLED || ts.keyer_mode_tx == BEACON)
                {
                    ts.keyer_cycled_tx_active = true;
                    ts.keyer_cycled_tx_button = n;
                }
                if(ts.keyer_mode_tx == BEACON)
                {
                    UiDriver_TextMsgClear();
                }
				RadioManagement_Request_TxOn();
			}
		}
		if(!(ts.dmod_mode == DEMOD_CW && !ts.cw_decoder_enable))
		{
		    UiDriver_TextMsgPutChar('>');
		}
	}
	else if (ts.keyer_mode.button_recording == n)
	{
		ts.cw_text_entry = false; // FIXME looks like we can remove cw_text_entry
		pmacro = (uint8_t *)ts.keyer_mode.macro[n];
		c = 0;
		/*
		 * Kind of trick. It will not check second condition if first is the false,
		 * so it keeps pointer to the last available element in array for macro
		 * to put there terminator
		 */
		while (( ++c <= KEYER_MACRO_LEN - 1 ) && DigiModes_TxBufferRemove( pmacro, UI ))
		{
		    pmacro++;
		}
		*pmacro = '\0';

		// strip out the spaces from the end of line
		while(( pmacro != ts.keyer_mode.macro[n] ) && *--pmacro == ' ' )
		{
			*pmacro = '\0';
		}

		UiConfiguration_UpdateMacroCap();
		if(!(ts.dmod_mode == DEMOD_CW && !ts.cw_decoder_enable))
		{
		    UiDriver_TextMsgPutChar('<');
		}
		ts.keyer_mode.button_recording = KEYER_BUTTON_NONE;

		// setup back the previous consumer of digi buffer
		// as UI has higher priority we can be sure that
		// it's not changed by changing modes
		DigiModes_Restore_BufferConsumer();
	}
	UiDriver_CreateFunctionButtons( false );
}

static void UiAction_PlayKeyerBtn1(void)
{
	UiAction_PlayKeyerBtnN(KEYER_BUTTON_1);
}

static void UiAction_PlayKeyerBtn2(void)
{
	UiAction_PlayKeyerBtnN(KEYER_BUTTON_2);
}

static void UiAction_PlayKeyerBtn3(void)
{
	UiAction_PlayKeyerBtnN(KEYER_BUTTON_3);
}

static void UiAction_PlayKeyerBtn4(void)
{
    UiAction_PlayKeyerBtnN(KEYER_BUTTON_4);
}

static void UiAction_RecordKeyerBtnN(int8_t n)
{
	if (ts.keyer_mode.button_recording == KEYER_BUTTON_NONE
	        && ts.txrx_mode == TRX_MODE_RX
	        && !ts.cw_text_entry
	        && ( ts.dmod_mode == DEMOD_CW
	        || is_demod_psk()
	        || is_demod_rtty()))
	{
		ts.cw_text_entry = true; // FIXME can be removed later

		ts.keyer_mode.button_recording = n;
		DigiModes_Set_BufferConsumer( UI );
		DigiModes_TxBufferReset();
		if(!(ts.dmod_mode == DEMOD_CW && !ts.cw_decoder_enable))
		{
		    UiDriver_TextMsgPutChar(':');
		}
		UiDriver_CreateFunctionButtons(false);
	}
}

static void UiAction_RecordKeyerBtn1(void)
{
	UiAction_RecordKeyerBtnN(KEYER_BUTTON_1);
}

static void UiAction_RecordKeyerBtn2(void)
{
	UiAction_RecordKeyerBtnN(KEYER_BUTTON_2);
}

static void UiAction_RecordKeyerBtn3(void)
{
	UiAction_RecordKeyerBtnN(KEYER_BUTTON_3);
}

static void UiAction_RecordKeyerBtn4(void)
{
    UiAction_RecordKeyerBtnN(KEYER_BUTTON_4);
}

static void UiAction_ToggleBufferedTXMode(void)
{
//	ts.buffered_tx = ! ts.buffered_tx;
	switch(ts.keyer_mode_tx)
	{
	case TXRX_U:
		ts.keyer_mode_tx = TXRX_B;
		break;
	case TXRX_B:
		ts.keyer_mode_tx = CYCLED;
		break;
	case CYCLED:
		ts.keyer_mode_tx = BEACON;
		break;
	case BEACON:
		ts.keyer_mode_tx = TXRX_U;
		ts.keyer_cycled_tx_active = false;
		break;
	}
	UiDriver_FButton_F5Tune();
}

static void UiAction_ToggleTxRx(void)
{
    ts.keyer_cycled_tx_active = false;
	if(ts.txrx_mode == TRX_MODE_RX)
	{
	    RadioManagement_Request_TxOn();
	}
	else
	{
	    RadioManagement_Request_TxOff();
        if(ts.keyer_mode_tx == CYCLED || ts.keyer_mode_tx == BEACON)
        {
        DigiModes_TxBufferReset();
        }
        if(ts.cw_keyer_speed != ts.cw_keyer_speed_bak)
        {
            ts.cw_keyer_speed = ts.cw_keyer_speed_bak;
            CwGen_SetSpeed();
        }
	}
	UiDriver_FButton_F5Tune();
}

static void UiDriver_CheckCycledKeyerTX(void)
{
    if(ts.keyer_cycled_tx_active)
    {
        if (ts.txrx_mode == TRX_MODE_TX)
        {
            ts.keyer_cycled_tx_timer = ts.sysclock + (ts.keyer_mode_tx == BEACON? ts.cw_beacon_interval: 100 * ts.keyer_cycled_interval); // by menu options
        }
        else // RX mode
        {
            if(ts.keyer_cycled_tx_timer < ts.sysclock)
            {
                UiAction_PlayKeyerBtnN(ts.keyer_cycled_tx_button);
                ts.keyer_cycled_tx_timer = ts.sysclock + 50; // for prevent double TX-ing of macro
            }
        }
    }
}

static void UiAction_ToggleSplitModeOrToggleMemMode(void)
{
	if(!ts.vfo_mem_flag)	 		// update screen if in VFO (not memory) mode
	{
		UiDriver_SetSplitMode(!is_splitmode());
	}
	else	 		// in memory mode
	{
		UiSpectrum_Clear();		// always clear display
		ts.mem_disp = !ts.mem_disp;
		if (ts.mem_disp == 0 )
		{
			UiSpectrum_Init();	// init spectrum scope
		}
		else
		{
		    UiAction_ShowMems();
		}
	}
}

static void UiAction_ToggleMenuMode(void) {
	if (!ts.mem_disp)	 			// allow only if NOT in memory display mode
	{
		if(ts.menu_mode == false)	 	// go into menu mode if NOT already in menu mode and not to halt on startup
		{
			ts.menu_mode = true;
			ts.encoder3state = filter_path_change;
			filter_path_change = false;			// deactivate while in menu mode
			UiDriver_DisplayFilter();
			UiSpectrum_Clear();

			UiDriver_DisplayFButton_F1MenuExit();
			UiDriver_DrawFButtonLabel(2,"PREV",Yellow);
			UiDriver_DrawFButtonLabel(3,"NEXT",Yellow);
			UiDriver_DrawFButtonLabel(4,"DEFLT",Yellow);

			// Grey out adjustments and put encoders in known states
			UiDriver_RefreshEncoderDisplay();

			ts.menu_var = 0;

			UiMenu_RenderMenu(MENU_RENDER_ONLY);	// Draw the menu the first time
			UiMenu_RenderMenu(MENU_PROCESS_VALUE_CHANGE);	// Do update of the first menu item
		}
		else	 	// already in menu mode - we now exit
		{
			ts.menu_mode = false;
			filter_path_change = ts.encoder3state;
			UiDriver_DisplayFilter();
			UiSpectrum_Init();			// init spectrum scope

			// Restore encoder displays to previous modes
			UiDriver_RefreshEncoderDisplay();
			UiDriver_DisplayFilter();	// update bandwidth display
			UiDriver_CreateFunctionButtons(false);
		}
	}
}

static void UiAction_EncoderHold(uint8_t enc) {
    if (ts.menu_mode) return;

    uint8_t state = ts.enc_state[enc] >> ENC_STATE_BITS;
    uint8_t mode = ts.enc_state[enc] & ENC_MODE_MASK;

    switch (state) {
        case ENC_STATE_NORM:
            state = ENC_STATE_TUNE_NORM;
            break;

        case ENC_STATE_CAROUSEL:
            state = ENC_STATE_TUNE_CAROUSEL;
            break;

        case ENC_STATE_TUNE_NORM:
            state = ENC_STATE_NORM;
            break;

        case ENC_STATE_TUNE_CAROUSEL:
            state = ENC_STATE_CAROUSEL;
            break;

        default:
            return;
    }

    ts.enc_state[enc] = (state << ENC_STATE_BITS) | mode;
    UiDriver_DisplayEncoderMode(enc);
}

static void UiAction_EncoderOneHold(void) {
    UiAction_EncoderHold(ENC1);
}

static void UiAction_EncoderTwoHold(void) {
    UiAction_EncoderHold(ENC2);
}

static void UiAction_EncoderThreeHold(void) {
    UiAction_EncoderHold(ENC3);
}

static void UiAction_ToggleKeyerMode(void)
{
//    if(ts.vfo_mem_flag) // wrong hole!
//    {
//        return;
//    }
//	ts.keyer_mode.active = !ts.keyer_mode.active;

    if(ts.menu_mode)
    {
        return;
    }

    if(ts.mem_disp || ts.xvtr_disp)
    {
        ts.mem_disp = false;
        ts.xvtr_disp = false;
        UiSpectrum_Clear();
        UiSpectrum_Init();
    }

    if(ts.expflags2 & EXPFLAGS2_FMENUS_SELECT)
    {
        ts.keyer_mode.active = false;
        ts.ExtFMenuActive = false;
        ts.vfo_mem_flag = false;
        ts.XvtrFMenuActive = false;
        ts.MenuOfFMenusActive = true;
    }
    else if(!ts.keyer_mode.active && !ts.ExtFMenuActive && !ts.vfo_mem_flag && !ts.XvtrFMenuActive)
    {
        ts.MenuOfFMenusActive = false;
    	ts.keyer_mode.active = true;
    }
    else if(ts.keyer_mode.active && !ts.ExtFMenuActive && !ts.vfo_mem_flag && !ts.XvtrFMenuActive)
    {
        ts.MenuOfFMenusActive = false;
    	ts.keyer_mode.active = false;
    	ts.ExtFMenuActive = true;
    }
    else if(!ts.keyer_mode.active && ts.ExtFMenuActive && !ts.vfo_mem_flag && !ts.XvtrFMenuActive)
    {
        ts.MenuOfFMenusActive = false;
    	ts.ExtFMenuActive = false;
    	ts.vfo_mem_flag = true;
    }
    else if(!ts.keyer_mode.active && !ts.ExtFMenuActive && ts.vfo_mem_flag && !ts.XvtrFMenuActive)
    {
        ts.MenuOfFMenusActive = false;
    	ts.vfo_mem_flag = false;
    	ts.XvtrFMenuActive = true;
    }
    else if(!ts.keyer_mode.active && !ts.ExtFMenuActive && !ts.vfo_mem_flag && ts.XvtrFMenuActive)
    {
        ts.MenuOfFMenusActive = false;
    	ts.XvtrFMenuActive = false;
    }
    else // miracle!
    {
        ts.MenuOfFMenusActive = false;
    	ts.keyer_mode.active = false;
    	ts.ExtFMenuActive = false;
    	ts.vfo_mem_flag = false;
    }
	ts.keyer_cycled_tx_active = false;
	UiDriver_CreateFunctionButtons(false);
}

static void UiAction_BandMinusHold(void)
{
	if(UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED))	 	// and POWER button pressed-and-held at the same time?
	{
		UiDriver_LcdBlankingStealthSwitch();
	}
	else if(UiDriver_IsButtonPressed(BUTTON_BNDP_PRESSED))	 	// and BAND-UP pressed at the same time?
	{
		if(!ts.menu_mode)	 			// do not do this in menu mode!
		{
			UiAction_ToggleWaterfallScopeDisplay();
		}
	}
	else                                                    //skip freq -48kHz/magnify - for quick scan of band
    {
//        if(ts.txrx_mode == TRX_MODE_RX)
//        {
//            df.tune_new -= 48000 / (1 << sd.magnify);
//        }
		if(ts.expflags1 & EXPFLAGS1_BANDCH_JUMP_SWAP)
		{
			UiDriver_HandleBandButtons(BUTTON_BNDM);
		}
		else
		{
		    bool buttondirSwap = (ts.flags1 & FLAGS1_SWAP_BAND_BTN)?true:false;
		    if (!buttondirSwap) { df.tune_new -= 48000 / (1 << sd.magnify); }
		    else { df.tune_new += 48000 / (1 << sd.magnify); }
		}
    }
}

static void UiAction_BandPlusHold(void)
{
	if(UiDriver_IsButtonPressed(BUTTON_BNDM_PRESSED))	 	// and BAND-DOWN pressed at the same time?
	{
		if(!ts.menu_mode)	 		// do not do this if in menu mode!
		{
			UiAction_ToggleWaterfallScopeDisplay();
		}
	}
	else if(UiDriver_IsButtonPressed(BUTTON_PWR_PRESSED))	 	// and POWER button pressed-and-held at the same time?
	{
		UiDriver_PowerDownCleanup(false); // do not save the configuration
	}
	else                                                    //skip freq +48kHz/magnify - for quick scan of band
    {
//        if(ts.txrx_mode == TRX_MODE_RX)
//        {
//            df.tune_new += 48000 / (1 << sd.magnify);
//        }
		if(ts.expflags1 & EXPFLAGS1_BANDCH_JUMP_SWAP)
		{
			UiDriver_HandleBandButtons(BUTTON_BNDP);
		}
		else
		{
		    bool buttondirSwap = (ts.flags1 & FLAGS1_SWAP_BAND_BTN)?true:false;
		    if (!buttondirSwap) { df.tune_new += 48000 / (1 << sd.magnify); }
		    else { df.tune_new -= 48000 / (1 << sd.magnify); }
		}
    }
}

static void UiAction_PowerHold(void)
{
	if(UiDriver_IsButtonPressed(BUTTON_BNDM_PRESSED))	 	// was keyPin BAND- pressed at the same time?
	{
		UiDriver_LcdBlankingStealthSwitch();
	}
	else
	{
		// ONLY the POWER button was pressed
		if(ts.txrx_mode == TRX_MODE_RX)  		// only allow power-off in RX mode
		{
			UiDriver_PowerDownCleanup(true);
		}
	}
}

static void UiAction_StepMinusHold(void)
{
	if(UiDriver_IsButtonPressed(BUTTON_STEPP_PRESSED))	 	// was keyPin STEP+ pressed at the same time?
	{
		ts.frequency_lock = !ts.frequency_lock;
		// update frequency display
		UiDriver_FrequencyUpdateLOandDisplay(true);
	}
	else
	{
		if(!(ts.freq_step_config & FREQ_STEP_SWAP_BTN))	  // keyPin swap NOT enabled
		{
			UiDriver_PressHoldStep(0);	// decrease step size
		}
		else  		// keyPin swap enabled
		{
			UiDriver_PressHoldStep(1);	// increase step size
		}
	}
}

static void UiAction_MenuSetDefaultValue(void)
{
	UiMenu_RenderMenu(MENU_PROCESS_VALUE_SETDEFAULT);
}

static void UiAction_MenuRenderPrevScreen(void)
{
    UiMenu_RenderPrevScreen();
}

static void UiAction_MenuRenderNextScreen(void)
{
    UiMenu_RenderNextScreen();
}

static void UiAction_StepPlusHold(void)
{
	if(UiDriver_IsButtonPressed(BUTTON_STEPM_PRESSED))	 	// was keyPin STEP- pressed at the same time?
	{
		ts.frequency_lock = !ts.frequency_lock;
		// update frequency display
		UiDriver_FrequencyUpdateLOandDisplay(true);
	}
	else
	{
		if(!(ts.freq_step_config & FREQ_STEP_SWAP_BTN))  	// keyPin swap NOT enabled
		{
			UiDriver_PressHoldStep(1);	// increase step size
		}
		else  		// keyPin swap enabled
		{
			UiDriver_PressHoldStep(0);	// decrease step size
		}
	}
}

#ifdef SDR_AMBER
static void UiAction_ToggleIllumButt(void)
{
    if(ts.amber_io8_present)
    {
        // bit6 == 1 - Illumination of buttons is ON
        if(ts.expflags2 & EXPFLAGS2_ILLUM_BUTT)
        {

            ts.amber_io8_state = ts.amber_io8_state & ~64; // reset bit
            ts.expflags2 = ts.expflags2 & ~EXPFLAGS2_ILLUM_BUTT; // reset flag
        }
        else
        {
            ts.amber_io8_state = ts.amber_io8_state | 64; // set bit
            ts.expflags2 = ts.expflags2 | EXPFLAGS2_ILLUM_BUTT; // set flag
        }
        Board_AmberIOx8_Write(ts.amber_io8_state);
    }
}
#endif

static void UiDriver_HandleTouchScreen(bool is_long_press) {
	if (!(ts.disabled_tp) && is_touchscreen_pressed()) {
		uint32_t touchaction_idx = ts.menu_mode ? 1 : 0;

		if (ts.show_debug_info) {					// show coordinates for coding purposes
			char text[14];
			snprintf(text, sizeof(text), "%04d : %04d ", ts.tp->hr_x, ts.tp->hr_y);

    #ifdef TOUCH_SHOW_REGIONS_AND_POINTS
			UiLcdHy28_DrawColorPoint(ts.tp->hr_x, ts.tp->hr_y, White);

			uint16_t x, y, w, h;

			for (int n=0; n<ts.Layout->touchaction_list[touchaction_idx].size; n++) {
				x=ts.Layout->touchaction_list[touchaction_idx].actions[n].region.x;
				y=ts.Layout->touchaction_list[touchaction_idx].actions[n].region.y;
				w=ts.Layout->touchaction_list[touchaction_idx].actions[n].region.w;
				h=ts.Layout->touchaction_list[touchaction_idx].actions[n].region.h;
				UiLcdHy28_DrawEmptyRect(x,y,h,w,Red);
			}
    #endif
			UiLcdHy28_PrintText(0, ts.Layout->LOADANDDEBUG_Y, text, White, Black, 0);
		}

		bool TouchProcessed = 0;

		if(ts.VirtualKeysShown_flag) {
			TouchProcessed = UiVk_Process_VirtualKeypad(is_long_press);
		}

#if 0
		// Temporarily disabled

		if (!TouchProcessed) {
			UiDriver_ProcessTouchActions(&ts.Layout->touchaction_list[touchaction_idx], is_long_press);
		}
#endif
		ts.tp->state = TP_DATASETS_PROCESSED;							// set statemachine to data fetched
	}
}

static void UiDriver_HandleTouchScreenShortPress(bool is_long_press)
{
    UiDriver_HandleTouchScreen(false);
}
static void UiDriver_HandleTouchScreenLongPress(bool is_long_press)
{
    UiDriver_HandleTouchScreen(true);
}

static const keyaction_descr_t keyactions_normal[] =
{
		{ TOUCHSCREEN_ACTIVE, 	UiDriver_HandleTouchScreenShortPress,       UiDriver_HandleTouchScreenLongPress },
		{ BUTTON_F1_PRESSED, 	UiAction_ToggleMenuMode, 					UiAction_SaveConfigurationToMemory },
		{ BUTTON_F2_PRESSED, 	UiAction_ChangeSnap, 		KEYACTION_NOP },
		{ BUTTON_F3_PRESSED, 	UiAction_ToggleSplitModeOrToggleMemMode, 	UiAction_ToggleVfoMem },
		{ BUTTON_F4_PRESSED, 	UiAction_ToggleVfoAB, 						UiAction_CopyVfoAB },
		{ BUTTON_F5_PRESSED, 	UiAction_ToggleTuneMode,					UiAction_ToggleTxDisable },
		{ BUTTON_G1_PRESSED, 	UiAction_ChangeDemodMode,					UiAction_ChangeDemodModeToAlternativeMode },
		{ BUTTON_G2_PRESSED, 	UiAction_ChangeToNextDspMode,				KEYACTION_NOP },
		{ BUTTON_G3_PRESSED, 	UiAction_ChangePowerLevel,					UiAction_ChangePowerLevelToFull },
		{ BUTTON_G4_PRESSED, 	UiAction_ChangeFilterBW,					UiAction_ChangeRxFilterOrFmToneBurst },
		{ BUTTON_M1_PRESSED, 	UiDriver_PressEncoderOne,				    UiAction_EncoderOneHold },          // UiAction_ToggleKeyerMode
		{ BUTTON_M2_PRESSED, 	UiDriver_PressEncoderTwo,				    UiAction_EncoderTwoHold },          // UiAction_ToggleNoiseblanker
		{ BUTTON_M3_PRESSED, 	UiDriver_PressEncoderThree,			        UiAction_EncoderThreeHold },
		{ BUTTON_STEPM_PRESSED, UiAction_ChangeTuningStepDownOrUp,			UiAction_StepMinusHold },
		{ BUTTON_STEPP_PRESSED, UiAction_ChangeTuningStepUpOrDown,			UiAction_StepPlusHold },
		{ BUTTON_BNDM_PRESSED, 	UiAction_ChangeBandDownOrUp,				UiAction_BandMinusHold },
		{ BUTTON_BNDP_PRESSED,  UiAction_ChangeBandUpOrDown,				UiAction_BandPlusHold },
		{ BUTTON_PWR_PRESSED, UiAction_ChangeBacklightBrightness,			UiAction_PowerHold },
#if defined(UI_BRD_OVI40) && (defined(SDR_AMBER) || defined(OVI40_MOD_480_320))
        { BUTTON_E1_PRESSED,    UiDriver_PressEncoderOne,                   UiAction_EncoderOneHold },
        { BUTTON_E2_PRESSED,    UiDriver_PressEncoderTwo,                   UiAction_EncoderTwoHold },
        { BUTTON_E3_PRESSED,    UiDriver_PressEncoderThree,                 UiAction_EncoderThreeHold },
//      { BUTTON_E4_PRESSED,    KEYACTION_NOP,                              KEYACTION_NOP },
//      { BUTTON_F6_PRESSED,    KEYACTION_NOP,                              KEYACTION_NOP },
    #ifdef SDR_AMBER_480_320
        { BUTTON_L1_PRESSED,    UiAction_ToggleIllumButt,                   KEYACTION_NOP },
    #endif
#endif
};

static const keyaction_descr_t keyactions_menu[] =
{
		{ BUTTON_F2_PRESSED, 	UiAction_MenuRenderPrevScreen, 		UiMenu_RenderFirstScreen },
		{ BUTTON_F3_PRESSED, 	UiAction_MenuRenderNextScreen, 		UiMenu_RenderLastScreen },
		{ BUTTON_F4_PRESSED, 	UiAction_MenuSetDefaultValue,				KEYACTION_NOP },
};

static const keyaction_descr_t keyactions_keyer[] =
{
		{ BUTTON_F1_PRESSED, 	UiAction_PlayKeyerBtn1,     UiAction_RecordKeyerBtn1 },
		{ BUTTON_F2_PRESSED, 	UiAction_PlayKeyerBtn2,     UiAction_RecordKeyerBtn2 },
		{ BUTTON_F3_PRESSED, 	UiAction_PlayKeyerBtn3,     UiAction_RecordKeyerBtn3 },
//		{ BUTTON_F4_PRESSED, 	KEYACTION_NOP, 		KEYACTION_NOP },
        { BUTTON_F4_PRESSED,    UiAction_PlayKeyerBtn4,     UiAction_RecordKeyerBtn4 },
		{ BUTTON_F5_PRESSED, 	UiAction_ToggleTxRx,        UiAction_ToggleBufferedTXMode },
};

static const keyaction_descr_t keyactions_mems[] =
{
        { BUTTON_F1_PRESSED,    UiDriver_LoadMemData1,                      UiDriver_StoreMemData1 },
        { BUTTON_F2_PRESSED,    UiDriver_LoadMemData2,                      UiDriver_StoreMemData2 },
        { BUTTON_F3_PRESSED,    UiAction_ToggleSplitModeOrToggleMemMode,    UiAction_ToggleVfoMem },
        { BUTTON_F4_PRESSED,    UiDriver_LoadMemData3,                      UiDriver_StoreMemData3 },
        { BUTTON_F5_PRESSED,    UiDriver_LoadMemData4,                      UiDriver_StoreMemData4 },
};

static const keyaction_descr_t keyactions_ext[] =
{
#ifndef SDR_AMBER
        { BUTTON_F1_PRESSED,    UiDriver_RIT_Reset,							KEYACTION_NOP },
#else
        { BUTTON_F1_PRESSED,    UiDriver_RIT_Reset,							UiAction_ToggleIllumButt },
#endif
        { BUTTON_F2_PRESSED,    UiAction_ChangeSpectrumZoomLevelDown,		UiAction_ZoomResetToOne },
        { BUTTON_F3_PRESSED,    UiAction_ChangeSpectrumZoomLevelUp,			UiAction_GoToMainFmenu },
#ifndef SDR_AMBER // not Amber
        { BUTTON_F5_PRESSED,    Codec_RestartI2S,							KEYACTION_NOP },
#else
        { BUTTON_F4_PRESSED,    UiAction_ChangeRXInputStateDown,			KEYACTION_NOP },
        { BUTTON_F5_PRESSED,    UiAction_ChangeRXInputStateUp,				KEYACTION_NOP },
#endif
};

static const keyaction_descr_t keyactions_xvtr[] =
{
        { BUTTON_F1_PRESSED,    UiDriver_LoadXVTR1,        UiDriver_StoreXVTR1 },
        { BUTTON_F2_PRESSED,    UiDriver_LoadXVTR2,        UiDriver_StoreXVTR2 },
        { BUTTON_F3_PRESSED,    UiAction_ToggleListXVTR,   UiAction_OffXVTR },
        { BUTTON_F4_PRESSED,    UiDriver_LoadXVTR3,        UiDriver_StoreXVTR3 },
        { BUTTON_F5_PRESSED,    UiDriver_LoadXVTR4,        UiDriver_StoreXVTR4 },
};

static const keyaction_descr_t keyactions_choice[] =
{
        { BUTTON_F1_PRESSED,    UiDriver_ChoiceKeyer,        KEYACTION_NOP },
        { BUTTON_F2_PRESSED,    UiDriver_ChoiceExt,          KEYACTION_NOP },
        { BUTTON_F3_PRESSED,    UiAction_ChoiceMems,         KEYACTION_NOP },
        { BUTTON_F4_PRESSED,    UiDriver_ChoiceXvtr,         KEYACTION_NOP },
        { BUTTON_F5_PRESSED,    UiDriver_ChoiceNormal,       KEYACTION_NOP },
};

static const keyaction_list_descr_t key_sets[] =
{
		// ATTENTION: the size calculation only works for true arrays, not for pointers!
		{ keyactions_normal, sizeof(keyactions_normal)/sizeof(*keyactions_normal) },
		{ keyactions_menu, sizeof(keyactions_menu)/sizeof(*keyactions_menu) },
		{ keyactions_keyer, sizeof(keyactions_keyer)/sizeof(*keyactions_keyer) },
        { keyactions_mems, sizeof(keyactions_mems)/sizeof(*keyactions_mems) },
        { keyactions_ext, sizeof(keyactions_ext)/sizeof(*keyactions_ext) },
		{ keyactions_xvtr, sizeof(keyactions_xvtr)/sizeof(*keyactions_xvtr) },
		{ keyactions_choice, sizeof(keyactions_choice)/sizeof(*keyactions_choice) },
};

static void UiDriver_HandleKeyboard(void)
{
	if(ks.button_processed)
	{
		UiDriver_LcdBlankingStartTimer();	// calculate/process LCD blanking timing
		AudioManagement_KeyBeep();  // make keyboard beep, if enabled

		bool keyIsProcessed = false;

        if (ts.MenuOfFMenusActive == true)
        {
            keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[6]);
        }
        if (keyIsProcessed == false && ts.XvtrFMenuActive == true)
        {
            keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[5]);
        }
		if (keyIsProcessed == false && ts.ExtFMenuActive == true)
		{
			keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[4]);
		}
//		if (ts.vfo_mem_flag == true)
		if (keyIsProcessed == false && ts.vfo_mem_flag == true)
        {
            keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[3]);
        }
//		if (ts.keyer_mode.active == true)
		if (keyIsProcessed == false && ts.keyer_mode.active == true)
		{
			keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[2]);
		}
		if (keyIsProcessed == false && ts.menu_mode == true)
		{
			keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[1]);
		}
		if (keyIsProcessed == false)
		{
			keyIsProcessed = UiDriver_ProcessKeyActions(&key_sets[0]);
		}

		// Reset flag, allow other buttons to be checked
		ks.button_processed = 0;
		ks.debounce_time	= 0;
	}
}


typedef enum {
	SCTimer_ENCODER_KEYS =0, // 10ms
	SCTimer_RTC, // 100 * 10ms
	SCTimer_LODRIFT, // 64 * 10ms
	SCTimer_VOLTAGE, // 8 * 10ms
	SCTimer_SMETER, // 4 * 10ms
	SCTimer_MAIN, // 4 * 10ms
	SCTimer_LEDBLINK, // 64 * 10ms
    SCTimer_SAM, // 25 * 10ms
	SCTimer_NUM
} SysClockTimers;

uint32_t last_sysclock_seen[SCTimer_NUM];


/*
 * Implements a simple timeout timer.
 * Returns true if the current sysclock differs by equal or more than divider cycles.
 * Dividers should be powers of 2 to generate optimal code
 */
bool UiDriver_TimerIsExpired(SysClockTimers sct,uint32_t now, uint32_t divider)
{
	return (last_sysclock_seen[sct] != now/divider);
}

/**
 * @brief Implements a simple timeout timer. Sets the time to now/divider, so it will expire in now+divider cycles
 * Dividers should be powers of 2 to generate optimal code
 */
void UiDriver_TimerRewind(SysClockTimers sct,uint32_t now, uint32_t divider)
{
	last_sysclock_seen[sct] = now/divider;
}

/**
 * @brief Implements a simple timeout timer. If expired the timer is automatically restarted.
 *
 * @param sct the timer data structure
 * @param divider should be powers of 2 to generate optimal code
 * @param now current sysclock value
 *
 * @returns true if the now differs by equal or more than divider cycles.
 *
 */
bool UiDriver_TimerExpireAndRewind(SysClockTimers sct,uint32_t now, uint32_t divider)
{
	bool retval = UiDriver_TimerIsExpired(sct, now, divider);
	if (retval)
	{
		UiDriver_TimerRewind(sct, now, divider);
	}
	return retval;
}



#ifdef USE_USBHOST
#include "usb_host.h"
#include "usbh_core.h"
#include "usbh_hid_keybd.h"
extern USBH_HandleTypeDef hUsbHostHS;
#endif

/**
 * This handler is activated by the Audio-Interrupt with a frequency of 1500 Hz via PendSV interrupt.
 * However, since this handler may run longer than 0.66uS it cannot be used to keep track of time
 * or you cannot count on it called every 0.66uS. It will be next activate no longer than 0.66uS after
 * last finishing the handler.
 *
 * This handler is executed as PendSV interrupt handler with lowest possible priority, i.e. its execution
 * is interrupted by all other interrupts with a higher priority.
 *
 * Please note: Do not call any UI related functions here, essentially we should only include high prio
 * tasks such as longer running audio processing (e.g. FreeDV or the NR processing) which should be executed
 * with as low latency as possible but take too long for being included in the audio interrupt handler itself
 * Also relevant: If this handler function does take too long, the UI handling including display updates
 * will feel sluggish or will not work at all since all processing time is spend here then and
 * the main tasks will never get executed.
 *
 * In a nutshell, unless there is a very, very good reason, do not add or change anything here.
 *
 */
void UiDriver_TaskHandler_HighPrioTasks()
{
    // READ THE LENGTHY COMMENT ABOVE BEFORE CHANGING ANYTHING BELOW!!!
    // YES, READ IT! Thank you!
    if (ads.af_disabled == false)
    {
        // we only process these audio things if rx data processing is active
#ifdef USE_FREEDV
        if (ts.dmod_mode == DEMOD_DIGI && ts.digital_mode == DigitalMode_FreeDV)
        {
            FreeDv_HandleFreeDv();
        }
#endif // USE_FREEDV

#ifdef USE_ALTERNATE_NR
        if ((is_dsp_nb_active() || is_dsp_nr()) && (ads.decimated_freq == 12000))
        {
          AudioNr_HandleNoiseReduction();
        }
#endif
    }
#ifdef USE_HIGH_PRIO_PTT
    if (RadioManagement_SwitchTxRx_Possible())
    {
        RadioManagement_HandlePttOnOff();
    }
#endif
#ifdef USE_CONVOLUTION
    	convolution_handle();
#endif

}

/**
 * Handles the regular updating and running of some real-time activities in the UI domain
 * mostly timers, do not place long running code in here. This is running the highest prio
 * interrupt context we have. So don't place anything in here unless you know what you are doing
 * and there is no other way to do it.
 *
 * Called with IQ_INTERRUPT_FREQ (i.e. 1500 Hz at the moment).
 */
void UiDriver_Callback_AudioISR()
{
    static uint32_t tcount = 0;

    // Perform LCD backlight PWM brightness function
    UiDriver_BacklightDimHandler();

    tcount+= SAMPLES_PER_DMA_CYCLE;        // add the number of samples that have passed in DMA cycle
    if(tcount >= SAMPLES_PER_CENTISECOND)  // enough samples for 0.01 second passed?
    {
        tcount -= SAMPLES_PER_CENTISECOND;  // yes - subtract that many samples
        ts.sysclock++;  // this clock updates at PRECISELY 100 Hz over the long term
    }

#ifdef FAST_FREQ_ENC
    if(ts.freq_enc_timer > 0)
    {
        ts.freq_enc_timer--;
    }
    else
    {
        ts.freq_enc_timer = 3; // 2 ms
        UiDriver_CheckFrequencyEncoder();
    }
#endif

    // Has the timing for the keyboard beep expired?
    if(ts.beep_timing > 0)
    {
        ts.beep_timing--;
    }

    if(ts.scope_scheduler)      // update thread timer if non-zero
    {
        ts.scope_scheduler--;
    }

    if(ts.waterfall.scheduler)      // update thread timer if non-zero
    {
        ts.waterfall.scheduler--;
    }

    if(ts.audio_spkr_unmute_delay_count)        // this updates at 1.5 kHz - used to time TX->RX delay
    {
        ts.audio_spkr_unmute_delay_count--;
    }

    if(ks.debounce_time < DEBOUNCE_TIME_MAX)
    {
        ks.debounce_time++;   // keyboard debounce timer
    }
}

static void UiDriver_HandleUSB_Keyboard(void)
{
#ifdef USE_USBKEYBOARD
    if(USBH_HID_GetDeviceType(&hUsbHostHS) == HID_KEYBOARD)
    {
        HID_KEYBD_Info_TypeDef *k_pinfo = USBH_HID_GetKeybdInfo(&hUsbHostHS);

        const uint32_t kb_buffer_size = (sizeof(k_pinfo->keys)/sizeof(k_pinfo->keys[0]));
        static uint8_t keys_buffer[sizeof(k_pinfo->keys)/sizeof(k_pinfo->keys[0])] = { 0 };

        /*
         * Regarding -> https://www.usb.org/sites/default/files/documents/hid1_11.pdf 72p.
         *
         * The order of keycodes in array fields has no significance. Order determination
         * is done by the host software comparing the contents of the previous report to
         * the current report. If two or more keys are reported in one report, their order is
         * indeterminate. Keyboards may buffer events that would have otherwise
         * resulted in multiple event in a single report.
         *
         * So, the code below is keeping the previous report from the keyboard to compare
         * with the next one and filter out multi-pressed keys. Later this could be used
         * to determine long press key or repeat key if it's holding - for example:
         *
         * Start recording F1-button if F5 was pressed for 2 sec...
         */
        if(k_pinfo != NULL)
        {
            /*
             * The discussion about code below was there
             * -> https://github.com/df8oe/UHSDR/pull/1702
             */
            for( uint32_t idx = 0; idx < kb_buffer_size; idx++ )
            {
                bool is_exist_in_buffer = false;
                for( uint32_t i = 0; i < kb_buffer_size; i++ )
                {
                    /*
                     * Looking for the same symbol in prev. array by iterating over this array.
                     */
                    if ( keys_buffer[i] == k_pinfo->keys[idx])
                    {
                        /*
                         * if symbol presents in both arrays that means
                         * it's already handled and we need to ignore it.
                         */
                        is_exist_in_buffer = true;
                        break;
                    }
                }
                if ( is_exist_in_buffer == true )
                {
                    /*
                     * The same character was found in previous report,
                     * so, Ignoring this one
                     */
                    continue;
                }
                else
                {
                    switch(k_pinfo->keys[idx])
                    {
                    case KEY_ESCAPE:
                      DigiModes_TxBufferReset();
                      break;
                    case KEY_BACKSPACE:
                        UiDriver_TextMsgClear();
                      break;
                    case KEY_F1:
                      RadioManagement_Request_TxOn();
                      break;
                    case KEY_F2:
                      RadioManagement_Request_TxOff();
                      break;
                    case KEY_F5:
                        if (k_pinfo->lshift)
                        {
                            UiAction_RecordKeyerBtn1();
                        }
                        else
                        {
                            UiAction_PlayKeyerBtn1();
                        }
                        break;
                    case KEY_F6:
                        if (k_pinfo->lshift)
                        {
                            UiAction_RecordKeyerBtn2();
                        }
                        else
                        {
                            UiAction_PlayKeyerBtn2();
                        }
                        break;
                    case KEY_F7:
                        if (k_pinfo->lshift)
                        {
                            UiAction_RecordKeyerBtn3();
                        }
                        else
                        {
                            UiAction_PlayKeyerBtn3();
                        }
                        break;
                    case KEY_F8:
                        if (k_pinfo->lshift)
                        {
                            UiAction_ToggleBufferedTXMode();
                        }
                        else
                        {
                            UiAction_ToggleKeyerMode();
                        }
                      break;

                    }

                    uint8_t kbdChar = USBH_HID_GetASCIICode( k_pinfo, idx );
                    if (kbdChar != '\0')
                    {
                      // FIXME seems we can push only into Digi_buffer....
                      if (is_demod_rtty() || is_demod_psk() || ts.dmod_mode == DEMOD_CW)
                      {
                          DigiModes_TxBufferPutChar( kbdChar, KeyBoard );
                      }
                      else
                      {
                          UiDriver_TextMsgPutChar( kbdChar );
                      }
                    }
                }
            }
            memcpy( keys_buffer, k_pinfo->keys, sizeof(k_pinfo->keys));
        }
    }
#endif // USE_USBKEYBOARD
}

void UiDriver_TaskHandler_MainTasks()
{

	uint32_t now = ts.sysclock;
	//        HAL_GetTick()/10;
	RadioManagement_TxRxSwitching_Disable();
	CatDriver_HandleProtocol();
	RadioManagement_TxRxSwitching_Enable();

#ifndef USE_PENDSV_FOR_HIGHPRIO_TASKS
	UiDriver_TaskHandler_HighPrioTasks();
#endif

    // START CALLED AS OFTEN AS POSSIBLE
#ifndef USE_HIGH_PRIO_PTT
	if (ts.tx_stop_req == true  || ts.ptt_req == true)
	{
		RadioManagement_HandlePttOnOff();
	}
#endif
	// END CALLED AS OFTEN AS POSSIBLE

#ifdef USE_USBHOST
    MX_USB_HOST_Process();
#endif // USE_USBHOST

	// BELOW ALL CALLING IS BASED ON SYSCLOCK 10ms clock
	if (UiDriver_TimerExpireAndRewind(SCTimer_ENCODER_KEYS,now,1))
	{
		// 10ms have elapsed.
		// Now process events which should be handled regularly at a rate of 100 Hz
		// Remember to keep this as short as possible since this is executed in addition
		// to all other processing below.
	    RadioManagement_TxRxSwitching_Disable();
		UiDriver_RotateEncoderOne();
		UiDriver_RotateEncoderTwo();
		UiDriver_RotateEncoderThree();
#ifndef FAST_FREQ_ENC
		UiDriver_CheckFrequencyEncoder();
#endif
        UiDriver_CheckCycledKeyerTX();
		UiDriver_KeyboardProcessOldClicks();
#ifndef USE_HIGH_PRIO_PTT
		RadioManagement_HandlePttOnOff();
#endif
		RadioManagement_UpdatePowerAndVSWR();
		RadioManagement_TxRxSwitching_Enable();
		if (ts.twinpeaks_tested == TWINPEAKS_CODEC_RESTART)
		{
		    Codec_RestartI2S();
		    ts.twinpeaks_tested = TWINPEAKS_WAIT;
		}
        UiDriver_HandleUSB_Keyboard();
	}

	UiSpectrum_Redraw();

	// Expect the code below to be executed around every 40 - 80ms.
	// The exact time between two calls is unknown and varies with different
	// display options  (waterfall/scope, DSP settings etc.)
	// Nothing with short intervals < 100ms  and/or need for very regular intervals between calls
	// should be placed in here.
	if(UiDriver_TimerIsExpired(SCTimer_MAIN,now,1))            // bail out if it is not time to do this task
	{
		switch(drv_state)
		{
		case STATE_S_METER:
		    // we update all the meters (either TX or RX) no more than 25 times a second
			if (UiDriver_TimerExpireAndRewind(SCTimer_SMETER,now,4))
			{
	            UiDriver_HandleTXMeters();
	            RadioManagement_HandleIqGainAndSMeter();
				UiDriver_HandleSMeter();
#ifdef USE_FREEDV
				if (ts.dmod_mode == DEMOD_DIGI && ts.digital_mode == DigitalMode_FreeDV)
				{
			        FreeDv_DisplayUpdate();
				}
#endif // USE_FREEDV

			}
			break;
		case STATE_HANDLE_POWERSUPPLY:
			Board_HandlePowerDown();

			if (UiDriver_TimerExpireAndRewind(SCTimer_VOLTAGE,now,8))
			{
				if (UiDriver_HandleVoltage())
				{
					UiDriver_DisplayVoltage();
				}

				if (pwmt.undervoltage_detected == true) {
					if (UiDriver_TimerExpireAndRewind(SCTimer_LEDBLINK, now, 64)) {
						Board_GreenLed(LED_STATE_TOGGLE);
					}
				}
				UiDriver_TextMsgDisplay();
			}
			break;
		case STATE_LO_TEMPERATURE:
			if (UiDriver_TimerExpireAndRewind(SCTimer_LODRIFT,now,64))
			{
				UiDriver_HandleLoTemperature();
#if 1
				ProfilingTimedEvent* pe_ptr = profileTimedEventGet(ProfileAudioInterrupt);

				// Percent audio interrupt load  = Num of cycles per audio interrupt  / ((max num of cycles between two interrupts ) / 100 )
				//
				// Num of cycles per audio interrupt = cycles for all counted interrupts / number of interrupts
				// Max num of cycles between two interrupts / 100 = HCLK frequency / Interruptfrequenz -> e.g. 168000000 / 1500 / 100 = 1120
				// FIXME: Need to figure out which clock is being used, 168000000 in mcHF, I40 UI = 168.000.000 or 216.000.000 or something else...

				uint32_t load =  pe_ptr->duration / (pe_ptr->count * (1120));
				profileTimedEventReset(ProfileAudioInterrupt);
				char str[20];
				snprintf(str,20,"L%3u%%",(unsigned int)load);
				if(ts.show_debug_info)
				{
					UiLcdHy28_PrintText(ts.Layout->LOAD_X,ts.Layout->LOADANDDEBUG_Y,str,White,Black,0);
				}
#endif
			}

			if (UiDriver_TimerExpireAndRewind(SCTimer_RTC,now,100)) {
				if (ts.rtc_present) {
					RTC_TimeTypeDef sTime;

					Rtc_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

					char str[20];

					snprintf(str, sizeof(str), "%2u:%02u:%02u", sTime.Hours, sTime.Minutes, sTime.Seconds);
					UiLcdHy28_PrintText(ts.Layout->RTC_IND.x, ts.Layout->RTC_IND.y, str, sd.txt_colour, Black, 4);
				}
			}
			break;

		case STATE_TASK_CHECK:
		    RadioManagement_TxRxSwitching_Disable();
			UiDriver_TimeScheduler();
			RadioManagement_TxRxSwitching_Enable();
			// Handles live update of Calibrate between TX/RX and volume control
			break;
		case STATE_UPDATE_FREQUENCY:
			/* at this point we handle request for changing the frequency
			 * either from a difference in dial freq or a temp change
			 *  */
			if((df.tune_old != df.tune_new))
			{
			    RadioManagement_TxRxSwitching_Disable();
				UiDriver_FrequencyUpdateLOandDisplay(false);
				RadioManagement_TxRxSwitching_Enable();
				UiDriver_DisplayMemoryLabel();				// this is because a frequency dialing via CAT must be indicated if "CAT in sandbox" is active
				UiSpectrum_DisplayFilterBW();
			}
			else if (df.temp_factor_changed  || ts.tune_freq != ts.tune_freq_req)
			{
				// this handles the cases where the dial frequency remains the same but the
				// LO tune frequency needs adjustment, e.g. in CW mode  or if temp of LO changes
			    RadioManagement_TxRxSwitching_Disable();
				RadioManagement_ChangeFrequency(false,df.tune_new, ts.txrx_mode);
				RadioManagement_TxRxSwitching_Enable();
			}
            else
            {
                // this handles cases in which switch from tx to rx and vice versa
                // changes the displayed frequency. This is currently only the case
                // if in xverter mode and we have tx_offset != rx_offset and is not
                // split mode.
                static uint8_t last_seen_txrx_mode = TRX_MODE_RX;
                if (ts.txrx_mode != last_seen_txrx_mode)
                {
                    last_seen_txrx_mode = ts.txrx_mode;
                    if (RadioManagement_Transverter_IsEnabled()
                            && is_splitmode() == false
                            && ts.xverter_offset_tx != 0 // tx offset enabled
                            && ts.xverter_offset != ts.xverter_offset_tx) // and not equal rx
                    {
                        RadioManagement_TxRxSwitching_Disable();
                        UiDriver_FrequencyUpdateLOandDisplay(false);
                        RadioManagement_TxRxSwitching_Enable();
                    }
                }
            }
			break;
		case STATE_PROCESS_KEYBOARD:
			UiDriver_HandleKeyboard();
			break;
		case STATE_DISPLAY_SAM_CARRIER:
			if (UiDriver_TimerExpireAndRewind(SCTimer_SAM,now,25))
			{
				if(ts.dmod_mode == DEMOD_SAM)
				{
					UiDriver_UpdateLcdFreq(df.tune_old, Yellow, UFM_SECONDARY);
				}
				else if (ts.dmod_mode == DEMOD_CW && cw_decoder_config.snap_enable)
				{
					//UiDriver_UpdateLcdFreq(ads.snap_carrier_freq, Green, UFM_SECONDARY);
				}
                UiDriver_DisplayStateAGC();

				// display CW decoder WPM speed
				if (ts.cw_decoder_enable && ts.dmod_mode == DEMOD_CW) {
					CwDecoder_WpmDisplayUpdate(false);
				}
			}
			break;

		default:
			break;
		}

		if (drv_state < STATE_MAX) {
			// advance to next state
			drv_state++;
		} else {
			UiDriver_TimerRewind(SCTimer_MAIN, now, 1);
			// wrap state to first state
			drv_state = 0;
		}
	}
}

/*
 * This handler creates a software pwm for the LCD backlight. It needs to be called
 * very regular to work properly. Right now it is activated from the audio interrupt
 * at a rate of 1.5khz The rate itself is not too critical,
 * just needs to be high and very regular.
 */

#define LCD_DIMMING_PWM_COUNTS 16

void UiDriver_BacklightDimHandler()
{
	static uchar lcd_dim = 0;
	static const uint16_t dimming_pattern_map[1 + LCD_DIMMING_LEVEL_MAX - LCD_DIMMING_LEVEL_MIN] =
	{
	        0xffff, // 16/16
	        0x3f3f, // 12/16
	        0x0f0f, // 8/16
	        0x0303, // 4/16
	        0x0101, // 2/16
	        0x0001, // 1/1
	};
    // most of the patterns generate a 1500/8 =  187.5 Hz noise, lowest 1500/16 = 93.75 Hz.

	static uint16_t dim_pattern = 0xffff; // gives us the maximum brightness

	if(!ts.lcd_blanking_flag)       // is LCD *NOT* blanked?
	{
	    if (lcd_dim == 0 )
	    {
	        dim_pattern = dimming_pattern_map[ts.lcd_backlight_brightness - LCD_DIMMING_LEVEL_MIN];
	    }

	    // UiLcdHy28_BacklightEnable(lcd_dim >= dimming_map[ts.lcd_backlight_brightness - LCD_DIMMING_LEVEL_MIN]);   // LCD backlight off or on
	    UiLcdHy28_BacklightEnable((dim_pattern & 0x001) == 1);   // LCD backlight off or on

	    dim_pattern >>=1;
	    lcd_dim++;
	    lcd_dim %= LCD_DIMMING_PWM_COUNTS;   // limit brightness PWM count to 0-3
	}
	else if(!ts.menu_mode)
	{ // LCD is to be blanked - if NOT in menu mode
		UiLcdHy28_BacklightEnable(false);
	}
}

static void UiDriver_LoadXvtrData(uint8_t xvtr_cell)
{
    if(xvtr_data[xvtr_cell][0] & 0xf)
    {
    //Don't show BndMem
    ts.expflags1 = ts.expflags1 | EXPFLAGS1_NO_SHOW_BNDMEM;
    UiDriver_ClearMemoryLabel();

    ts.xverter_mode = xvtr_data[xvtr_cell][0];
    ts.xverter_offset = xvtr_data[xvtr_cell][1];
    ts.xverter_offset_tx = xvtr_data[xvtr_cell][2];

    UiDriver_CreateMainFreqDisplay(true);
    UiDriver_FrequencyUpdateLOandDisplay(true);

#ifdef SDR_AMBER
    if (ts.expflags2 & EXPFLAGS2_XVTR_OFF_PA)
    {
        ts.amber_io8_state = ts.amber_io8_state & ~128; // reset bit
        Board_AmberIOx8_Write(ts.amber_io8_state);
    }
#endif
    }
}

static void UiDriver_LoadXVTR1()
{
    UiDriver_LoadXvtrData(0);
}

static void UiDriver_LoadXVTR2()
{
    UiDriver_LoadXvtrData(1);
}

static void UiDriver_LoadXVTR3()
{
    UiDriver_LoadXvtrData(2);
}

static void UiDriver_LoadXVTR4()
{
    UiDriver_LoadXvtrData(3);
}

static void UiDriver_StoreXvtrData(uint8_t xvtr_cell)
{
    xvtr_data[xvtr_cell][0] = ts.xverter_mode;
    xvtr_data[xvtr_cell][1] = ts.xverter_offset;
    xvtr_data[xvtr_cell][2] = ts.xverter_offset_tx;
}

static void UiDriver_StoreXVTR1()
{
    UiDriver_StoreXvtrData(0);
}

static void UiDriver_StoreXVTR2()
{
    UiDriver_StoreXvtrData(1);
}

static void UiDriver_StoreXVTR3()
{
    UiDriver_StoreXvtrData(2);
}

static void UiDriver_StoreXVTR4()
{
    UiDriver_StoreXvtrData(3);
}

static void UiAction_ShowListXVTR(void)
{
        char txt_cell[5];
        char txt_mult[5];
        char txt_rx[12];
        char txt_tx[15];
        uint32_t temp;
        uint32_t offset_offset = XVERTER_OFFSET_MAX_HZ-(XVERTER_OFFSET_MAX_HZ/1000);
        uint8_t y_start = 140;
        uint8_t y_interval = 21;

        for(int cell = 0; cell<4; cell++)
        {
            snprintf(txt_cell, 32, "XVTR%1u", cell+1);
            UiLcdHy28_PrintText(10,y_start+(cell*y_interval), txt_cell,Yellow,Black,0);
            temp = xvtr_data[cell][0];
            if(temp & 0xf && ((uint)temp < 400)) // cell is no empty
            {
                snprintf(txt_mult,32, "x%u", (uint)temp);
                UiLcdHy28_PrintText(60, y_start+(cell*y_interval), txt_mult,Yellow,Black,0);
                temp = xvtr_data[cell][1];
                if(temp > XVERTER_OFFSET_MAX_HZ)
                {
                    sprintf(txt_rx,"RX-k%9ld", temp-offset_offset);
                }
                else
                {
                    sprintf(txt_rx,"RX- %9ld", temp);
                }
                UiLcdHy28_PrintText(94, y_start+(cell*y_interval), txt_rx,Yellow,Black,0);
                temp = xvtr_data[cell][2];
                if(temp > XVERTER_OFFSET_MAX_HZ)
                {
                    sprintf(txt_tx,"TX-k%9ld", temp-offset_offset);
                }
                else if(temp == 0)
                {
                    sprintf(txt_tx,"TX-%s", " as RX");
                }
                else
                {
                    sprintf(txt_tx,"TX- %9ld", temp);
                }
                UiLcdHy28_PrintText(94,y_start+10+(cell*y_interval), txt_tx,Blue2,Black,0);
            }
            else
            {
                UiLcdHy28_PrintText(60,y_start+(cell*y_interval), "empty",White,Black,0);
            }
        }
}

static void UiAction_ToggleListXVTR()
{
    UiSpectrum_Clear();
    ts.xvtr_disp = !ts.xvtr_disp;
    if (!ts.xvtr_disp)
    {
        UiSpectrum_Init();  // init spectrum scope
    }
    else
    {
        UiAction_ShowListXVTR();
    }
}

static void UiAction_OffXVTR()
{
    ts.xverter_mode = 0;
    ts.XvtrFMenuActive = false;
    ts.xvtr_disp = false;
    UiDriver_CreateMainFreqDisplay(true);
    UiDriver_FrequencyUpdateLOandDisplay(true);
    UiSpectrum_Init();
    UiDriver_CreateFunctionButtons(false);

#ifdef SDR_AMBER
    if (ts.expflags2 & EXPFLAGS2_XVTR_OFF_PA)
    {
        ts.amber_io8_state = ts.amber_io8_state | 128; // set bit
        Board_AmberIOx8_Write(ts.amber_io8_state);
    }
#endif
}

static void UiDriver_ChoiceKeyer()
{
    ts.MenuOfFMenusActive = false;
    ts.keyer_mode.active = true;
    UiDriver_CreateFunctionButtons(false);
}

static void UiDriver_ChoiceExt()
{
    ts.MenuOfFMenusActive = false;
    ts.ExtFMenuActive = true;
    UiDriver_CreateFunctionButtons(false);
}

static void UiAction_ChoiceMems()
{
    ts.MenuOfFMenusActive = false;
    ts.vfo_mem_flag = true;
    UiDriver_CreateFunctionButtons(false);
}

static void UiDriver_ChoiceXvtr()
{
    ts.MenuOfFMenusActive = false;
    ts.XvtrFMenuActive = true;
    UiDriver_CreateFunctionButtons(false);
}

static void UiDriver_ChoiceNormal()
{
    ts.MenuOfFMenusActive = false;
    UiDriver_CreateFunctionButtons(false);
}
