#ifndef __RENDER_TEST_PSP_H__
#define __RENDER_TEST_PSP_H__

// Draws a hand-built model in skeletool64's layout, to check the format and
// its scale factors. The state is what the cube is drawn with, for the
// on-screen readout.
struct RenderTestState {
    float rotationX;
    float rotationY;
    float lodBias;
    int   autoLod;
    int   textured;
    int   mipmaps;
    int   debugTexture;
};

void renderTestInit();
void renderTestDraw(float seconds);

const struct RenderTestState* renderTestGetState();

#endif
