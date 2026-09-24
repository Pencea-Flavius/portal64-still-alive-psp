#include "graphics.h"

#include "system/display.h"
#include "system/psp/display_psp.h"

#include "graphics/psp/psp_model_render.h"
#include "graphics/psp/psp_render.h"

#include <pspdisplay.h>
#include <pspgu.h>

#include "system/psp/psp_profile.h"

#include "codegen/assets/materials/static.h"
#include "codegen/assets/materials/ui.h"

struct GraphicsTask gGraphicsTasks[2];

// Clears run under the whole depth range: the MINZ/MAXZ window of a stage's
// slice would discard the cleared 0. Each stage sets its slice again.
static void graphicsFullDepthRange() {
    pspModelSetDepthBias(0);
    renderViewportApply(renderViewportFullscreen());
}

void graphicsInit() {
    for (unsigned i = 0; i < 2; ++i) {
        gGraphicsTasks[i].taskIndex = i;
        gGraphicsTasks[i].framebuffer = 0;
        renderStateInit(&gGraphicsTasks[i].renderState);
    }

    // Level textures first; they fill most of every view.
    pspMaterialsToVram(static_material_list, STATIC_MATERIAL_COUNT);
    pspMaterialsToVram(ui_material_list, UI_MATERIAL_COUNT);
}

void graphicsCreateTask(struct GraphicsTask* targetTask, GraphicsCallback callback, void* data) {
    // The last frame's sceGuSync freed the scratch.
    renderStateReset(&targetTask->renderState);

    displayPspStartFrame();
    pspModelStartFrame();

    targetTask->framebuffer = displayPspGetDrawBufferOffset();

    // Clear colour too: menus don't cover the screen, and with two buffers
    // old frames showed through. Far depth is 0.
    graphicsFullDepthRange();
    sceGuClearColor(0);
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    callback(data, &targetTask->renderState, targetTask);

    unsigned long long waitStart = pspProfileNow();
    displayPspEndFrame();
    pspProfileAdd(PspProfileGpuWait, waitStart);

    // The swap waits for vblank.
    waitStart = pspProfileNow();
    displayPspSwapBuffers();
    pspProfileAdd(PspProfileIdle, waitStart);
}

void graphicsTaskClearZBuffer(struct GraphicsTask* task, int minX, int minY, int maxX, int maxY) {
    (void)task;

    graphicsFullDepthRange();
    // Width and height; see pspRenderSetScissor().
    sceGuScissor(minX, minY, maxX - minX, maxY - minY);

    // Far is 0, as display_psp.c sets.
    sceGuClearDepth(0);
    sceGuClear(GU_DEPTH_BUFFER_BIT);

    sceGuScissor(0, 0, SCREEN_WD, SCREEN_HT);
}
