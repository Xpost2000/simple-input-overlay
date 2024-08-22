#ifndef CONTROLLER_ASSET_SET_H
#define CONTROLLER_ASSET_SET_H

enum ControllerAssetSet {
    CONTROLLER_ASSET_SET_XBOX = 0,
    CONTROLLER_ASSET_SET_PLAYSTATION = 1,
    CONTROLLER_ASSET_SET_COUNT,
};

// defined in controller_asset_set.cpp
extern const char* asset_image_list[CONTROLLER_ASSET_SET_COUNT][XBOXCONTROLLER_ASSET_COUNT];

#endif
