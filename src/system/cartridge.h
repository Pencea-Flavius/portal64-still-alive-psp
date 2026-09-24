#ifndef __CARTRIDGE_H__
#define __CARTRIDGE_H__

#define SRAM_SIZE 0x8000
#define CALC_SEGMENT_POINTER(segmentedAddress, baseAddress) (void*)(((unsigned)(segmentedAddress) & 0xFFFFFF) + (baseAddress))

void cartridgeInit();

// Whether an address is already readable memory rather than something
// romCopy() has to fetch. The N64 asks this of pointers the asset pipeline
// baked in, which may be cartridge addresses; on a machine with no cartridge
// every pointer is already memory.
int romIsInMemory(const void* address);

void romCopy(const void* romAddr, void* ramAddr, const int size);
void romCopyAsync(const void* romAddr, void* ramAddr, const int size);
void romCopyAsyncDrain();

void sramWrite(void* sramAddr, const void* ramAddr, const int size);
int sramRead(const void* sramAddr, void* ramAddr, const int size);

#endif
