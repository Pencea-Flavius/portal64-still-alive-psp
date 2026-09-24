#include "translations.h"

#include "util/memory.h"

#include "codegen/assets/strings/strings.h"

char* gLoadedLanugageBlock = NULL;
char** gCurrentTranslations = NULL;
int gCurrentLoadedLanguage = 0;


void translationsReload(int language) {
    if (language == gCurrentLoadedLanguage) {
        return;
    }

    free(gLoadedLanugageBlock);
    translationsLoad(language);
}

int translationsCurrentLanguage() {
    return gCurrentLoadedLanguage;
}

char* translationsGet(int message) {
    if (message < 0 || message >= NUM_TRANSLATED_STRINGS || !gCurrentTranslations) {
        return "";
    }

    return gCurrentTranslations[message];
}