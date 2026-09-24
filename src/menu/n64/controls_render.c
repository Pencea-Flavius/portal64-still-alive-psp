#include "menu/controls.h"

#include "font/dejavu_sans.h"
#include "system/controller.h"
#include "util/memory.h"

#include "codegen/assets/materials/ui.h"

static struct Coloru8 sButtonPromptTextColor = { 232, 206, 80, 255 };

static Gfx* controlsRenderIcon(Gfx* dl, struct ControllerIcon* icon, int x, int y) {
    gSPTextureRectangle(
        dl++,
        x << 2, y << 2,
        (x + icon->w) << 2, (y + icon->h) << 2,
        G_TX_RENDERTILE,
        icon->x << 5, icon->y << 5,
        0x400, 0x400
    );

    return dl;
}

static Gfx* controlsRenderActionSourceInputIcons(Gfx* dl, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ControllerIcon* inputIcon = sourceIcons[i].inputIcon;

        x -= inputIcon->w;
        dl = controlsRenderIcon(dl, inputIcon, x, y);
    }
    
    return dl;
}

static Gfx* controlsRenderActionSourceControllerIndexIcons(Gfx* dl, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y) {
    for (int i = 0; i < sourceCount; ++i) {
        struct ActionSourceIcon* sourceIcon = &sourceIcons[i];

        dl = controlsRenderIcon(
            dl,
            sourceIcon->controllerIndexIcon,
            x - sourceIcon->controllerIndexIcon->w,
            y + sourceIcon->inputIcon->h - sourceIcon->controllerIndexIcon->h
        );

        x -= sourceIcon->inputIcon->w;
    }

    return dl;
}

void controlsRowRenderInit(struct ControlsMenuRow* row) {
    Gfx* inputIconsDL = row->render.sourceInputIcons;
    Gfx* controllerIndexIconsDL = row->render.sourceControllerIndexIcons;

    for (int i = 0; i < SOURCE_ICON_COUNT; ++i) {
        gSPEndDisplayList(inputIconsDL++);
        gSPEndDisplayList(controllerIndexIconsDL++);
    }
}

void controlsRowBuildIcons(struct ControlsMenuRow* row, struct ActionSourceIcon* sourceIcons, int sourceCount, int x, int y, int showControllerIndex) {
    Gfx* dl = controlsRenderActionSourceInputIcons(
        row->render.sourceInputIcons,
        sourceIcons,
        sourceCount,
        x, y
    );
    gSPEndDisplayList(dl++);

    dl = row->render.sourceControllerIndexIcons;

    if (showControllerIndex) {
        dl = controlsRenderActionSourceControllerIndexIcons(dl, sourceIcons, sourceCount, x, y);
    }

    gSPEndDisplayList(dl++);
}

void controlsSeparatorAdd(struct ControlsMenu* controlsMenu, int index, int y) {
    gDPFillRectangle(
        &controlsMenu->render.headerSeparators[index],
        CONTROLS_X + SEPARATOR_PADDING_X,
        y,
        CONTROLS_X + CONTROLS_WIDTH - SEPARATOR_PADDING_X,
        y + SEPARATOR_THICKNESS
    );
}

void controlsSeparatorsFinish(struct ControlsMenu* controlsMenu, int count) {
    gSPEndDisplayList(&controlsMenu->render.headerSeparators[count]);
}

void controlsMenuRender(struct ControlsMenu* controlsMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPFillRectangle(renderState->dl++, CONTROLS_X, CONTROLS_Y, CONTROLS_X + CONTROLS_WIDTH, CONTROLS_Y + CONTROLS_HEIGHT);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    // Outlines
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_ENV_INDEX]);
    gSPDisplayList(renderState->dl++, controlsMenu->scrollOutline);
    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
    renderStateAppendDL(renderState, controlsMenu->render.headerSeparators);

    // Selection indicator
    if (controlsMenu->selectedRow >= 0 && controlsMenu->selectedRow < ControllerActionCount) {
        struct ControlsMenuRow* selectedAction = &controlsMenu->actionRows[controlsMenu->selectedRow];

        gDPPipeSync(renderState->dl++);
        if (controlsMenu->waitingForAction != ControllerActionNone) {
            gDPSetEnvColor(renderState->dl++, gSelectionGray.r, gSelectionGray.g, gSelectionGray.b, gSelectionGray.a);    
        } else {
            gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        }
        gDPFillRectangle(
            renderState->dl++,
            CONTROLS_X + ROW_PADDING_X,
            selectedAction->y,
            CONTROLS_X + CONTROLS_WIDTH - ROW_PADDING_X,
            selectedAction->y + selectedAction->actionText->height
        );
    }

    if (controlsMenu->selectedRow == ControllerActionCount) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gSelectionOrange.r, gSelectionOrange.g, gSelectionOrange.b, gSelectionOrange.a);
        gDPFillRectangle(
            renderState->dl++, 
            controlsMenu->useDefaults.x, 
            USE_DEFAULTS_Y, 
            controlsMenu->useDefaults.x + controlsMenu->useDefaults.w,
            USE_DEFAULTS_Y + USE_DEFAULTS_HEIGHT
        );
    }

    gSPDisplayList(renderState->dl++, controlsMenu->useDefaults.outline);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_ENV_INDEX]);

    // Text
    gSPDisplayList(renderState->dl++, ui_material_list[DEJAVU_SANS_0_INDEX]);

    gDPPipeSync(renderState->dl++);
    struct PrerenderedTextBatch* batch = prerenderedBatchStart();
    prerenderedBatchAdd(batch, controlsMenu->useDefaults.text, controlsMenu->selectedRow == ControllerActionCount ? &gColorBlack : &gColorWhite);
    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);

    gDPSetScissor(
        renderState->dl++,
        G_SC_NON_INTERLACE,
        CONTROLS_X_INNER,
        CONTROLS_Y_INNER,
        CONTROLS_X_INNER + CONTROLS_WD_INNER,
        CONTROLS_Y_INNER + CONTROLS_HT_INNER
    );

    batch = prerenderedBatchStart();

    for (int i = 0; i < ControllerActionCount; ++i) {
        prerenderedBatchAdd(batch, controlsMenu->actionRows[i].actionText, controlsMenu->selectedRow == i ? &gColorBlack : &gColorWhite);
    }
    for (int i = 0; i < MAX_CONTROLS_SECTIONS; ++i) {
        if (controlsMenu->headers[i].headerText == NULL) {
            break;
        }

        prerenderedBatchAdd(batch, controlsMenu->headers[i].headerText, &gColorWhite);
    }

    renderState->dl = prerenderedBatchFinish(batch, gDejaVuSansImages, renderState->dl);
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    // Input icons
    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);

            renderStateAppendDL(renderState, controlsMenu->actionRows[i].render.sourceInputIcons);

            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);
        } else {
            renderStateAppendDL(renderState, controlsMenu->actionRows[i].render.sourceInputIcons);
        }
    }

    gDPPipeSync(renderState->dl++);
    gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
    for (int i = 0; i < ControllerActionCount; ++i) {
        if (controlsMenu->selectedRow == i) {
            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, gColorWhite.a);

            renderStateAppendDL(renderState, controlsMenu->actionRows[i].render.sourceControllerIndexIcons);

            gDPPipeSync(renderState->dl++);
            gDPSetEnvColor(renderState->dl++, gColorBlack.r, gColorBlack.g, gColorBlack.b, gColorBlack.a);
        } else {
            renderStateAppendDL(renderState, controlsMenu->actionRows[i].render.sourceControllerIndexIcons);
        }
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);

    gDPSetScissor(renderState->dl++, G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT);
}

