#ifndef __SCENE_PSP_CLOCK_DIGITS_H__
#define __SCENE_PSP_CLOCK_DIGITS_H__

// The PSP half of the countdown clock's digits; see src/scene/n64/clock_digits.h.
// A digit is one quad scrolled along a strip of eleven glyphs; the UV units
// are the machine's.

// Scrolls the digit's quad to show this value, if it is not showing it
// already.
void clockSetDigit(int digitIndex, int currDigit);

// Forgets what every digit was showing, so the next set writes.
void clockDigitsReset();

#endif
