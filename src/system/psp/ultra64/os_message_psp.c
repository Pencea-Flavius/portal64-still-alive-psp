#include <ultra64.h>

#include "os_psp_internal.h"

#include <pspthreadman.h>

// One semaphore for every queue: one per queue can exhaust the kernel's
// object slots on hardware.
//
// Blocking waits poll every 200us for the same reason.
static SceUID sQueueLock = -1;

// Blocking waits poll instead of sleeping on a per-queue semaphore, for the
// same reason. 200us is well under a frame, so a blocked consumer still wakes
// within the frame the producer posted in.
#define BLOCK_POLL_MICROSECONDS 200

// Also created here, for queues used before osInitialize() (no threads yet,
// so no race).
static void queueLockAcquire(void) {
    if (sQueueLock < 0) {
        osPspMessageInit();
    }

    sceKernelWaitSema(sQueueLock, 1, NULL);
}

static void queueLockRelease(void) {
    sceKernelSignalSema(sQueueLock, 1);
}

void osPspMessageInit(void) {
    if (sQueueLock < 0) {
        sQueueLock = sceKernelCreateSema("portal64_mesgq", 0, 1, 1, NULL);
    }
}

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msgBuffer, s32 count) {
    mq->msg = msgBuffer;
    mq->msgCount = count;
    mq->validCount = 0;
    mq->first = 0;
}

s32 osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flags) {
    while (1) {
        queueLockAcquire();

        if (mq->validCount < mq->msgCount) {
            s32 slot = (mq->first + mq->validCount) % mq->msgCount;
            mq->msg[slot] = msg;
            ++mq->validCount;

            queueLockRelease();
            return 0;
        }

        queueLockRelease();

        if (flags == OS_MESG_NOBLOCK) {
            return -1;
        }

        sceKernelDelayThread(BLOCK_POLL_MICROSECONDS);
    }
}

s32 osJamMesg(OSMesgQueue* mq, OSMesg msg, s32 flags) {
    while (1) {
        queueLockAcquire();

        if (mq->validCount < mq->msgCount) {
            mq->first = (mq->first + mq->msgCount - 1) % mq->msgCount;
            mq->msg[mq->first] = msg;
            ++mq->validCount;

            queueLockRelease();
            return 0;
        }

        queueLockRelease();

        if (flags == OS_MESG_NOBLOCK) {
            return -1;
        }

        sceKernelDelayThread(BLOCK_POLL_MICROSECONDS);
    }
}

s32 osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flags) {
    while (1) {
        queueLockAcquire();

        if (mq->validCount > 0) {
            // libultra allows a NULL destination, which discards the message.
            if (msg != NULL) {
                *msg = mq->msg[mq->first];
            }

            mq->first = (mq->first + 1) % mq->msgCount;
            --mq->validCount;

            queueLockRelease();
            return 0;
        }

        queueLockRelease();

        if (flags == OS_MESG_NOBLOCK) {
            return -1;
        }

        sceKernelDelayThread(BLOCK_POLL_MICROSECONDS);
    }
}
