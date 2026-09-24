#include "scene/scene.h"

#include "menu/game_menu.h"
#include "physics/debug_renderer.h"
#include "scene/debug_scene.h"
#include "scene/hud.h"
#include "scene/render_plan.h"
#include "scene/scene_animator.h"
#include "system/psp/psp_profile.h"

extern struct GameMenu gGameMenu;

// The N64 hands the RSP its one directional light, a look at and the opaque z
// buffered render mode here, before the plan runs. The GU takes its lighting
// through sceGuLight and its modes through sceGuEnable, which the frame sets
// up, so what is left is the order the plan and the overlays are drawn in.
void sceneRender(struct Scene* scene, struct RenderState* renderState, struct GraphicsTask* task) {
    playerApplyCameraTransform(&scene->player, &scene->camera.transform);
    vector3Add(&scene->camera.transform.position, &scene->freeCameraOffset, &scene->camera.transform.position);

    struct RenderPlan renderPlan;

    unsigned long long planStart = pspProfileNow();
    RenderMatrices staticMatrices = sceneAnimatorBuildTransforms(&scene->animator, renderState);

    renderPlanBuild(&renderPlan, scene, renderState);
    pspProfileAdd(PspProfilePlan, planStart);

    // Timed stage by stage inside.
    renderPlanExecute(&renderPlan, scene, staticMatrices, scene->animator.transforms, renderState, task);

    unsigned long long overlayStart = pspProfileNow();

    if (scene->showCollisionContacts) {
        contactSolverDebugDraw(&gContactSolver, renderState);
    }

    if (!scene->hideHud) {
        unsigned long long gunStart = pspProfileDetailNow();

        portalGunRenderReal(
            &scene->portalGun,
            renderState,
            &scene->camera,
            scene->hud.lastPortalIndexShot
        );

        pspProfileDetailAdd(PspProfileGun, gunStart);

        if (gGameMenu.state == GameMenuStateResumeGame || hudOverlayVisible(&scene->hud, &scene->player)) {
            hudRender(&scene->hud, &scene->player, renderState);
            debugSceneRender(scene, renderState, &renderPlan);
        }
    }

    if (gGameMenu.state != GameMenuStateResumeGame) {
        gameMenuRender(&gGameMenu, renderState, task);
    }

    pspProfileAdd(PspProfileOverlay, overlayStart);

    unsigned long long selfStart = pspProfileNow();
    pspProfileRender(renderState);
    pspProfileAdd(PspProfileSelf, selfStart);
}
