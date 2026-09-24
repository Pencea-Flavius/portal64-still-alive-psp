// Smoke test for the PSP platform layer through the system/*.h interfaces
// and the libultra shim. No game code.

#include "system/cartridge.h"
#include "system/controller.h"
#include "system/display.h"
#include "system/time.h"

#include "boot_psp.h"
#include "display_psp.h"
#include "render_test_psp.h"

#include <ultra64.h>

#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspkernel.h>
#include <pspthreadman.h>

PSP_MODULE_INFO("portal64_boot_test", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define PRODUCER_THREAD_ID          2
#define PRODUCER_THREAD_PRIORITY    11
#define PRODUCER_INTERVAL_US        100000

#define MESSAGE_QUEUE_LENGTH        8

// Called the way game code does, though the kernel owns the stack.
static u64 sProducerStack[512];

static OSThread     sProducerThread;
static OSMesgQueue  sMessageQueue;
static OSMesg       sMessageBuffer[MESSAGE_QUEUE_LENGTH];

static volatile int sMessagesSent;

#define BOOT_COUNT_OFFSET   0x0000
#define ROUNDTRIP_OFFSET    0x0100
#define ROUNDTRIP_PATTERN   0xC0FFEE64

// Checks the save persists across runs.
static int bootTestBumpBootCount() {
    int bootCount = 0;

    sramRead((void*)BOOT_COUNT_OFFSET, &bootCount, sizeof(bootCount));
    ++bootCount;
    sramWrite((void*)BOOT_COUNT_OFFSET, &bootCount, sizeof(bootCount));

    return bootCount;
}

static int bootTestSramRoundTrip() {
    unsigned int written = ROUNDTRIP_PATTERN;
    unsigned int read = 0;

    sramWrite((void*)ROUNDTRIP_OFFSET, &written, sizeof(written));

    if (!sramRead((void*)ROUNDTRIP_OFFSET, &read, sizeof(read))) {
        return 0;
    }

    // A write past the end of the region must be refused, not wrapped.
    if (sramRead((void*)(SRAM_SIZE - 2), &read, sizeof(read))) {
        return 0;
    }

    sramRead((void*)ROUNDTRIP_OFFSET, &read, sizeof(read));

    return read == ROUNDTRIP_PATTERN;
}

// Posts from its own OSThread, to test the queue across threads.
static void producerThreadEntry(void* arg) {
    while (1) {
        ++sMessagesSent;
        osSendMesg(&sMessageQueue, (OSMesg)(long)sMessagesSent, OS_MESG_BLOCK);
        sceKernelDelayThread(PRODUCER_INTERVAL_US);
    }
}

static void bootTestPrintButton(const char* name, enum ControllerButtons button) {
    pspDebugScreenPrintf("%s ", controllerGetButtons(0, button) ? name : "  ");
}

// Tint the clear from the stick, to show the GU is working.
static unsigned int bootTestClearColor(const struct ControllerStick* stick) {
    unsigned int red   = 64 + (stick->x + 80);
    unsigned int green = 64 + (stick->y + 80);

    return 0xFF000000 | (green << 8) | red;
}

int main() {
    bootPspInit();

    controllersInit();
    displayInit(0);
    cartridgeInit();
    renderTestInit();

    int sramOk = bootTestSramRoundTrip();
    int bootCount = bootTestBumpBootCount();

    // setup = 0 leaves the display to the GU. The format must match the draw
    // buffer or the text renders as garbage.
    pspDebugScreenInitEx(NULL, PSP_DISPLAY_PIXEL_FORMAT_5551, 0);

    osCreateMesgQueue(&sMessageQueue, sMessageBuffer, MESSAGE_QUEUE_LENGTH);

    osCreateThread(
        &sProducerThread,
        PRODUCER_THREAD_ID,
        producerThreadEntry,
        NULL,
        sProducerStack + (sizeof(sProducerStack) / sizeof(*sProducerStack)),
        (OSPri)PRODUCER_THREAD_PRIORITY
    );
    osStartThread(&sProducerThread);

    Time startTime = timeGetTime();

    int messagesReceived = 0;
    long lastMessage = 0;

    while (1) {
        controllersPoll();

        if (controllerGetButtons(0, ControllerButtonStart)) {
            break;
        }

        // Drain whatever the producer posted since the last frame.
        OSMesg message;
        while (osRecvMesg(&sMessageQueue, &message, OS_MESG_NOBLOCK) == 0) {
            lastMessage = (long)message;
            ++messagesReceived;
        }

        struct ControllerStick stick;
        controllerGetStick(0, &stick);

        enum ControllerDirection direction = controllerGetDirection(0);

        displayPspStartFrame();
        sceGuClearColor(bootTestClearColor(&stick));
        sceGuClearDepth(0);
        sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

        renderTestDraw(timeMicroseconds(timeGetTime() - startTime) / 1000000.0f);

        displayPspEndFrame();

        // Text into the buffer the GU just cleared, presented by the swap.
        pspDebugScreenSetOffset((int)displayPspGetDrawBufferOffset());
        pspDebugScreenSetXY(0, 0);

        pspDebugScreenPrintf("Portal 64 PSP -- platform layer\n\n");

        const struct RenderTestState* test = renderTestGetState();

        pspDebugScreenPrintf("lod        %s  bias %+.2f  mips %s  tex %s\n",
            test->autoLod ? "auto " : "fixed",
            test->lodBias,
            test->mipmaps ? "on " : "off",
            test->textured ? "on " : "off");
        pspDebugScreenPrintf("rotation   x %+.2f  y %+.2f\n\n",
            test->rotationX, test->rotationY);

        pspDebugScreenPrintf("nub rotate   dpad L/R bias\n");
        pspDebugScreenPrintf("X lod mode   O mips   [] texture   /\\ reset\n");
        pspDebugScreenPrintf("L debug texture: %s\n\n", test->debugTexture ? "on " : "off");

        pspDebugScreenPrintf("screen     %dx%d  stride %d  %d fps\n",
            SCREEN_WD, SCREEN_HT, SCREEN_STRIDE, displayGetFPS());
        pspDebugScreenPrintf("time       %10llu us\n",
            timeMicroseconds(timeGetTime() - startTime));
        pspDebugScreenPrintf("osGetTime  %10llu us\n\n", (unsigned long long)osGetTime());

        pspDebugScreenPrintf("save       boot #%d  round trip %s\n\n",
            bootCount, sramOk ? "ok" : "FAILED");

        pspDebugScreenPrintf("thread     sent %d\n", sMessagesSent);
        pspDebugScreenPrintf("mesg queue recv %d  last %ld  %s\n\n",
            messagesReceived, lastMessage,
            (messagesReceived == lastMessage) ? "in order" : "MISMATCH");

        pspDebugScreenPrintf("stick      x %4d  y %4d   dir %c%c%c%c\n\n",
            stick.x, stick.y,
            (direction & ControllerDirectionUp)    ? 'U' : '-',
            (direction & ControllerDirectionDown)  ? 'D' : '-',
            (direction & ControllerDirectionLeft)  ? 'L' : '-',
            (direction & ControllerDirectionRight) ? 'R' : '-');

        pspDebugScreenPrintf("buttons    ");
        bootTestPrintButton("A", ControllerButtonA);
        bootTestPrintButton("B", ControllerButtonB);
        bootTestPrintButton("Z", ControllerButtonZ);
        bootTestPrintButton("L", ControllerButtonL);
        bootTestPrintButton("R", ControllerButtonR);
        pspDebugScreenPrintf("\nC cluster  ");
        bootTestPrintButton("CU", ControllerButtonCUp);
        bootTestPrintButton("CD", ControllerButtonCDown);
        bootTestPrintButton("CL", ControllerButtonCLeft);
        bootTestPrintButton("CR", ControllerButtonCRight);
        pspDebugScreenPrintf("\nd-pad      ");
        bootTestPrintButton("U", ControllerButtonUp);
        bootTestPrintButton("D", ControllerButtonDown);
        bootTestPrintButton("L", ControllerButtonLeft);
        bootTestPrintButton("R", ControllerButtonRight);
        pspDebugScreenPrintf("\n\nSTART to exit.\n");

        displayPspSwapBuffers();
    }

    osDestroyThread(&sProducerThread);

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
