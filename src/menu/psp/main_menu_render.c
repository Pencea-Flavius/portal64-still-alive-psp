#include "menu/main_menu.h"

#include "menu/game_menu.h"
#include "scene/render_plan.h"
#include "scene/scene.h"
#include "scene/scene_animator.h"

// The PSP half of the main menu's drawing; see src/menu/n64/main_menu_render.c.
// Lights and modes are set once per frame, so there is no RSP preamble.

void mainMenuRender(struct GameMenu* gameMenu, struct RenderState* renderState, struct GraphicsTask* task) {
    struct RenderPlan renderPlan;

    RenderMatrices staticMatrices = sceneAnimatorBuildTransforms(&gScene.animator, renderState);

    renderPlanBuild(&renderPlan, &gScene, renderState);
    renderPlanExecute(&renderPlan, &gScene, staticMatrices, gScene.animator.transforms, renderState, task);

    gameMenuRender(gameMenu, renderState, task);
}
