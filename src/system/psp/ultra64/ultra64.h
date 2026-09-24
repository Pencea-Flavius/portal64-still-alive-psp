#ifndef __ULTRA64_PSP_H__
#define __ULTRA64_PSP_H__

// A PSP stand-in for the parts of <ultra64.h> the game calls directly, on
// sceKernel. Not derived from the N64 SDK. Graphics types are left to the
// native renderer.

#include <psptypes.h>

typedef s32 OSPri;
typedef s32 OSId;
typedef u64 OSTime;
typedef void* OSMesg;

#define OS_PRIORITY_IDLE    0

#define OS_MESG_NOBLOCK     0
#define OS_MESG_BLOCK       1

// libultra hands the thread struct to the caller to own, so it has to be a
// complete type. The PSP only needs enough of it to find its kernel thread.
typedef struct OSThread {
    OSId    id;
    OSPri   priority;
    int     pspThreadId;
    void    (*entry)(void*);
    void*   arg;
} OSThread;

// Field names follow libultra's so the ring-buffer bookkeeping reads the same.
typedef struct OSMesgQueue {
    OSMesg* msg;
    s32     msgCount;
    s32     validCount;
    s32     first;
} OSMesgQueue;

void osInitialize(void);

void osCreateThread(OSThread* thread, OSId id, void (*entry)(void*), void* arg, void* stackEnd, OSPri priority);
void osStartThread(OSThread* thread);
void osStopThread(OSThread* thread);
void osDestroyThread(OSThread* thread);
void osYieldThread(void);
void osSetThreadPri(OSThread* thread, OSPri priority);
OSId osGetThreadId(OSThread* thread);

void osCreateMesgQueue(OSMesgQueue* mq, OSMesg* msgBuffer, s32 count);
s32  osSendMesg(OSMesgQueue* mq, OSMesg msg, s32 flags);
s32  osJamMesg(OSMesgQueue* mq, OSMesg msg, s32 flags);
s32  osRecvMesg(OSMesgQueue* mq, OSMesg* msg, s32 flags);

OSTime osGetTime(void);

#endif
