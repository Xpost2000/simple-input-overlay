#ifndef MOUSE_ASSET_SET_H
#define MOUSE_ASSET_SET_H

enum MouseAssetSet {
    MOUSE_ASSET_SET_BASE = 0,
    MOUSE_ASSET_SET_COUNT,
};

extern const char* mouse_asset_image_list[MOUSE_ASSET_SET_COUNT][MOUSE_ASSET_COUNT];
extern const char* mouse_asset_set_strings[MOUSE_ASSET_SET_COUNT];

#endif
