#include "strings/translations.h"

#include "system/cartridge.h"
#include "util/memory.h"

#include "codegen/assets/strings/strings.h"

// The N64 half of loading a language (src/strings/psp/...): copy the
// language's ROM segment in and shift its string pointers.

void translationsLoad(int language) {
    if (NUM_STRING_LANGUAGES == 0) {
        gCurrentTranslations = NULL;
        return;
    }

    if (language < 0) {
        language = 0;
    }

    if (language >= NUM_STRING_LANGUAGES) {
        language = NUM_STRING_LANGUAGES - 1;
    }

    struct StringBlock* block = &StringLanguageBlocks[language];

    int blockSize = (int)block->romEnd - (int)block->romStart;
    gLoadedLanugageBlock = malloc(blockSize);
    romCopy(block->romStart, gLoadedLanugageBlock, blockSize);

    gCurrentTranslations = CALC_SEGMENT_POINTER(block->values, gLoadedLanugageBlock);
    gCurrentLoadedLanguage = language;

    for (int i = 0; i < NUM_TRANSLATED_STRINGS; ++i) {
        gCurrentTranslations[i] = CALC_SEGMENT_POINTER(gCurrentTranslations[i], gLoadedLanugageBlock);
    }
}
