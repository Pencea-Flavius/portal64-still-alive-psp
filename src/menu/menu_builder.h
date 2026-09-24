#ifndef __MENU_MENU_BUILDER_H__
#define __MENU_MENU_BUILDER_H__

#include "font/font.h"
#include "graphics/renderstate.h"
#include "menu.h"

struct MenuBuilderElement;
struct MenuElementParams;

enum MenuElementType {
    MenuElementTypeText,
    MenuElementTypeCheckbox,
    MenuElementTypeSlider,
};

struct MenuAction {
    enum MenuElementType type;
    union
    {
        struct {
            unsigned char isChecked;
        } checkbox;
        struct {
            float value;
        } fSlider;
        struct {
            short value;
        } iSlider;
    } state;
};

typedef void (*MenuActionCalback)(void* data, int selection, struct MenuAction* action);

typedef void (*MenuItemInit)(struct MenuBuilderElement* element);
typedef enum InputCapture (*MenuItemUpdate)(struct MenuBuilderElement* element, MenuActionCalback actionCallback, void* data);
typedef void (*MenuItemRebuildText)(struct MenuBuilderElement* element, int deferFree);
typedef void (*MenuItemRender)(struct MenuBuilderElement* element, int selection, int materialIndex, struct PrerenderedTextBatch* textBatch, struct RenderState* renderState);

struct MenuBuilderCallbacks {
    MenuItemInit init;
    MenuItemUpdate update;
    MenuItemRebuildText rebuildText;
    MenuItemRender render;
};

struct MenuElementParams {
    enum MenuElementType type;
    short x;
    short y;
    union
    {
        struct {
            struct Font* font;
            short messageId;
            char* message;
            char rightAlign;
        } text;
        struct {
            struct Font* font;
            short messageId;
            // Shown instead of messageId when set, for an option with no
            // translation.
            char* message;
        } checkbox;
        struct {
            struct Font* font;
            short messageId;
            short width;
            short numberOfTicks;
            short discrete;
        } slider;
    } params;
    short selectionIndex;
};

struct MenuBuilderElement {
    void* data;
    struct MenuBuilderCallbacks* callbacks;
    struct MenuElementParams* params;
    short selectionIndex;
};

struct MenuBuilder {
    struct MenuBuilderElement* elements;
    MenuActionCalback actionCallback;
    void* data;
    short elementCount;
    short selection;
    short maxSelection;
};

void menuBuilderInit(
    struct MenuBuilder* menuBuilder, 
    struct MenuElementParams* params, 
    int elementCount, 
    int maxSelection, 
    MenuActionCalback actionCallback, 
    void* data
);
enum InputCapture menuBuilderUpdate(struct MenuBuilder* menuBuilder);
void menuBuilderRebuildText(struct MenuBuilder* menuBuilder);

void menuBuilderSetCheckbox(struct MenuBuilderElement* element, int value);
void menuBuilderSetFSlider(struct MenuBuilderElement* element, float value);
void menuBuilderSetISlider(struct MenuBuilderElement* element, int value);

// Everything that draws is declared in the platform's half, which the build
// puts on the include path.
#include "menu_builder_render.h"

#endif