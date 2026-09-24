#ifndef __SCENE_N64_CLOCK_DIGITS_H__
#define __SCENE_N64_CLOCK_DIGITS_H__

// The N64 half of the clock's digits (src/scene/psp/...): each digit is a
// quad whose UVs scroll along a strip of eleven glyphs.

// Scrolls the digit's quad to this value if needed.
void clockSetDigit(int digitIndex, int currDigit);

// Forgets what every digit shows, so the next set writes.
void clockDigitsReset();

#endif
