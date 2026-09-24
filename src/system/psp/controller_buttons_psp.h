#ifndef __CONTROLLER_BUTTONS_PSP_H__
#define __CONTROLLER_BUTTONS_PSP_H__

// The PSP has no counterpart to the N64's C buttons, so these are plain
// sequential bits rather than hardware values. controller_psp.c translates
// sceCtrl input into them.
enum ControllerButtons {
    ControllerButtonNone    = 0,
    ControllerButtonA       = (1 << 0),
    ControllerButtonB       = (1 << 1),
    ControllerButtonZ       = (1 << 2),
    ControllerButtonStart   = (1 << 3),
    ControllerButtonUp      = (1 << 4),
    ControllerButtonDown    = (1 << 5),
    ControllerButtonLeft    = (1 << 6),
    ControllerButtonRight   = (1 << 7),
    ControllerButtonL       = (1 << 8),
    ControllerButtonR       = (1 << 9),
    ControllerButtonCUp     = (1 << 10),
    ControllerButtonCDown   = (1 << 11),
    ControllerButtonCLeft   = (1 << 12),
    ControllerButtonCRight  = (1 << 13)
};

#endif
