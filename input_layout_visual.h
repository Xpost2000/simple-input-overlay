#ifndef INPUT_LAYOUT_VISUAL_H
#define INPUT_LAYOUT_VISUAL_H

// inc SDL.h

void init_controller_keymap(void);
void draw_controller(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, ControllerAssetSet asset_set);
void draw_keyboard(SDL_Renderer* renderer, const OverlaySettings& g_settings);

#endif
