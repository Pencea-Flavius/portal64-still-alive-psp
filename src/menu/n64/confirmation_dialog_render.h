#ifndef __MENU_N64_CONFIRMATION_DIALOG_RENDER_H__
#define __MENU_N64_CONFIRMATION_DIALOG_RENDER_H__

// The N64 half of the confirmation dialog's drawing; see src/menu/psp/confirmation_dialog_render.h.

#include "graphics/renderstate.h"

struct ConfirmationDialog;

void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState);

#endif
