#include "clock_digits.h"

#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include "codegen/assets/models/dynamic_model_list.h"
#include "codegen/assets/models/signage/clock_digits.h"

// The N64 half of the clock's digits; see src/scene/psp/clock_digits.c.

// One glyph's width along the strip, in the RDP's texel fixed point.
#define DIGIT_WIDTH     512

static u8 gCurrentClockDigits[7];

static Vtx* gClockDigits[] = {
    signage_clock_digits_clock_digits_hour_01_color,
    signage_clock_digits_clock_digits_minute_10_color,
    signage_clock_digits_clock_digits_minute_01_color,
    signage_clock_digits_clock_digits_second_10_color,
    signage_clock_digits_clock_digits_second_01_color,
    signage_clock_digits_clock_digits_ms_10_color,
    signage_clock_digits_clock_digits_ms_01_color,
};

void clockSetDigit(int digitIndex, int currDigit) {
    int prevDigit = gCurrentClockDigits[digitIndex];

    if (prevDigit == currDigit) {
        return;
    }

    Vtx* digitPointer = dynamicAssetFixPointer(SIGNAGE_CLOCK_DIGITS_DYNAMIC_MODEL, gClockDigits[digitIndex]);

    for (int i = 0; i < 4; ++i) {
        digitPointer[i].v.tc[0] += (currDigit - prevDigit) * DIGIT_WIDTH;
    }
    gCurrentClockDigits[digitIndex] = (u8)currDigit;
    osWritebackDCache(digitPointer, sizeof(Vtx) * 4);
}

void clockDigitsReset() {
    zeroMemory(gCurrentClockDigits, sizeof(gCurrentClockDigits));
}
