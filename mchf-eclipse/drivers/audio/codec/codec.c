/*  -*-  mode: c; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4; coding: utf-8  -*-  */
/************************************************************************************
**                                                                                 **
**                               mcHF QRP Transceiver                              **
**                             K Atanassov - M0NKA 2014                            **
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
#include "audio_driver.h"
#include "radio_management.h"

#include <stdio.h>

#include "uhsdr_hw_i2c.h"
#include "codec.h"
#include "uhsdr_mcu.h"

// I2C addresses
#define W8731_ADDR_0                    0x1A        // CS = 0, MODE to GND
#define W8731_ADDR_1                    0x1B        // CS = 1, MODE to GND

// The 7 bits Codec address (sent through I2C interface)
#define CODEC_ADDRESS                   (W8731_ADDR_0<<1)

// Registers
#define W8731_LEFT_LINE_IN              0x00        // 0000000
#define W8731_RIGHT_LINE_IN             0x01        // 0000001
#define W8731_LEFT_HEADPH_OUT           0x02        // 0000010
#define W8731_RIGHT_HEADPH_OUT          0x03        // 0000011
#define W8731_ANLG_AU_PATH_CNTR         0x04        // 0000100
#define W8731_DIGI_AU_PATH_CNTR         0x05        // 0000101
#define W8731_POWER_DOWN_CNTR           0x06        // 0000110
#define W8731_DIGI_AU_INTF_FORMAT       0x07        // 0000111
#define W8731_SAMPLING_CNTR             0x08        // 0001000
#define W8731_ACTIVE_CNTR               0x09        // 0001001
#define W8731_RESET                     0x0F        // 0001111

// -------------------------------------------------

//#define W8731_DEEMPH_CNTR                 0x06        // WM8731 codec De-emphasis enabled
#define W8731_DEEMPH_CNTR               0x00        // WM8731 codec De-emphasis disabled


#define W8731_HEADPH_OUT_ZCEN     0x0080      // bit 7 W8731_LEFT_HEADPH_OUT / W8731_RIGHT_HEADPH_OUT
#define W8731_HEADPH_OUT_HPBOTH   0x0100      // bit 8 W8731_LEFT_HEADPH_OUT / W8731_RIGHT_HEADPH_OUT
#define W8731_LINE_IN_LRBOTH   0x0100      // bit 8 W8731_LEFT_LINE_IN_OUT / W8731_RIGHT_LINE_IN

#define W8731_ANLG_AU_PATH_CNTR_DACSEL      (0x10)
#define W8731_ANLG_AU_PATH_CNTR_INSEL_MIC       (0x04)
#define W8731_ANLG_AU_PATH_CNTR_INSEL_LINE       (0x00)
#define W8731_ANLG_AU_PATH_CNTR_MUTEMIC     (0x02)
#define W8731_ANLG_AU_PATH_CNTR_MICBBOOST   (0x01)
#define W8731_DIGI_AU_INTF_FORMAT_PHILIPS 0x02
#define W8731_DIGI_AU_INTF_FORMAT_PCM     0x00
#define W8731_DIGI_AU_INTF_FORMAT_16B     (0x0 << 2)
#define W8731_DIGI_AU_INTF_FORMAT_20B     (0x1 << 2)
#define W8731_DIGI_AU_INTF_FORMAT_24B     (0x2 << 2)
#define W8731_DIGI_AU_INTF_FORMAT_32B     (0x3 << 2)

#define W8731_DIGI_AU_INTF_FORMAT_I2S_PROTO W8731_DIGI_AU_INTF_FORMAT_PHILIPS

#define W8731_POWER_DOWN_CNTR_POWEROFF  (0x80)
#define W8731_POWER_DOWN_CNTR_CLKOUTPD  (0x40)
#define W8731_POWER_DOWN_CNTR_OSCPD     (0x20)
#define W8731_POWER_DOWN_CNTR_OUTPD     (0x10)
#define W8731_POWER_DOWN_CNTR_DACPD     (0x08)
#define W8731_POWER_DOWN_CNTR_ADCPD     (0x04)
#define W8731_POWER_DOWN_CNTR_MICPD     (0x02)
#define W8731_POWER_DOWN_CNTR_LINEPD    (0x01)

#define W8731_SAMPLING_CNTR_BOSR        (0x0002)

#define W8731_SAMPLING_CNTR_96K         (0x0007 << 2)
#define W8731_SAMPLING_CNTR_48K         (0x0000 << 2)
#define W8731_SAMPLING_CNTR_32K         (0x0006 << 2)
#define W8731_SAMPLING_CNTR_8K          (0x0003 << 2)


#define W8731_VOL_MAX 0x50

#define W8731_POWER_DOWN_CNTR_MCHF_ALL_ON    (W8731_POWER_DOWN_CNTR_CLKOUTPD|W8731_POWER_DOWN_CNTR_OSCPD)
// all on but osc and out, since we don't need it, clock comes from STM

#define W8731_POWER_DOWN_CNTR_MCHF_MIC_OFF    (W8731_POWER_DOWN_CNTR_CLKOUTPD|W8731_POWER_DOWN_CNTR_OSCPD|W8731_POWER_DOWN_CNTR_MICPD)

typedef struct
{
    bool present;
} mchf_codec_t;


__IO mchf_codec_t mchf_codecs[CODEC_NUM];

// FIXME: for now we use 32bits transfer size, does not change the ADC/DAC resolution
// which is 24 bits in any case. We should reduce finally to 24bits (which requires also the I2S/SAI peripheral to
// use 24bits)

#if defined(USE_32_IQ_BITS)
    #define IQ_WORD_SIZE WORD_SIZE_32
#else
    #define IQ_WORD_SIZE WORD_SIZE_16
#endif

#if defined(USE_32_AUDIO_BITS)
    #define AUDIO_WORD_SIZE WORD_SIZE_32
#else
    #define AUDIO_WORD_SIZE WORD_SIZE_16
#endif

#ifdef UI_BRD_OVI40
#include "dac.h"
/**
 * @brief controls volume on "external" PA via DAC
 * @param vol volume in range of 0 to CODEC_SPEAKER_MAX_VOLUME
 */
