#ifndef __MENU_MENU_H__
#define __MENU_MENU_H__

#include "font/font.h"
#include "graphics/color.h"
#include "graphics/render_types.h"
#include "graphics/renderstate.h"

// Widget geometry, shared by both machines.
#define CHECKBOX_SIZE           12

#define BUTTON_LEFT_PADDING     4
#define BUTTON_RIGHT_PADDING    9
#define BUTTON_TOP_PADDING      2

// The options box, centred; wider on the PSP to fit all five tabs. Here
// because the pages inside it include options_menu.h.
#ifdef PSP
#define OPTIONS_MENU_WIDTH  400
#define OPTIONS_MENU_HEIGHT 236
#else
#define OPTIONS_MENU_WIDTH  280
#define OPTIONS_MENU_HEIGHT 200
#endif
#define OPTIONS_MENU_LEFT   ((SCREEN_WD - OPTIONS_MENU_WIDTH) / 2)
#define OPTIONS_MENU_TOP    ((SCREEN_HT - OPTIONS_MENU_HEIGHT) / 2)

// Where an options page starts, below the tabs.
#define OPTIONS_PAGE_TOP    (OPTIONS_MENU_TOP + 34)

#define SLIDER_TRACK_HEIGHT     4
#define SLIDER_HEIGHT           12
#define SLIDER_WIDTH            6
#define TICK_Y                  11
#define TICK_HEIGHT             3

struct MenuButton {
    RenderDisplayList outline;
    struct PrerenderedText* text;
    short x, y;
    short w, h;
};

struct MenuCheckbox {
    RenderDisplayList outline;
    struct PrerenderedText* prerenderedText;
    RenderDisplayList checkedIndicator;
    short x, y;
    short checked;
};

struct MenuSlider {
    RenderDisplayList back;
    float value;
    short x, y;
    short w;
    // The N64 bakes the ticks in; the PSP needs their count.
    short tickCount;
};

enum InputCapture {
    InputCapturePass,
    InputCaptureGrab,
    InputCaptureExit,
};

extern struct Coloru8 gSelectionOrange;
extern struct Coloru8 gSelectionGray;

extern struct Coloru8 gBorderHighlight;
extern struct Coloru8 gBorderDark;

struct PrerenderedText* menuBuildPrerenderedText(struct Font* font, char* message, int x, int y, int maxWidth);

struct MenuButton menuBuildButton(struct Font* font, char* message, int x, int y, int height, int rightAlign);
void menuRebuildButtonText(struct MenuButton* button, struct Font* font, char* message, int rightAlign);
void menuRelocateButton(struct MenuButton* button, int x, int y, int rightAlign);

struct MenuCheckbox menuBuildCheckbox(struct Font* font, char* message, int x, int y);
struct MenuSlider menuBuildSlider(int x, int y, int w, int tickCount);

void menuFreePrerenderedDeferred(struct PrerenderedText* text);
void menuTickDeferredQueue();
void menuResetDeferredQueue();

// Drawing is declared in the platform's half.
#include "menu_render.h"

#endif