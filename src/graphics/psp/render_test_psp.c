#include "render_test_psp.h"

#include "system/controller.h"
#include "system/display.h"

#include "psp_vertex.h"
#include "psp_model_render.h"
#include "psp_render.h"

#include "companion_cube.h"
#include "test_cube.h"

#include <pspgu.h>
#include <pspgum.h>
#include <psputils.h>

// The game's world scale; positions below are in these units.
#define SCENE_SCALE         128



// One UV unit in the N64's S10.5 texel fixed point, for a texture this size.



static struct RenderState sRenderState;

static struct RenderTestState sState = {
    .rotationX = 0.0f,
    .rotationY = 0.0f,
    .lodBias   = 0.0f,
    .autoLod   = 1,
    .textured  = 1,
    .mipmaps   = 1,
    .debugTexture = 0,
};

const struct RenderTestState* renderTestGetState() {
    return &sState;
}

#define ROTATION_SPEED      0.04f
#define STICK_DEADZONE      16
#define LOD_BIAS_STEP       0.25f

static void renderTestReadControls() {
    struct ControllerStick stick;
    controllerGetStick(0, &stick);

    if (stick.x > STICK_DEADZONE || stick.x < -STICK_DEADZONE) {
        sState.rotationY += (float)stick.x * ROTATION_SPEED / 80.0f;
    }

    if (stick.y > STICK_DEADZONE || stick.y < -STICK_DEADZONE) {
        sState.rotationX += (float)stick.y * ROTATION_SPEED / 80.0f;
    }

    if (controllerGetButtonsDown(0, ControllerButtonLeft)) {
        sState.lodBias -= LOD_BIAS_STEP;
    }

    if (controllerGetButtonsDown(0, ControllerButtonRight)) {
        sState.lodBias += LOD_BIAS_STEP;
    }

    if (controllerGetButtonsDown(0, ControllerButtonCDown)) {
        sState.autoLod = !sState.autoLod;
    }

    if (controllerGetButtonsDown(0, ControllerButtonCRight)) {
        sState.mipmaps = !sState.mipmaps;
    }

    if (controllerGetButtonsDown(0, ControllerButtonCLeft)) {
        sState.textured = !sState.textured;
    }

    // A texture with an obvious layout, to see which part lands where.
    if (controllerGetButtonsDown(0, ControllerButtonL)) {
        sState.debugTexture = !sState.debugTexture;
    }

    if (controllerGetButtonsDown(0, ControllerButtonCUp)) {
        sState.rotationX = 0.0f;
        sState.rotationY = 0.0f;
    }
}

void renderTestInit() {
    renderStateReset(&sRenderState);
}

void renderTestDraw(float seconds) {
    // The model renderer handles texture and material state.
    renderTestReadControls();

    pspModelSetLodOverride(sState.autoLod, sState.mipmaps, sState.lodBias);

    {
        ScePspFVector3 lightDirection = { 0.4f, 0.7f, 1.0f };
        sceGuLight(0, GU_DIRECTIONAL, GU_DIFFUSE, &lightDirection);
        sceGuLightColor(0, GU_DIFFUSE, 0xFFFFFFFF);
        sceGuAmbient(0xFF404040);
        sceGuEnable(GU_LIGHT0);
    }

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    // 16 bit depth: keep the near/far ratio small (the game uses 0.5 to 50).
    sceGumPerspective(60.0f, 480.0f / 272.0f, 0.5f, 50.0f);

    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    pspModelViewChanged();

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    {
        ScePspFVector3 position = { 0.0f, 0.0f, -1.6f };
        ScePspFVector3 rotation = { sState.rotationX, sState.rotationY, 0.0f };
        ScePspFVector3 scale = {
            PSP_VERTEX_POSITION_SCALE(SCENE_SCALE),
            PSP_VERTEX_POSITION_SCALE(SCENE_SCALE),
            PSP_VERTEX_POSITION_SCALE(SCENE_SCALE)
        };

        sceGumTranslate(&position);
        sceGumRotateXYZ(&rotation);
        sceGumScale(&scale);
    }

    {
        const struct PspTexture* textured = companion_cube_cube_material.texture;

        if (!sState.textured) {
            companion_cube_cube_material.texture = 0;
        } else if (sState.debugTexture) {
            companion_cube_cube_material.texture = &test_cube_test_cube_rgba_16b_texture;
        }

        pspModelDraw(&companion_cube_model);
        companion_cube_cube_material.texture = textured;
    }

    // The 2D pass the menus will use, drawn over the world with depth off.
    renderStateReset(&sRenderState);

    sceGuDisable(GU_DEPTH_TEST);
    sceGuDisable(GU_LIGHTING);
    sceGuDisable(GU_TEXTURE_2D);
    sceGuEnable(GU_BLEND);
    sceGuBlendFunc(GU_ADD, GU_SRC_ALPHA, GU_ONE_MINUS_SRC_ALPHA, 0, 0);

    pspRenderSetScissor(0, 0, SCREEN_WD, SCREEN_HT);

    // An opaque bar and a half transparent one over it.
    pspRenderFillRect(&sRenderState, 16, SCREEN_HT - 48, 120, 16, 0xFF3030FF);
    pspRenderFillRect(&sRenderState, 76, SCREEN_HT - 40, 120, 16, 0x80FFFF30);

    sceGuDisable(GU_BLEND);
    sceGuEnable(GU_DEPTH_TEST);
    pspMaterialForgetState();
}