static void AudioPA_Volume(uint8_t vol)
{
    uint32_t lv = vol>CODEC_SPEAKER_MAX_VOLUME?CODEC_SPEAKER_MAX_VOLUME:vol;
    HAL_DAC_SetValue(&hdac,DAC_CHANNEL_1,DAC_ALIGN_12B_R, (lv * 4095)/CODEC_SPEAKER_MAX_VOLUME);
}
/**
 * @brief controls sound delivery on "external" PA via DAC
 * @param enable  true == amplification, false == powerdown
 */
static void AudioPA_Enable(bool enable)
{
    if (enable)
    {
        GPIO_SetBits(AUDIO_PA_EN_PIO,AUDIO_PA_EN);
    }
    else
    {
        GPIO_ResetBits(AUDIO_PA_EN_PIO,AUDIO_PA_EN);
    }
}
#endif
/**
 * @brief writes 16 bit data word to codec register
 * @returns I2C error code
 */
static uint32_t Codec_WriteRegister(I2C_HandleTypeDef* hi2c, uint8_t RegisterAddr, uint16_t RegisterValue)
{
    // Assemble 2-byte data in WM8731 format
    uint8_t Byte1 = ((RegisterAddr<<1)&0xFE) | ((RegisterValue>>8)&0x01);
    uint8_t Byte2 = RegisterValue&0xFF;
    return UhsdrHw_I2C_WriteRegister(hi2c, CODEC_ADDRESS, Byte1, 1, Byte2);
}

