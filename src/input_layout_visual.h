#ifndef INPUT_LAYOUT_VISUAL_H
#define INPUT_LAYOUT_VISUAL_H

// inc SDL.h

void init_controller_keymap(void);
void draw_controller(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, ControllerAssetSet asset_set);
void draw_keyboard(SDL_Renderer* renderer, const OverlaySettings& g_settings, Uint8* keystate, KeyboardAssetSet asset_set);

// NOTE: only one mouse asset type.
void draw_mouse(SDL_Renderer* renderer, const OverlaySettings& g_settingsm, MouseData* mouse_data);

// NOTE: the method I do the rendering (which requires no config) is already kind of wasteful, so
// here's my attempt to try and save some memory.
void load_xbox_controller_assets(void);
void load_playstation_controller_assets(void);
void load_keyboard_key_assets(void);
void load_mouse_assets(void);

void unload_xbox_controller_assets(void);
void unload_playstation_controller_assets(void);
void unload_keyboard_key_assets(void);
void unload_mouse_assets(void);

void unload_controller_assets(void);
void load_controller_assets(void);

void set_global_controller_asset_set(ControllerAssetSet controller_asset_set);
void set_global_keyboard_asset_set(KeyboardAssetSet keyboard_asset_set);
void set_global_mouse_asset(void);

void get_current_recommended_screen_size(const OverlaySettings& g_settings, int* window_width, int* window_height);

#endif
