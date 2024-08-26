#ifndef INPUT_LAYOUT_VISUAL_H
#define INPUT_LAYOUT_VISUAL_H

// inc SDL.h

void init_controller_keymap(void);
void draw_controller(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, ControllerAssetSet asset_set);
void draw_keyboard(SDL_Renderer* renderer, const OverlaySettings& g_settings, Uint8* keystate, KeyboardAssetSet asset_set);

// NOTE: the method I do the rendering (which requires no config) is already kind of wasteful, so
// here's my attempt to try and save some memory.
void load_xbox_controller_assets(void);
void load_playstation_controller_assets(void);
void load_keyboard_key_assets(void);

void unload_xbox_controller_assets(void);
void unload_playstation_controller_assets(void);
void unload_keyboard_key_assets(void);

void unload_controller_assets(void);
void load_controller_assets(void);

void set_global_controller_asset_set(ControllerAssetSet controller_asset_set);
void set_global_keyboard_asset_set(KeyboardAssetSet keyboard_asset_set);

#endif
