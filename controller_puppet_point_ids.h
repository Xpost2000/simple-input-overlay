#ifndef CONTROLLER_PUPPET_POINT_IDS_H
#define CONTROLLER_PUPPET_POINT_IDS_H

/*
 * Most controllers can just be done by just blitting a wasteful,
 * but easily masked fullscreen image, which is fine since it's quicker
 * to just overlay images like this.
 *
 * It's not a big program anyway...
 *
 * However, for nicer visuals, some stuff like the joystick and the triggers (anything that
 * has a non-binary input), it will require some puppetering to look good.
 *
 *
 * NOTE: for optimization all things will be converted to puppet points so that I can programmatically "turn" them
 * on, and it'll also just be easier for me to support more types of customization, although tbh I'm not exactly intending
 * to expand the way this is done in any significant way.
 *
 * At some point it'd just be too generic...
 */

enum ControllerPuppetPointId {
    CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT = 0,
    CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT,
    CONTROLLER_PUPPET_POINT_LEFT_TRIGGER,
    CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER,

    CONTROLLER_PUPPET_POINT_BUTTON_Y,
    CONTROLLER_PUPPET_POINT_BUTTON_X,
    CONTROLLER_PUPPET_POINT_BUTTON_A,
    CONTROLLER_PUPPET_POINT_BUTTON_B,

    CONTROLLER_PUPPET_POINT_BUTTON_MENU,
    CONTROLLER_PUPPET_POINT_BUTTON_START,

    CONTROLLER_PUPPET_POINT_BUTTON_DPAD_UP,
    CONTROLLER_PUPPET_POINT_BUTTON_DPAD_DOWN,
    CONTROLLER_PUPPET_POINT_BUTTON_DPAD_LEFT,
    CONTROLLER_PUPPET_POINT_BUTTON_DPAD_RIGHT,

    CONTROLLER_PUPPET_POINT_BUTTON_LB,
    CONTROLLER_PUPPET_POINT_BUTTON_RB,

    CONTROLLER_PUPPET_POINT_COUNT
};

#endif
