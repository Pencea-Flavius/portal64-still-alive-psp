#include "strings/translations.h"

#include <stddef.h>

#include "codegen/assets/strings/strings.h"

// The PSP half of loading a language; see src/strings/n64/translations_load.c.
// Every language is linked, so nothing is loaded or relocated.
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

    // The N64 keeps the copy so it can be freed when the language changes.
    // There is no copy, and translationsReload() frees this before calling
    // back in, so it has to stay null rather than point at linked data.
    gLoadedLanugageBlock = NULL;

    gCurrentTranslations = StringLanguageBlocks[language].values;
    gCurrentLoadedLanguage = language;
}