static uint32_t Codec_ResetCodec(I2C_HandleTypeDef* hi2c, uint32_t AudioFreq, CodecSampleWidth_t word_size)
{
    uint32_t retval = HAL_OK;

    retval = Codec_WriteRegister(hi2c, W8731_RESET, 0);
    // Reset register
    if( retval == HAL_OK)
    {
        // Reg 00: Left Line In (0dB, mute off)
        Codec_WriteRegister(hi2c, W8731_LEFT_LINE_IN,0x001F);

        // Reg 01: Right Line In (0dB, mute off)
        Codec_WriteRegister(hi2c, W8731_RIGHT_LINE_IN,0x001F);

        // Reg 02: Left Headphone out (0dB)
        //Codec_WriteRegister(0x02,0x0079);
        // Reg 03: Right Headphone out (0dB)
        //Codec_WriteRegister(0x03,0x0079);


        // Reg 04: Analog Audio Path Control (DAC sel, ADC line, Mute Mic)
        Codec_WriteRegister(hi2c, W8731_ANLG_AU_PATH_CNTR,
                W8731_ANLG_AU_PATH_CNTR_DACSEL |
                W8731_ANLG_AU_PATH_CNTR_INSEL_LINE |
                W8731_ANLG_AU_PATH_CNTR_MUTEMIC);

        // Reg 05: Digital Audio Path Control(all filters disabled)
        // De-emphasis control, bx11x - 48kHz
        //                      bx00x - off
        // DAC soft mute        b1xxx - mute on
        //                      b0xxx - mute off
        //
        Codec_WriteRegister(hi2c, W8731_DIGI_AU_PATH_CNTR,W8731_DEEMPH_CNTR);

        // Reg 06: Power Down Control (Clk off, Osc off, Mic off))
        Codec_WriteRegister(hi2c, W8731_POWER_DOWN_CNTR,W8731_POWER_DOWN_CNTR_MCHF_MIC_OFF);


        // Reg 07: Digital Audio Interface Format (i2s, 16/32 bit, slave)
        uint16_t size_reg_val;

        switch(word_size)
        {
            case WORD_SIZE_32:
                size_reg_val = W8731_DIGI_AU_INTF_FORMAT_32B;
                break;
            case WORD_SIZE_24:
                size_reg_val = W8731_DIGI_AU_INTF_FORMAT_24B;
                break;
            case WORD_SIZE_16:
            default:
                size_reg_val = W8731_DIGI_AU_INTF_FORMAT_16B;
                break;
        }

        Codec_WriteRegister(hi2c, W8731_DIGI_AU_INTF_FORMAT,W8731_DIGI_AU_INTF_FORMAT_I2S_PROTO|size_reg_val);


        // Reg 08: Sampling Control (Normal, 256x, 48k ADC/DAC)
        // master clock: 12.288 Mhz
        uint16_t samp_reg_val;

        switch (AudioFreq)
        {
        case 32000:
            samp_reg_val = W8731_SAMPLING_CNTR_32K;
            break;
        case 8000:
            samp_reg_val = W8731_SAMPLING_CNTR_8K;
            break;
        case 96000:
            samp_reg_val = W8731_SAMPLING_CNTR_96K;
            break;
        case 48000:
        default:
            samp_reg_val = W8731_SAMPLING_CNTR_48K;
            break;
        }

        Codec_WriteRegister(hi2c, W8731_SAMPLING_CNTR,samp_reg_val);

        // Reg 09: Active Control
        // and now we start the Codec Digital Interface
        Codec_WriteRegister(hi2c, W8731_ACTIVE_CNTR,0x0001);
    }
    return retval;

}

