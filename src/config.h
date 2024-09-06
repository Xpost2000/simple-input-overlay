#ifndef CONFIG_H
#define CONFIG_H

// inc SDL2.h

struct OverlaySettings {
    OverlaySettings(void);

    SDL_Color controller_color;
    SDL_Color button_color;
    SDL_Color activated_color;
    int       image_scale_ratio;

    int       last_device_mode_id;
    int       last_device_asset_set_id;
    bool      autodetect_controller;
    bool      always_on_top;
    bool      use_mouse_move;
};

void write_config(const OverlaySettings& settings);
void load_config(OverlaySettings& settings);

SDL_Color color_from_hex_string(const char* hexstr);
char*     color_into_hex_string(SDL_Color color); // NOTE: temp buffer. Not safe to keep.

#endif
