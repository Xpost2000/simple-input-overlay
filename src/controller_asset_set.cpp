#include "xbox_controller_asset_id.h"

#include "controller_asset_set.h"

const char* asset_image_list[CONTROLLER_ASSET_SET_COUNT][XBOXCONTROLLER_ASSET_COUNT] = {
    // Xbox Controller
    {
        "Controllers/Xbox/Base.png",
        "Controllers/Xbox/BaseFillmask.png",
        "Controllers/Xbox/ButtonAFillmask.png",
        "Controllers/Xbox/ButtonBFillmask.png",
        "Controllers/Xbox/ButtonXFillmask.png",
        "Controllers/Xbox/ButtonYFillmask.png",
        "Controllers/Xbox/ButtonDpadUpFillmask.png",
        "Controllers/Xbox/ButtonDpadDownFillmask.png",
        "Controllers/Xbox/ButtonDpadLeftFillmask.png",
        "Controllers/Xbox/ButtonDpadRightFillmask.png",
        "Controllers/Xbox/ButtonMenuFillmask.png",
        "Controllers/Xbox/ButtonStartFillmask.png",
        "Controllers/Xbox/ButtonRBFillmask.png",
        "Controllers/Xbox/ButtonLBFillmask.png",
        "Controllers/Xbox/Joystick.png",
        "Controllers/Xbox/JoystickFillMask.png",
        "Controllers/Xbox/RightTrigger.png",
        "Controllers/Xbox/RightTriggerFillmask.png",
        "Controllers/Xbox/LeftTrigger.png",
        "Controllers/Xbox/LeftTriggerFillmask.png",
    },

    // Playstation Controller
    {
        "Controllers/Playstation/Base.png",
        "Controllers/Playstation/BaseFillmask.png",
        "Controllers/Playstation/ButtonCrossFillmask.png",
        "Controllers/Playstation/ButtonCircleFillmask.png",
        "Controllers/Playstation/ButtonSquareFillmask.png",
        "Controllers/Playstation/ButtonTriangleFillmask.png",
        "Controllers/Playstation/ButtonDpadUpFillmask.png",
        "Controllers/Playstation/ButtonDpadDownFillmask.png",
        "Controllers/Playstation/ButtonDpadLeftFillmask.png",
        "Controllers/Playstation/ButtonDpadRightFillmask.png",
        "Controllers/Playstation/ButtonMenuFillmask.png",
        "Controllers/Playstation/ButtonStartFillmask.png",
        "Controllers/Playstation/ButtonRBFillmask.png",
        "Controllers/Playstation/ButtonLBFillmask.png",
        "Controllers/Playstation/Joystick.png",
        "Controllers/Playstation/JoystickFillMask.png",
        "Controllers/Playstation/RightTrigger.png",
        "Controllers/Playstation/RightTriggerFillmask.png",
        "Controllers/Playstation/LeftTrigger.png",
        "Controllers/Playstation/LeftTriggerFillmask.png",
    },
};

const char* asset_set_strings[CONTROLLER_ASSET_SET_COUNT] = {
    "Preset: Xbox Series Controller",
    "Preset: Dualsense Controller",
};
