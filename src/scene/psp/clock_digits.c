#include "clock_digits.h"

#include "graphics/psp/psp_model.h"
#include "graphics/psp/psp_vertex.h"
#include "util/dynamic_asset_loader.h"
#include "util/memory.h"

#include <pspkernel.h>

#include "codegen/assets/models/dynamic_model_list.h"
#include "codegen/assets/models/signage/clock_digits.h"

// The PSP half of the clock's digits; see src/scene/n64/clock_digits.c.
// The scroll is sixteen texels over the bound (padded) width, read from the
// model: countdown.png is 176 wide but bound as 256.
#define DIGIT_WIDTH_TEXELS      16

static float clockDigitWidthUV(const void* vertices) {
    for (unsigned short i = 0; i < signage_clock_digits_model.partCount; ++i) {
        const struct PspModelPart* part = &signage_clock_digits_model.parts[i];

        if (part->vertices == vertices && part->material->texture) {
            return DIGIT_WIDTH_TEXELS / (float)part->material->texture->width;
        }
    }

    return 0.0f;
}

// What each digit shows, 0xFF if unknown (after a reset).
static unsigned char gCurrentClockDigits[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Each digit's U at glyph 0. The model is never reloaded here, so digits
// are placed from these rather than moved relative.
static float gBaseU[7][4];
static int gBaseCaptured;

static struct PspVertexColor* gClockDigits[] = {
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

    if (!gBaseCaptured) {
        for (int digit = 0; digit < 7; ++digit) {
            for (int i = 0; i < 4; ++i) {
                gBaseU[digit][i] = gClockDigits[digit][i].u;
            }
        }

        gBaseCaptured = 1;
    }

    struct PspVertexColor* digitPointer =
        dynamicAssetFixPointer(SIGNAGE_CLOCK_DIGITS_DYNAMIC_MODEL, gClockDigits[digitIndex]);

    float glyphWidth = clockDigitWidthUV(digitPointer);

    for (int i = 0; i < 4; ++i) {
        digitPointer[i].u = gBaseU[digitIndex][i] + currDigit * glyphWidth;
    }

    gCurrentClockDigits[digitIndex] = (unsigned char)currDigit;

    // Written back for the GE.
    sceKernelDcacheWritebackRange(digitPointer, sizeof(struct PspVertexColor) * 4);
}

void clockDigitsReset() {
    for (int i = 0; i < 7; ++i) {
        gCurrentClockDigits[i] = 0xFF;
    }
}