void controlsRenderPrompt(enum ControllerAction action, char* message, float opacity, struct RenderState* renderState) {
    if (message == NULL || *message == '\0') {
        return;
    }
    
    struct FontRenderer* fontRender = stackMalloc(sizeof(struct FontRenderer));
    fontRendererLayout(fontRender, &gDejaVuSansFont, message, SCREEN_WD - (PROMPT_MARGIN_X + (PROMPT_PADDING * 2)));
    
    struct ActionSourceIcon sourceIcons[MAX_SOURCES_PER_CONTROLLER_ACTION];
    int sourceCount = controlsGetActionSourceIcons(action, sourceIcons);

    int iconsWidth = controlsActionSourceIconsWidth(sourceIcons, sourceCount);
    int opacityAsInt = (int)(255.0f * opacity);

    int textPositionX = (SCREEN_WD - PROMPT_MARGIN_X - PROMPT_PADDING) - fontRender->width;
    int textPositionY = (SCREEN_HT - PROMPT_MARGIN_Y - PROMPT_PADDING) - fontRender->height;
    
    gSPDisplayList(renderState->dl++, ui_material_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);
    gDPSetEnvColor(renderState->dl++, 0, 0, 0, opacityAsInt / 3);
    gDPFillRectangle(
        renderState->dl++,
        textPositionX - iconsWidth - (PROMPT_PADDING * 2),
        textPositionY - PROMPT_PADDING,
        SCREEN_WD - PROMPT_MARGIN_X,
        SCREEN_HT - PROMPT_MARGIN_Y
    );
    gSPDisplayList(renderState->dl++, ui_material_revert_list[SOLID_TRANSPARENT_OVERLAY_INDEX]);

    struct Coloru8 textColor = sButtonPromptTextColor;
    textColor.a = opacityAsInt;
        
    renderState->dl = fontRendererBuildGfx(fontRender, gDejaVuSansImages, textPositionX, textPositionY, &textColor, renderState->dl);
    
    gSPDisplayList(renderState->dl++, ui_material_revert_list[DEJAVU_SANS_0_INDEX]);

    gSPDisplayList(renderState->dl++, ui_material_list[BUTTON_ICONS_INDEX]);
    gDPSetEnvColor(renderState->dl++, textColor.r, textColor.g, textColor.b, textColor.a);
    renderState->dl = controlsRenderActionSourceInputIcons(
        renderState->dl,
        sourceIcons,
        sourceCount,
        textPositionX - PROMPT_PADDING,
        textPositionY
    );

    // Disambiguate between multiple bound controllers if necessary
    if (controllerActionUsedControllerCount() > 1) {
        gDPPipeSync(renderState->dl++);
        gDPSetEnvColor(renderState->dl++, gColorWhite.r, gColorWhite.g, gColorWhite.b, textColor.a);

        renderState->dl = controlsRenderActionSourceControllerIndexIcons(
            renderState->dl,
            sourceIcons,
            sourceCount,
            textPositionX - PROMPT_PADDING,
            textPositionY
        );
    }

    gSPDisplayList(renderState->dl++, ui_material_revert_list[BUTTON_ICONS_INDEX]);
    
    stackMallocFree(fontRender);
}

void controlsRenderInputIcon(enum ControllerActionInput input, int x, int y, struct RenderState* renderState) {
    renderState->dl = controlsRenderIcon(renderState->dl, controlsInputIcon(input), x, y);
}
