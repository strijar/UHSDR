/************************************************************************************
 **                                                                                **
 **                               UHSDR Firmware                                   **
 **                             O Belousov - R1CBU 2021                            **
 **                                                                                **
 **--------------------------------------------------------------------------------**
 **                                                                                **
 **  File name:                                                                    **
 **  Description:                                                                  **
 **  Last Modified:                                                                **
 **  Licence:       GNU GPLv3                                                      **
 ************************************************************************************/

#include "uhsdr_board.h"
#include "audio_reverb.h"

#define CF0 2595
#define CF1 2241
#define CF2 2911
#define CF3 3234

#define AP0 480
#define AP1 161
#define AP2 46

static float32_t    cfbuf0[CF0];
static float32_t    cfbuf1[CF1];
static float32_t    cfbuf2[CF2];
static float32_t    cfbuf3[CF3];

static float32_t    apbuf0[AP0];
static float32_t    apbuf1[AP1];
static float32_t    apbuf2[AP2];

typedef struct {
    float32_t   *buf;
    uint16_t    index;
    uint16_t    delay;
    float32_t   gain;
} item_t;

static float32_t    wet0 = 1.0f;
static float32_t    wet1 = 0.0f;

static item_t       cf0, cf1, cf2, cf3;
static item_t       ap0, ap1, ap2;

static float32_t CalcComb(float32_t in, item_t *comb) {
    float32_t   readback = comb->buf[comb->index];
    float32_t   new = readback * comb->gain + in;

    comb->buf[comb->index] = new;
    comb->index++;

    if (comb->index > comb->delay)
        comb->index = 0;

    return readback;
}

static float32_t CalcAllPass(float32_t in, item_t *allpass) {
    float32_t   reedback = allpass->buf[allpass->index] - allpass->gain*in;
    float32_t   new = reedback*allpass->gain + in;

    allpass->buf[allpass->index] = new;
    allpass->index++;

    if (allpass->index > allpass->delay)
        allpass->index = 0;

    return reedback;
}

void AudioReverb_Init(void) {
    cf0.buf = (float32_t*) &cfbuf0;  cf0.gain = 0.805f;
    cf1.buf = (float32_t*) &cfbuf1;  cf1.gain = 0.827f;
    cf2.buf = (float32_t*) &cfbuf2;  cf2.gain = 0.783f;
    cf3.buf = (float32_t*) &cfbuf3;  cf3.gain = 0.764f;

    ap0.buf = (float32_t*) &apbuf0;  ap0.gain = 0.7f;
    ap1.buf = (float32_t*) &apbuf1;  ap1.gain = 0.7f;
    ap2.buf = (float32_t*) &apbuf2;  ap2.gain = 0.7f;

    wet0 = 1.0f;
    wet1 = 0.0f;

    AudioReverb_SetDelay();
    AudioReverb_SetWet();
}

void AudioReverb_SetDelay() {
    float32_t x = ts.reverb_delay / 100.f;

    cf0.index = 0;  cf0.delay = (uint16_t) (x * CF0);
    cf1.index = 0;  cf1.delay = (uint16_t) (x * CF1);
    cf2.index = 0;  cf2.delay = (uint16_t) (x * CF2);
    cf3.index = 0;  cf3.delay = (uint16_t) (x * CF3);

    ap0.index = 0;  ap0.delay = (uint16_t) (x * AP0);
    ap1.index = 0;  ap1.delay = (uint16_t) (x * AP1);
    ap2.index = 0;  ap2.delay = (uint16_t) (x * AP2);
}

void AudioReverb_SetWet() {
    float32_t x = ts.reverb_gain / 100.0f;

    wet0 = x;
    wet1 = 1.0f - x;
}

float32_t AudioReverb_Calc(float32_t in) {
    float32_t   new = (CalcComb(in, &cf0) + CalcComb(in, &cf1) + CalcComb(in, &cf2) + CalcComb(in, &cf3)) / 4.0f;

    new = CalcAllPass(new, &ap0);
    new = CalcAllPass(new, &ap1);
    new = CalcAllPass(new, &ap2);

    return wet0 * new + wet1 * in;
}
