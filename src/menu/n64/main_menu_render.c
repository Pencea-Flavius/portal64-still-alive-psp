#include "menu/main_menu.h"

#include "menu/game_menu.h"
#include "scene/render_plan.h"
#include "scene/scene.h"
#include "scene/scene_animator.h"

// The N64 half of the main menu's drawing; see src/menu/psp/main_menu_render.c.
// It draws the scene behind it with the same RSP setup as sceneRender().

extern Lights1 gSceneLights;
extern LookAt gLookAt;

void mainMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    gSPSetLights1(renderState->dl++, gSceneLights);
    LookAt* lookAt = renderStateRequestLookAt(renderState);

    if (!lookAt) {
        return;
    }

    *lookAt = gLookAt;
    gSPLookAt(renderState->dl++, lookAt);

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    struct RenderPlan renderPlan;

    Mtx* staticMatrices = sceneAnimatorBuildTransforms(&gScene.animator, renderState);

    renderPlanBuild(&renderPlan, &gScene, renderState);
    renderPlanExecute(&renderPlan, &gScene, staticMatrices, gScene.animator.transforms, renderState, task);

    gameMenuRender(gameMenu, renderState, task);
}
