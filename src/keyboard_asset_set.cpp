#include "keyboard_asset_id.h"
#include "keyboard_asset_set.h"

#define Common_Keyboard_Asset_List              \
    "Controllers/KeyboardCommon/U1.png",        \
    "Controllers/KeyboardCommon/U125.png",      \
    "Controllers/KeyboardCommon/U150.png",      \
    "Controllers/KeyboardCommon/U175.png",      \
    "Controllers/KeyboardCommon/U2.png",        \
    "Controllers/KeyboardCommon/U225.png",      \
    "Controllers/KeyboardCommon/U275.png",      \
    "Controllers/KeyboardCommon/USpacebar.png", \
    "Controllers/KeyboardCommon/VU225.png",
    

const char* keyboard_asset_image_list[KEYBOARD_ASSET_SET_COUNT][KEYBOARD_ASSET_COUNT] = {
    { // ALPHANUMERIC
        "Controllers/KeyboardAlphanumeric/Base.png",
        Common_Keyboard_Asset_List
    },
    { // TENKEYLESS
        "Controllers/KeyboardTenkeyless/Base.png",
        Common_Keyboard_Asset_List
    },
    { // FULLSIZE
        "Controllers/KeyboardFullsize/Base.png",
        Common_Keyboard_Asset_List
    }
};

const char* keyboard_asset_set_strings[KEYBOARD_ASSET_SET_COUNT] = {
    "Preset: Keyboard Alphanumeric",
    "Preset: Keyboard Ten-keyless",
    "Preset: Keyboard Fullsize 104",
};
