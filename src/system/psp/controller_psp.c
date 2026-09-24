#include "system/controller.h"

#include <pspctrl.h>

#define PSP_CONTROLLER_COUNT        1
#define STICK_DIRECTION_THRESHOLD   40

// The game scales stick input against MAX_JOYSTICK_RANGE (80), not the full
// signed byte range, so match that instead of the N64 hardware range.
#define STICK_RANGE                 80
#define PSP_STICK_CENTER            128
#define PSP_STICK_DEFLECTION        128

// The PSP's two groups of four map to the N64's C buttons and D-pad:
//
//     triangle  C-up       up     D-up         L       L
//     circle    C-right    right  D-right      R       R
//     cross     C-down     down   D-down       START   Start
//     square    C-left     left   D-left       SELECT  Z
//
// Cross and circle also report A and B for the menus; A and B can't be bound
// in the controls menu (see controllerActionReadAnySource()). Default layout
// in controller_actions.c.
static const struct {
    unsigned int pspButton;
    enum ControllerButtons controllerButton;
} sButtonMap[] = {
    { PSP_CTRL_TRIANGLE, ControllerButtonCUp                        },
    { PSP_CTRL_CIRCLE,   ControllerButtonCRight | ControllerButtonB },
    { PSP_CTRL_CROSS,    ControllerButtonCDown  | ControllerButtonA },
    { PSP_CTRL_SQUARE,   ControllerButtonCLeft                      },

    { PSP_CTRL_UP,       ControllerButtonUp     },
    { PSP_CTRL_DOWN,     ControllerButtonDown   },
    { PSP_CTRL_LEFT,     ControllerButtonLeft   },
    { PSP_CTRL_RIGHT,    ControllerButtonRight  },

    { PSP_CTRL_LTRIGGER, ControllerButtonL      },
    { PSP_CTRL_RTRIGGER, ControllerButtonR      },
    { PSP_CTRL_START,    ControllerButtonStart  },
    { PSP_CTRL_SELECT,   ControllerButtonZ      },
};

#define BUTTON_MAP_COUNT (sizeof(sButtonMap) / sizeof(*sButtonMap))

static enum ControllerButtons   sButtons;
static enum ControllerButtons   sLastButtons;
static struct ControllerStick   sStick;
static enum ControllerDirection sLastDirection;

// Nub deadzone, out of 128: a used nub rests 20 to 40 off centre.
#define PSP_NUB_DEADZONE            36

static int8_t stickAxisFromPsp(unsigned char raw) {
    int deflection = (int)raw - PSP_STICK_CENTER;
    int magnitude = deflection < 0 ? -deflection : deflection;

    if (magnitude <= PSP_NUB_DEADZONE) {
        return 0;
    }

    // Stretch the rest back over the full range.
    int scaled = ((magnitude - PSP_NUB_DEADZONE) * STICK_RANGE) / (PSP_STICK_DEFLECTION - PSP_NUB_DEADZONE);

    if (scaled > STICK_RANGE) {
        scaled = STICK_RANGE;
    }

    return (int8_t)(deflection < 0 ? -scaled : scaled);
}

void controllersInit() {
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
}

void controllersPoll() {
    sLastButtons = sButtons;
    sLastDirection = controllerGetDirection(0);

    SceCtrlData pad;

    if (sceCtrlPeekBufferPositive(&pad, 1) < 1) {
        return;
    }

    enum ControllerButtons buttons = ControllerButtonNone;

    for (unsigned i = 0; i < BUTTON_MAP_COUNT; ++i) {
        if (pad.Buttons & sButtonMap[i].pspButton) {
            buttons |= sButtonMap[i].controllerButton;
        }
    }

    sButtons = buttons;

    sStick.x = stickAxisFromPsp(pad.Lx);
    // The nub reports Y growing downwards, the game expects it growing upwards.
    sStick.y = -stickAxisFromPsp(pad.Ly);
}

int controllerIsConnected(int index) {
    return index < PSP_CONTROLLER_COUNT;
}

void controllerSetRumble(int index, uint8_t enabled) {
    // The PSP has no rumble hardware, so this stays empty
}

enum ControllerButtons controllerGetButtons(int index, enum ControllerButtons buttons) {
    return sButtons & buttons;
}

enum ControllerButtons controllerGetButtonsDown(int index, enum ControllerButtons buttons) {
    return sButtons & ~sLastButtons & buttons;
}

enum ControllerButtons controllerGetButtonsUp(int index, enum ControllerButtons buttons) {
    return ~sButtons & sLastButtons & buttons;
}

enum ControllerButtons controllerGetButtonsHeld(int index, enum ControllerButtons buttons) {
    return sButtons & sLastButtons & buttons;
}

void controllerGetStick(int index, struct ControllerStick* stick) {
    *stick = sStick;
}

enum ControllerDirection controllerGetDirection(int index) {
    enum ControllerDirection result = ControllerDirectionNone;

    if (sStick.y > STICK_DIRECTION_THRESHOLD || (sButtons & ControllerButtonUp)) {
        result |= ControllerDirectionUp;
    }

    if (sStick.y < -STICK_DIRECTION_THRESHOLD || (sButtons & ControllerButtonDown)) {
        result |= ControllerDirectionDown;
    }

    if (sStick.x > STICK_DIRECTION_THRESHOLD || (sButtons & ControllerButtonRight)) {
        result |= ControllerDirectionRight;
    }

    if (sStick.x < -STICK_DIRECTION_THRESHOLD || (sButtons & ControllerButtonLeft)) {
        result |= ControllerDirectionLeft;
    }

    return result;
}

enum ControllerDirection controllerGetDirectionDown(int index) {
    return controllerGetDirection(index) & ~sLastDirection;
}
