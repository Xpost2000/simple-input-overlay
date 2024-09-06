#ifndef DEVICE_MODE_H
#define DEVICE_MODE_H

enum DeviceMode {
    DEVICE_MODE_USING_CONTROLLER,
    DEVICE_MODE_USING_KEYBOARD,
    DEVICE_MODE_USING_MOUSE,
    DEVICE_MODE_COUNT,
    DEVICE_MODE_UNKNOWN=-1,
};

static const char* device_mode_strings[DEVICE_MODE_COUNT] = {
    "Controller",
    "Keyboard",
    "Mouse"
};

#endif
