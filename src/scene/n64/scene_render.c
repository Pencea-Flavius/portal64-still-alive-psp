#include "scene/scene.h"

#include "menu/game_menu.h"
#include "physics/debug_renderer.h"
#include "scene/debug_scene.h"
#include "scene/hud.h"
#include "scene/render_plan.h"
#include "scene/scene_animator.h"

extern struct GameMenu gGameMenu;

// The scene's one directional light, which main_menu.c draws against too.
Lights1 gSceneLights = gdSPDefLights1(128, 128, 128, 128, 128, 128, 0, 127, 0);

// The one directional light and the look at the RSP is handed once a frame,
// before the plan runs. The GU takes its lighting through sceGuLight instead,
// which is why neither of these has a PSP counterpart.
LookAt gLookAt = gdSPDefLookAt(127, 0, 0, 0, 127, 0);

void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    playerApplyCameraTransform(&scene->player, &scene->camera.transform);
    vector3Add(&scene->camera.transform.position, &scene->freeCameraOffset, &scene->camera.transform.position);

    gSPSetLights1(renderState->dl++, gSceneLights);
    LookAt* lookAt = renderStateRequestLookAt(renderState);

    if (!lookAt) {
        return;
    }

    *lookAt = gLookAt;
    gSPLookAt(renderState->dl++, lookAt);

    gDPSetRenderMode(renderState->dl++, G_RM_ZB_OPA_SURF, G_RM_ZB_OPA_SURF2);

    struct RenderPlan renderPlan;

    Mtx* staticMatrices = sceneAnimatorBuildTransforms(&scene->animator, renderState);

    renderPlanBuild(&renderPlan, scene, renderState);
    renderPlanExecute(&renderPlan, scene, staticMatrices, scene->animator.transforms, renderState, task);

    if (scene->showCollisionContacts) {
        contactSolverDebugDraw(&gContactSolver, renderState);
    }

    if (!scene->hideHud) {
        portalGunRenderReal(
            &scene->portalGun,
            renderState,
            &scene->camera,
            scene->hud.lastPortalIndexShot
        );

        if (gGameMenu.state == GameMenuStateResumeGame || hudOverlayVisible(&scene->hud, &scene->player)) {
            hudRender(&scene->hud, &scene->player, renderState);
            debugSceneRender(scene, renderState, &renderPlan);
        }
    }

    if (gGameMenu.state != GameMenuStateResumeGame) {
        gameMenuRender(&gGameMenu, renderState, task);
    }
}
