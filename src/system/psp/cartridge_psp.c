#include "system/cartridge.h"

#include <pspiofilemgr.h>
#include <pspkernel.h>

#include <string.h>

// The N64's 32KB SRAM mirrored in RAM and backed by one file, so
// savefile.c's offsets work unchanged.
#define SAVE_DIRECTORY  "ms0:/PSP/SAVEDATA/PORTAL64"
#define SAVE_PATH       SAVE_DIRECTORY "/portal64.sav"
#define SAVE_TEMP_PATH  SAVE_DIRECTORY "/portal64.sav.tmp"

static unsigned char sSram[SRAM_SIZE];

static int sramRangeIsValid(const void* sramAddr, const int size) {
    unsigned int offset = (unsigned int)sramAddr;

    if (size < 0 || (unsigned int)size > SRAM_SIZE) {
        return 0;
    }

    return offset <= SRAM_SIZE - (unsigned int)size;
}

static void sramLoad() {
    memset(sSram, 0, sizeof(sSram));

    SceUID file = sceIoOpen(SAVE_PATH, PSP_O_RDONLY, 0777);

    if (file < 0) {
        // No save yet: zeroed, like a blank cartridge.
        return;
    }

    sceIoRead(file, sSram, sizeof(sSram));
    sceIoClose(file);
}

// Written to a temp file and renamed, so a power loss keeps the old save.
static void sramStore(const unsigned char* sram) {
    sceIoMkdir("ms0:/PSP/SAVEDATA", 0777);
    sceIoMkdir(SAVE_DIRECTORY, 0777);

    SceUID file = sceIoOpen(SAVE_TEMP_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);

    if (file < 0) {
        // A failed write is non-fatal, as on the N64.
        return;
    }

    int written = sceIoWrite(file, sram, SRAM_SIZE);
    sceIoClose(file);

    if (written != SRAM_SIZE) {
        sceIoRemove(SAVE_TEMP_PATH);
        return;
    }

    sceIoRemove(SAVE_PATH);
    sceIoRename(SAVE_TEMP_PATH, SAVE_PATH);
}

// Saves are written on a low priority thread: on the game thread a
// checkpoint (three saves) froze it for 300ms. Saves during a write fold
// into one more. A save right before HOME may be lost, never torn.
#define SAVE_WRITER_PRIORITY 0x60

static unsigned char sStoreCopy[SRAM_SIZE];
static SceUID sStoreWake = -1;

static int sramWriterThread(SceSize args, void* argp) {
    (void)args;
    (void)argp;

    while (1) {
        sceKernelWaitSema(sStoreWake, 1, 0);

        // A snapshot, so the game can keep writing sSram; changes meanwhile
        // signal another pass.
        memcpy(sStoreCopy, sSram, SRAM_SIZE);
        sramStore(sStoreCopy);
    }

    return 0;
}

void cartridgeInit() {
    sramLoad();

    sStoreWake = sceKernelCreateSema("save", 0, 0, 1, 0);
    SceUID writer = sceKernelCreateThread("save", sramWriterThread, SAVE_WRITER_PRIORITY, 0x2000, 0, 0);

    if (sStoreWake < 0 || writer < 0 || sceKernelStartThread(writer, 0, 0) < 0) {
        sStoreWake = -1;
    }
}

void sramWrite(void* sramAddr, const void* ramAddr, const int size) {
    if (!sramRangeIsValid(sramAddr, size)) {
        return;
    }

    memcpy(sSram + (unsigned int)sramAddr, ramAddr, size);

    if (sStoreWake >= 0) {
        sceKernelSignalSema(sStoreWake, 1);
    } else {
        sramStore(sSram);
    }
}

int sramRead(const void* sramAddr, void* ramAddr, const int size) {
    if (!sramRangeIsValid(sramAddr, size)) {
        return 0;
    }

    memcpy(ramAddr, sSram + (unsigned int)sramAddr, size);
    return 1;
}

int romIsInMemory(const void* address) {
    (void)address;
    // No cartridge: asset pointers are already memory.
    return 1;
}

// Copies baked asset data (chapter images, Valve logo, bind poses) where
// the game can write it. A plain memcpy here; the name is the N64's.
void romCopy(const void* romAddr, void* ramAddr, const int size) {
    memcpy(ramAddr, romAddr, size);
}

// Nothing to overlap: the source is already memory.
void romCopyAsync(const void* romAddr, void* ramAddr, const int size) {
    memcpy(ramAddr, romAddr, size);
}

void romCopyAsyncDrain() {
}