#ifdef SDR_AMBER
#ifndef UI_BRD_MCHF
uint32_t Codec_Reset_CS4270_Codec()
{
    uint32_t retval = HAL_OK;
//    uint32_t AudioFreq = ts.samp_rate;
//    CodecSampleWidth_t word_size = IQ_WORD_SIZE;

    GPIO_ResetBits(BAND3_PIO, BAND3); // this port is using for CS4270 codec RESET in TRX Amber
    non_os_delay(); // we can't use HAL_Delay here
    GPIO_SetBits(BAND3_PIO, BAND3);

    retval = Board_AmberCS4270_Write(0x02, 0x01); // total codec powerdown
    if(retval != HAL_OK)
    {
        return retval;
    }
    non_os_delay();
    retval = Board_AmberCS4270_Write(0x02, 0x00); // total codec powerup

    if(retval == HAL_OK)
    {
//        Board_AmberCS4270_Write(0x02, 0x02); // DAC powerdown, we need RESET just on RX
        Board_AmberCS4270_Write(0x03, 0x00); // slave, divide 1
        Board_AmberCS4270_Write(0x04, 0x09); // I2S for DAC & ADC
        Board_AmberCS4270_Write(0x05, 0x80); // DACs single volume
        Board_AmberCS4270_Write(0x06, 0x00); // Automute off
        Board_AmberCS4270_Write(0x07, 0x00); // DACs max volume
    }
    return retval;
}
#endif
#endif

/**
 * @brief initializes codec
 * @param AudioFreq sample rate in Hertz
 * @param word_size should be set to WORD_SIZE_16, since we have not yet implemented any other word_size
 */
uint32_t Codec_Reset(uint32_t AudioFreq)
{

    uint32_t retval;
#ifdef UI_BRD_MCHF
    retval = Codec_ResetCodec(CODEC_I2C, AudioFreq, IQ_WORD_SIZE);
#else
    ts.codecWM8731_Audio_present = false;
    retval = Codec_ResetCodec(CODEC_ANA_I2C, AudioFreq, AUDIO_WORD_SIZE);
    if (retval == 0)
    {
        ts.codecWM8731_Audio_present = true;
        mchf_codecs[1].present = true;
        retval = Codec_ResetCodec(CODEC_IQ_I2C, AudioFreq, IQ_WORD_SIZE);
    }
#endif
    if (retval == 0)
    {
        mchf_codecs[0].present = true;

#ifdef UI_BRD_OVI40
        AudioPA_Enable(true);
#endif

        Codec_VolumeSpkr(0); // mute speaker
        Codec_VolumeLineOut(ts.txrx_mode); // configure lineout according to mode


    }

#if defined(SDR_AMBER) && defined(UI_BRD_OVI40)
    else
    {
        ts.codecCS4270_present = Codec_Reset_CS4270_Codec() == HAL_OK;
        if(ts.codecCS4270_present)
        {
            AudioPA_Enable(true);
        }
    }
#endif
    return retval;
}

/**
 * @brief Call this if the twin peaks happen, this restarts the I2S audio stream and it may fix the issue
 */
void Codec_RestartI2S(void)
{
#if !defined(UI_BRD_MCHF) && defined(SDR_AMBER)
    if(ts.codecCS4270_present)
    {
        Codec_Reset_CS4270_Codec();
    }
    else
    {
#endif
    // Reg 09: Active Control
    Codec_WriteRegister(CODEC_IQ_I2C, W8731_ACTIVE_CNTR,0x0000);
    non_os_delay(); // we can't use HAL_Delay here, since our audio interrupt has higher priority which stops the ticks.
    // Reg 09: Active Control
    Codec_WriteRegister(CODEC_IQ_I2C, W8731_ACTIVE_CNTR,0x0001);
#if !defined(UI_BRD_MCHF) && defined(SDR_AMBER)
    }
#endif
}

/**
 * @brief This enables the microphone if in TX and sets gain, does nothing in RX or if audio_source is not microphone
 * @param txrx_mode the mode for which it should be configured
 */
