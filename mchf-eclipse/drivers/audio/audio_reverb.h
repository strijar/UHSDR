/*************************************************************************************
 **                                                                                 **
 **                               UHSDR Firmware                                    **
 **                             O Belousov - R1CBU 2021                             **
 **                                                                                 **
 **---------------------------------------------------------------------------------**
 **                                                                                 **
 **  File name:                                                                     **
 **  Description:                                                                   **
 **  Last Modified:                                                                 **
 **  Licence:       GNU GPLv3                                                       **
 *************************************************************************************/

#ifndef AUDIO_AUDIO_REVERB_H_
#define AUDIO_AUDIO_REVERB_H_

#include "uhsdr_types.h"

extern void AudioReverb_Init(void);
extern void AudioReverb_SetDelay(float32_t x);
extern void AudioReverb_SetWet(float32_t x);
float32_t AudioReverb_Calc(float32_t in);

#endif /* AUDIO_AUDIO_REVERB_H_ */
