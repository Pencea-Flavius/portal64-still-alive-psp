#ifndef __LEVEL_METADATA_H__
#define __LEVEL_METADATA_H__

#include "level_definition.h"

// The three segment fields are the N64's, for the same reason
// struct DynamicAssetModel's are: a level lives in a ROM segment that is
// copied into RAM when it is loaded, because the machine has 4MB. Every level
// is linked and resident on the PSP, so there is nothing to copy.
struct LevelMetadata {
    struct LevelDefinition* levelDefinition;
#ifndef PSP
    char* segmentRomStart;
    char* segmentRomEnd;
    char* segmentStart;
#endif
};

#endif