void Codec_SwitchMicTxRxMode(uint8_t txrx_mode)
{
    // only adjust the hardware if in TX txrx_mode with mic selected (it will kill RX otherwise!)
    if(txrx_mode == TRX_MODE_TX && ts.tx_audio_source == TX_AUDIO_MIC)
    {
        // Set up microphone gain and adjust mic boost accordingly
        // Reg 04: Analog Audio Path Control (DAC sel, ADC Mic, Mic on)

        // non_os_delay();

        if(ts.tx_gain[TX_AUDIO_MIC] > 50)	 		// actively adjust microphone gain and microphone boost
        {
            Codec_WriteRegister(CODEC_ANA_I2C, W8731_ANLG_AU_PATH_CNTR,
                    W8731_ANLG_AU_PATH_CNTR_DACSEL |
                    W8731_ANLG_AU_PATH_CNTR_INSEL_MIC|
                    W8731_ANLG_AU_PATH_CNTR_MICBBOOST); // mic boost on

            ts.tx_mic_gain_mult = (ts.tx_gain[TX_AUDIO_MIC] - 35)/3;			// above 50, rescale software amplification
        }
        else
        {
            Codec_WriteRegister(CODEC_ANA_I2C, W8731_ANLG_AU_PATH_CNTR,
                    W8731_ANLG_AU_PATH_CNTR_DACSEL |
                    W8731_ANLG_AU_PATH_CNTR_INSEL_MIC);	// mic boost off

            ts.tx_mic_gain_mult = ts.tx_gain[TX_AUDIO_MIC];
        }
    }
}

static bool is_microphone_active(void)
{
	return ts.tx_audio_source == TX_AUDIO_MIC && (ts.dmod_mode != DEMOD_CW && is_demod_rtty() == false && is_demod_psk() == false);
}

/**
 * @brief sets certain settings in preparation for smooth TX switching, call before actual switch function is called
 * @param current_txrx_mode the current mode, not the future mode (this is assumed to be TRX_MODE_TX)
 */
void Codec_PrepareTx(uint8_t current_txrx_mode)
{
	Codec_LineInGainAdj(0); // yes - momentarily mute LINE IN audio if in LINE IN mode until we have switched to TX

	bool uses_mic_input = is_microphone_active();

	if (uses_mic_input)  // we are in MIC IN mode
	{
		ts.tx_mic_gain_mult = 0; // momentarily set the mic gain to zero while we go to TX
		Codec_WriteRegister(CODEC_ANA_I2C, W8731_ANLG_AU_PATH_CNTR,
				W8731_ANLG_AU_PATH_CNTR_DACSEL
						| W8731_ANLG_AU_PATH_CNTR_INSEL_LINE
						| W8731_ANLG_AU_PATH_CNTR_MUTEMIC);
		// Mute the microphone with the CODEC (this does so without a CLICK) and  remain/switch line in on
		Codec_WriteRegister(CODEC_ANA_I2C, W8731_POWER_DOWN_CNTR,
				W8731_POWER_DOWN_CNTR_MCHF_ALL_ON);
		// now we power on all amps including the mic preamp and bias
	}

	// Is translate mode active and we have NOT already muted the audio output?
	if ((ts.iq_freq_mode) && (current_txrx_mode == TRX_MODE_RX))
	{
		Codec_VolumeSpkr(0);
		Codec_VolumeLineOut(TRX_MODE_TX); // yes - mute the audio codec to suppress an approx. 6 kHz chirp when going in to TX mode
	}


	if (uses_mic_input)
	{
		HAL_Delay(10);
		// pause an instant because the codec chip has its own delay before tasks complete when we use the microphone input!
		// otherwise audible noise will be transmitted
	}
}


/**
 * @brief setups up the codec according to tx/rx mode and selected sources
 * @param txrx_mode the mode for which it should be configured
 *
 */
