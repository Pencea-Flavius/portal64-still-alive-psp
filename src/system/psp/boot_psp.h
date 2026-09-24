#ifndef __BOOT_PSP_H__
#define __BOOT_PSP_H__

// Call once at the top of main(). The PSP hangs when the player exits from the
// home menu unless an exit callback is registered, so this is not optional.
void bootPspInit(void);

#endif
