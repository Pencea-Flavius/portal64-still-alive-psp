#ifndef __MENU_TRANSLATIONS_H__
#define __MENU_TRANSLATIONS_H__

void translationsLoad(int language);
void translationsReload(int language);
int translationsCurrentLanguage();

char* translationsGet(int message);

// Loading a language is split per machine; the build picks which .c defines
// translationsLoad(). These are what both halves write.
extern char* gLoadedLanugageBlock;
extern char** gCurrentTranslations;
extern int gCurrentLoadedLanguage;

#endif