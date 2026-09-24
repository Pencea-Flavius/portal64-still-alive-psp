#include <ultra64.h>

#include "os_psp_internal.h"

#include <pspthreadman.h>

#define OS_THREAD_STACK_SIZE    0x4000

// libultra priorities count upwards towards more important; PSP priorities
// count downwards. Portal 64 uses 10..13, so this base keeps them inside the
// usable user range while preserving their order.
#define PSP_PRIORITY_BASE       42
#define PSP_PRIORITY_HIGHEST    8
#define PSP_PRIORITY_LOWEST     111

static int pspPriorityFromOs(OSPri priority) {
    int pspPriority = PSP_PRIORITY_BASE - (int)priority;

    if (pspPriority < PSP_PRIORITY_HIGHEST) {
        return PSP_PRIORITY_HIGHEST;
    }

    if (pspPriority > PSP_PRIORITY_LOWEST) {
        return PSP_PRIORITY_LOWEST;
    }

    return pspPriority;
}

static int threadTrampoline(SceSize args, void* argp) {
    OSThread* thread = *(OSThread**)argp;

    thread->entry(thread->arg);

    return 0;
}

void osInitialize(void) {
    // The PSP kernel is already running by the time a homebrew binary reaches
    // main(), so this only sets up what the shim itself owns.
    osPspMessageInit();
}

void osCreateThread(OSThread* thread, OSId id, void (*entry)(void*), void* arg, void* stackEnd, OSPri priority) {
    // The caller supplies an N64 stack; the PSP kernel allocates its own, so
    // stackEnd is ignored.
    (void)stackEnd;

    thread->id = id;
    thread->priority = priority;
    thread->entry = entry;
    thread->arg = arg;
    thread->pspThreadId = sceKernelCreateThread(
        "portal64_os_thread",
        threadTrampoline,
        pspPriorityFromOs(priority),
        OS_THREAD_STACK_SIZE,
        PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU,
        NULL
    );
}

void osStartThread(OSThread* thread) {
    if (thread->pspThreadId < 0) {
        return;
    }

    sceKernelStartThread(thread->pspThreadId, sizeof(OSThread*), &thread);
}

void osStopThread(OSThread* thread) {
    if (thread == NULL || thread->pspThreadId < 0) {
        return;
    }

    sceKernelSuspendThread(thread->pspThreadId);
}

void osDestroyThread(OSThread* thread) {
    // libultra takes NULL to mean the calling thread.
    if (thread == NULL) {
        sceKernelExitDeleteThread(0);
        return;
    }

    if (thread->pspThreadId < 0) {
        return;
    }

    sceKernelTerminateDeleteThread(thread->pspThreadId);
    thread->pspThreadId = -1;
}

void osYieldThread(void) {
    sceKernelDelayThread(0);
}

void osSetThreadPri(OSThread* thread, OSPri priority) {
    if (thread == NULL || thread->pspThreadId < 0) {
        return;
    }

    thread->priority = priority;
    sceKernelChangeThreadPriority(thread->pspThreadId, pspPriorityFromOs(priority));
}

OSId osGetThreadId(OSThread* thread) {
    return thread ? thread->id : 0;
}
