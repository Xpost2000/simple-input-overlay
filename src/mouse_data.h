#ifndef MOUSE_DATA_H
#define MOUSE_DATA_H

struct MouseData {
    int x;
    int y;

    int move_x;
    int move_y;

    bool buttons[3];
};

#endif
