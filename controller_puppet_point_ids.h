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
 */

enum ControllerPuppetPointId {
    CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT = 0,
    CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT,
    CONTROLLER_PUPPET_POINT_LEFT_TRIGGER,
    CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER,

    CONTROLLER_PUPPET_POINT_COUNT
};

#endif
