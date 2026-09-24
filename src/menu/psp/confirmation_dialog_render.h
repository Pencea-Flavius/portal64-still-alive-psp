#ifndef __MENU_PSP_CONFIRMATION_DIALOG_RENDER_H__
#define __MENU_PSP_CONFIRMATION_DIALOG_RENDER_H__

// The PSP half of the confirmation dialog's drawing; see src/menu/n64/confirmation_dialog_render.h.

#include "graphics/renderstate.h"

struct ConfirmationDialog;

void confirmationDialogRender(struct ConfirmationDialog* confirmationDialog, struct RenderState* renderState);

#endif