void Codec_SwitchTxRxMode(uint8_t txrx_mode)
{
    // First step - mute sound
    Codec_VolumeSpkr(0);
    Codec_VolumeLineOut(txrx_mode);

    if(txrx_mode == TRX_MODE_RX)
    {
        // Mute line input
        Codec_LineInGainAdj(0);

        // Reg 04: Analog Audio Path Control (DAC sel, ADC line, Mute Mic)
        Codec_WriteRegister(CODEC_ANA_I2C, W8731_ANLG_AU_PATH_CNTR,
                W8731_ANLG_AU_PATH_CNTR_DACSEL|
                W8731_ANLG_AU_PATH_CNTR_INSEL_LINE|
                W8731_ANLG_AU_PATH_CNTR_MUTEMIC);

        // Reg 06: Power Down Control (Clk off, Osc off, Mic Off)
        // COMMENT:  It would be tempting to set bit 1 "MICPD" of "W8731_POWER_DOWN_CTR" to zero to disable mic power down
        // and maintain microphone bias during receive, but this seems to cause problems on receive (e.g. deafness) even
        // if the microphone is muted and "mic boost" is disabled.  (KA7OEI 20151030)

        Codec_WriteRegister(CODEC_ANA_I2C, W8731_POWER_DOWN_CNTR,W8731_POWER_DOWN_CNTR_MCHF_MIC_OFF);	// turn off mic bias
    }
    else		// It is transmit
    {
        if(RadioManagement_UsesTxSidetone())
        {
            Codec_TxSidetoneSetgain(txrx_mode);	// set sidetone level
        }
        else	 	// Not CW or TUNE mode
        {

            // Select source or leave it as it is
            // PHONE out is muted, normal exit routed to TX modulator
            // input audio is routed via 4066 switch

            if(ts.tx_audio_source == TX_AUDIO_MIC)
            {
                // now enabled the analog path according to gain settings
                // with or without boost
                Codec_SwitchMicTxRxMode(txrx_mode);
            }
            else if (ts.tx_audio_source != TX_AUDIO_DIG || ts.tx_audio_source != TX_AUDIO_DIGIQ)
            {
                // we change gain only if it is not a digital tx input source
                Codec_LineInGainAdj(ts.tx_gain[ts.tx_audio_source]);
                // set LINE input gain if in LINE in mode
            }
        }
    }
}

/**
 * @brief calculates and sets sidetone gain based on tx power factor
 *
 * This calculates the relative level of the sidetone and sets the headphone gain appropriately
 * to keep the sidetone level more or less the same.
 * This seems to be slightly "off", particularly at the extremes of high and low
 * transmit power levels - this needs to be looked into...
 *
 */

void Codec_TxSidetoneSetgain(uint8_t txrx_mode)
{
// Note that this function is called from places OTHER than Codec_RX_TX(), above!

    if(txrx_mode == TRX_MODE_TX)  		// bail out if not in transmit mode
    {
        float32_t vcalc = 0;

        if(ts.cw_sidetone_gain)	 	// calculate if the sidetone gain is non-zero
        {
            float32_t pf = ts.tx_power_factor;	// get TX scaling power factor
            if ( pf == 0 )
            {
                pf = 0.001; // Almost zero but prevent from NoNe (1/0) in the next equation.
            }

            float32_t signal_level_db = 10* log10f(1/(pf*pf));
            // we invert the square of power_factor (aka the signal energy)
            // since we are calculating attenuation of the original signal (assuming normalization to 1.0)
            // get the log
            // and multiple with 10 to convert to deciBels

            float32_t sidetone_level_db = 6.0 *((float32_t)ts.cw_sidetone_gain-5);
            // get the sidetone gain (level) setting
            // offset by # of dB the desired sidetone gain

            vcalc = signal_level_db + sidetone_level_db;		// add the calculated gain to the desired sidetone gain

            if(vcalc > 127)  			// enforce limits of calculation to range of attenuator
            {
                vcalc = 127;
            }
            else if	(vcalc < 0)
            {
                vcalc = 0;
            }
        }

        Codec_VolumeSpkr(vcalc/5); // divide by 5 to convert decibel to volume control steps
    }
}


