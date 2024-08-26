#include "keyboard_asset_id.h"
#include "keyboard_asset_set.h"

const char* keyboard_asset_image_list[KEYBOARD_ASSET_SET_COUNT][KEYBOARD_ASSET_COUNT] = {
    { // ALPHANUMERIC
        "Controllers/KeyboardAlphanumeric/Base.png"
    },
    { // TENKEYLESS
        "Controllers/KeyboardTenkeyless/Base.png"
    },
    { // FULLSIZE
        "Controllers/KeyboardFullsize/Base.png"
    }
};

const char* keyboard_asset_set_strings[KEYBOARD_ASSET_SET_COUNT] = {
    "Preset: Keyboard Alphanumeric",
    "Preset: Keyboard Ten-keyless",
    "Preset: Keyboard Fullsize 104",
};
