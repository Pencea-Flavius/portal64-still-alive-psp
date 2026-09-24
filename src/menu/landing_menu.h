#ifndef __MENU_LANDING_MENU_H__
#define __MENU_LANDING_MENU_H__

#include "font/font.h"
#include "graphics/render_types.h"
#include "menu.h"

// The logo across the top and the option list under it, which both halves of
// the drawing lay out against.
#define PORTAL_LOGO_X               30
#define PORTAL_LOGO_Y               74

#define PORTAL_LOGO_WIDTH           128
#define PORTAL_LOGO_P_WIDTH         15
#define PORTAL_LOGO_O_WIDTH         30
#define PORTAL_LOGO_HEIGHT          47

#define PACKED_MENU_THRESHOLD       4

struct LandingMenuOption {
    short messageId;
    short id;
};

struct LandingMenu {
    struct LandingMenuOption* options;
    struct PrerenderedText** optionText;
    struct PrerenderedText* versionText;
    short selectedItem;
    short optionCount;
    short darkenBackground;
};

void landingMenuInit(struct LandingMenu* landingMenu, struct LandingMenuOption* options, int optionCount, int darkenBackground);
void landingMenuRebuildText(struct LandingMenu* landingMenu);
struct LandingMenuOption* landingMenuUpdate(struct LandingMenu* landingMenu);
int getCurrentStrideValue(struct LandingMenu* landingMenu);

// Drawing is declared in the platform's half, which the build puts on the
// include path.
#include "landing_menu_render.h"

#endif