#ifndef KEYBOARD_ASSET_SET_H
#define KEYBOARD_ASSET_SET_H

enum KeyboardAssetSet {
    KEYBOARD_ASSET_SET_UNKNOWN      = -1,
    KEYBOARD_ASSET_SET_ALPHANUMERIC = 0,
    KEYBOARD_ASSET_SET_TENKEYLESS,
    KEYBOARD_ASSET_SET_FULLSIZE,
    KEYBOARD_ASSET_SET_COUNT,
};

extern const char* keyboard_asset_image_list[KEYBOARD_ASSET_SET_COUNT][KEYBOARD_ASSET_COUNT];
extern const char* keyboard_asset_set_strings[KEYBOARD_ASSET_SET_COUNT];

#endif