/**
 * @brief audio volume control in TX and RX modes for speaker [left headphone]
 * @param vol speaker / headphone volume in range  [0 - CODEC_SPEAKER_MAX_VOLUME], unit is dB, 0 represents muting, one increment represents 5db
 */

void Codec_VolumeSpkr(uint8_t vol)
{
#ifdef UI_BRD_MCHF
    uint32_t lv = vol*5>W8731_VOL_MAX?W8731_VOL_MAX:vol*5;
    // limit max value to 80

    lv += 0x2F; // volume offset, all lower values including 0x2F represent muting
    // Reg 02: Speaker - variable volume, change at zero crossing in order to prevent audible clicks
    //    Codec_WriteRegister(W8731_LEFT_HEADPH_OUT,lv); // (lv | W8731_HEADPH_OUT_ZCEN));
    Codec_WriteRegister(CODEC_ANA_I2C, W8731_LEFT_HEADPH_OUT,(lv | W8731_HEADPH_OUT_ZCEN));
#else
    // external PA Control
    AudioPA_Volume(vol);
#endif
}
/**
 * @brief audio volume control in TX and RX modes for lineout [right headphone]
 *
 * At RX Lineout is always on with constant level (control via ts.lineout_gain)
 * At TX only if no frequency translation is active AND TX lineout mute is not set OR in CW
 * (because we send always without frequency translation in CW)
 *
 * @param txrx_mode txrx for which volume is to be set
 */
void Codec_VolumeLineOut(uint8_t txrx_mode)
{

    uint16_t lov =  ts.lineout_gain + 0x2F;
    // Selectively mute "Right Headphone" output (LINE OUT) depending on transceiver configuration

#ifdef UI_BRD_MCHF
    // only needed if we have MCHF_UI which shares IQ and Audio on same codec
    // we can only "listen to the transmit output if we are send
    if (
            (txrx_mode == TRX_MODE_TX)
            &&
            ((ts.flags1 & FLAGS1_MUTE_LINEOUT_TX) || (ts.iq_freq_mode && ts.dmod_mode != DEMOD_CW))
       )
    {
        // at CW we transmit without translation, no matter what the iq_freq_mode for RX is
        // is translate mode active OR translate mode OFF but LINE OUT to be muted during transmit
        Codec_WriteRegister(CODEC_ANA_I2C, W8731_RIGHT_HEADPH_OUT,0);  // yes - mute LINE OUT during transmit
    }
    else    // receive mode - LINE OUT always enabled
    {
        Codec_WriteRegister(CODEC_ANA_I2C, W8731_RIGHT_HEADPH_OUT,lov);   // value selected for 0.5VRMS at AGC setting
    }
#elif defined(UI_BRD_OVI40)
    UNUSED(txrx_mode);
    // we have a special shared lineout/headphone on the OVI40.
    // And since we have a dedidacted IQ codec, there is no need to switch of the lineout or headphones here
    Codec_WriteRegister(CODEC_ANA_I2C, W8731_RIGHT_HEADPH_OUT, lov | W8731_HEADPH_OUT_ZCEN | W8731_HEADPH_OUT_HPBOTH );   // value selected for 0.5VRMS at AGC setting
#endif
}

/**
 * @brief mute the Codecs Digital to Analog Converter Output
 * @param state true -> mute, false -> unmute
 */
void Codec_MuteDAC(bool state)
{
    if(state)
    {
        Codec_WriteRegister(CODEC_ANA_I2C, W8731_DIGI_AU_PATH_CNTR,(W8731_DEEMPH_CNTR|0x08));	// mute
    }
    else
    {
        Codec_WriteRegister(CODEC_ANA_I2C, W8731_DIGI_AU_PATH_CNTR,(W8731_DEEMPH_CNTR));		// mute off
    }
}

/**
 * @brief Sets the Codec WM8371 line input gain for both channels
 * @param gain in range of [0-255]
 */
