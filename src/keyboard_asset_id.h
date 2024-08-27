#ifndef KEYBOARD_ASSET_ID_H
#define KEYBOARD_ASSET_ID_H

// an asset per key.
enum KeyboardAssetId {
    KEYBOARD_ASSET_BASE,

    KEYBOARD_ASSET_KEY_U1,
    KEYBOARD_ASSET_KEY_U125, // control/windows etc.
    KEYBOARD_ASSET_KEY_U150, // tab
    KEYBOARD_ASSET_KEY_U175, // capslock
    KEYBOARD_ASSET_KEY_U2,   // backspace
    KEYBOARD_ASSET_KEY_U225, // enter
    KEYBOARD_ASSET_KEY_U275, // rshift
    KEYBOARD_ASSET_KEY_U625, // space bar

    // vertical
    KEYBOARD_ASSET_KEY_VU225, // enter
    KEYBOARD_ASSET_COUNT,
};

#endif