static void Codec_InGainAdj(I2C_HandleTypeDef* hi2c, uint16_t gain)
{
    // Use Reg 00: Left Line In, set flag to adjust gain of both channels simultaneously
    Codec_WriteRegister(hi2c, W8731_LEFT_LINE_IN, gain | W8731_LINE_IN_LRBOTH);
}

/**
 * @brief Sets the Codec WM8371 line input gain for both audio in channels
 * @param gain in range of [0-255]
 */
void Codec_LineInGainAdj(uint8_t gain)
{
    Codec_InGainAdj(CODEC_ANA_I2C, gain);
}

/**
 * @brief Sets the Codec WM8371 line input gain for IQ in (both channels)
 * @param gain in range of [0-255]
 */
void Codec_IQInGainAdj(uint8_t gain)
{
#ifdef SDR_AMBER
    if(ts.codecCS4270_present) // simulation of level control at the ADC input, which is absent in the CS4270 codec
    {
        float32_t gain_t;
        switch(gain)
        {
        case 0:  gain_t = 0.0188; break; // -34.5 dB
        case 1:  gain_t = 0.0224; break; // -33.0 dB
        case 2:  gain_t = 0.0266; break; // -31.5 dB
        case 3:  gain_t = 0.0316; break; // -30.0 dB
        case 4:  gain_t = 0.0376; break; // -28.5 dB
        case 5:  gain_t = 0.0447; break; // -27.0 dB
        case 6:  gain_t = 0.0531; break; // -25.5 dB
        case 7:  gain_t = 0.0631; break; // -24.0 dB
        case 8:  gain_t = 0.0750; break; // -22.5 dB
        case 9:  gain_t = 0.0891; break; // -21.0 dB
        case 10: gain_t = 0.1060; break; // -19.5 dB
        case 11: gain_t = 0.1260; break; // -18.0 dB
        case 12: gain_t = 0.1500; break; // -16.5 dB
        case 13: gain_t = 0.1780; break; // -15.0 dB
        case 14: gain_t = 0.2110; break; // -13.5 dB
        case 15: gain_t = 0.2510; break; // -12.0 dB
        case 16: gain_t = 0.2990; break; // -10.5 dB
        case 17: gain_t = 0.3550; break; //  -9.0 dB
        case 18: gain_t = 0.4220; break; //  -7.5 dB
        case 19: gain_t = 0.5010; break; //  -6.0 dB
        case 20: gain_t = 0.5960; break; //  -4.5 dB
        case 21: gain_t = 0.7080; break; //  -3.0 dB
        case 22: gain_t = 0.8410; break; //  -1.5 dB
        case 23: gain_t = 1.0000; break; //     0 dB
        case 24: gain_t = 1.1900; break; //   1.5 dB
        case 25: gain_t = 1.4100; break; //   3.0 dB
        case 26: gain_t = 1.6800; break; //   4.5 dB
        case 27: gain_t = 2.0000; break; //   6.0 dB
        case 28: gain_t = 2.3700; break; //   7.5 dB
        case 29: gain_t = 2.8200; break; //   9.0 dB
        case 30: gain_t = 3.3500; break; //  10.5 dB
        case 31: gain_t = 4.0000; break; //  12.0 dB
        }
        ts.rf_gain_codecCS4270 = gain_t;
    }
    else
    {
#endif
    Codec_InGainAdj(CODEC_IQ_I2C, gain);
#ifdef SDR_AMBER
    }
#endif
}

/**
 * @brief Checks if all codec resources are available for switching
 * It basically checks if the I2C is currently in use
 * This function must be called before changing the oscillator in interrupts
 * otherwise deadlocks may happen
 * @return true if it is safe to call codec functions in an interrupt
 */
bool Codec_ReadyForIrqCall(void)
{
    return (CODEC_ANA_I2C->Lock == HAL_UNLOCKED) && (CODEC_IQ_I2C->Lock == HAL_UNLOCKED);
}
