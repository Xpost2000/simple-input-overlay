#include "constants.h"

#include <SDL2/SDL.h>

#include "controller_puppet_point_ids.h"
#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"
#include "controller_asset_set.h"

#include "config.h"

#include <assert.h>

#include "input_layout_visual.h"

extern SDL_Texture* g_xbox_controller_assets[20];
extern SDL_Texture* g_playstation_controller_assets[20];

extern SDL_Point g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT];
extern SDL_Point g_playstation_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT];

static int g_part_id_to_button[64] = {};

// NOTE: input is obtained here directly,
// ideally input state is retrieved else where though as part of a refactor!

void init_controller_keymap(void)
{
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_A_FILL]     = SDL_CONTROLLER_BUTTON_A;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_B_FILL]     = SDL_CONTROLLER_BUTTON_B;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_X_FILL]     = SDL_CONTROLLER_BUTTON_X;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_Y_FILL]     = SDL_CONTROLLER_BUTTON_Y;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_DPAD_UP_FILL]      = SDL_CONTROLLER_BUTTON_DPAD_UP;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_DPAD_DOWN_FILL]    = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_DPAD_LEFT_FILL]    = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_DPAD_RIGHT_FILL]   = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_LB_FILL]           = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_RB_FILL]           = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_MENU_FILL]  = SDL_CONTROLLER_BUTTON_BACK;
    g_part_id_to_button[XBOXCONTROLLER_ASSET_BUTTON_START_FILL] = SDL_CONTROLLER_BUTTON_START;
}

static inline SDL_Color lerp_color(SDL_Color a, SDL_Color b, float t) {
    SDL_Color result;
    result.r = (a.r/255.0f * (1.0 - t) + t * b.r/255.0f) * 255;
    result.g = (a.g/255.0f * (1.0 - t) + t * b.g/255.0f) * 255;
    result.b = (a.b/255.0f * (1.0 - t) + t * b.b/255.0f) * 255;
    result.a = (a.a/255.0f * (1.0 - t) + t * b.a/255.0f) * 255;
    return result;
}

static inline void draw_controller_part_joystick(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    auto point = point_set[part_id];
    SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, point.y / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, SCALED_JOYSTICK_SZ, SCALED_JOYSTICK_SZ};

    bool  is_left = (part_id == CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT);
    short axis_x  = (controller) ? SDL_GameControllerGetAxis(controller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX) : 0;
    short axis_y  = (controller) ? SDL_GameControllerGetAxis(controller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY) : 0;

    destination.x += axis_x/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;
    destination.y += axis_y/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;

    if (SDL_GameControllerGetButton(controller, (is_left) ? SDL_CONTROLLER_BUTTON_LEFTSTICK : SDL_CONTROLLER_BUTTON_RIGHTSTICK)) {
        SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], g_settings.activated_color.r, g_settings.activated_color.g, g_settings.activated_color.b);
    } else {
        SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], g_settings.button_color.r, g_settings.button_color.g, g_settings.button_color.b);
    }

    SDL_RenderCopy(renderer, asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], 0, &destination);

    SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 255, 255, 255);
    SDL_RenderCopy(renderer, asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 0, &destination);
}

static inline void draw_controller_part_trigger(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    auto point = point_set[part_id];
    SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO, point.y / IMAGE_SCALE_RATIO, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT};

    bool is_left       = (part_id == CONTROLLER_PUPPET_POINT_LEFT_TRIGGER);
    int  fillmask_id   = (is_left) ? XBOXCONTROLLER_ASSET_LT_FILL : XBOXCONTROLLER_ASSET_RT_FILL;
    int  partsprite_id = (is_left) ? XBOXCONTROLLER_ASSET_LT : XBOXCONTROLLER_ASSET_RT;

    short axis_y = (controller) ?
        SDL_GameControllerGetAxis(controller,
                                  (is_left) ? SDL_CONTROLLER_AXIS_TRIGGERLEFT : SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        : 0;

    destination.y += axis_y/32767.0f * SCALED_MAX_TRIGGER_DISPLACEMENT_PX;

    {
        auto color = lerp_color(g_settings.button_color, g_settings.activated_color, axis_y / 32767.0f);
        SDL_SetTextureColorMod(asset_set[fillmask_id], color.r, color.g, color.b);
    }

    SDL_RenderCopy(renderer, asset_set[fillmask_id], 0, &destination);

    SDL_SetTextureColorMod(asset_set[partsprite_id], 255, 255, 255);
    SDL_RenderCopy(renderer, asset_set[partsprite_id], 0, &destination);
}

static void draw_controller_puppet_part(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    switch (part_id) {
        case CONTROLLER_PUPPET_POINT_LEFT_TRIGGER:
        case CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER: {
            draw_controller_part_trigger(renderer, controller, g_settings, part_id, asset_set, point_set);
        } break;

        case CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT:
        case CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT: {
            draw_controller_part_joystick(renderer, controller, g_settings, part_id, asset_set, point_set);
        } break;
    }
}

static void set_controller_visual_focus(SDL_Texture** asset_set, SDL_GameController* controller)
{
    for (unsigned part_index = 0; part_index < XBOXCONTROLLER_ASSET_COUNT; ++part_index) {
        if (controller) {
            SDL_SetTextureAlphaMod(asset_set[part_index], 255);
        } else {
            SDL_SetTextureAlphaMod(asset_set[part_index], 64);
        }
    }
}

void draw_controller(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, ControllerAssetSet asset_set)
{
    SDL_Texture** controller_asset_set = g_xbox_controller_assets;
    SDL_Point*    puppeter_point_set   = g_xbox_controller_puppet_piece_placements;

    switch (asset_set) {
        case CONTROLLER_ASSET_SET_XBOX: {
            controller_asset_set = g_xbox_controller_assets;
            puppeter_point_set = g_xbox_controller_puppet_piece_placements;
        } break;
        case CONTROLLER_ASSET_SET_PLAYSTATION: {
            controller_asset_set = g_playstation_controller_assets;
            puppeter_point_set = g_playstation_controller_puppet_piece_placements;
        } break;
    }

    set_controller_visual_focus(controller_asset_set, controller);
    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_LEFT_TRIGGER, controller_asset_set, puppeter_point_set);
    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER, controller_asset_set, puppeter_point_set);

    // NOTE: enums are Xbox to be more obvious to read, but they are aligned for all controller types.
    // Draw_All_Controller_Parts.
    {
        SDL_Rect destination = { 0, 0, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT };
        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], g_settings.controller_color.r, g_settings.controller_color.g, g_settings.controller_color.b);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], 0, &destination);

        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 255, 255, 255);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 0, &destination);

        for (unsigned part_index = XBOXCONTROLLER_ASSET_BUTTON_A_FILL; part_index < XBOXCONTROLLER_ASSET_JOYSTICK; ++part_index) {
            if (controller && SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)g_part_id_to_button[part_index])) {
                SDL_SetTextureColorMod(controller_asset_set[part_index], g_settings.activated_color.r, g_settings.activated_color.g, g_settings.activated_color.b);
            } else {
                SDL_SetTextureColorMod(controller_asset_set[part_index], g_settings.button_color.r, g_settings.button_color.g, g_settings.button_color.b);
            }

            SDL_RenderCopy(renderer, controller_asset_set[part_index], 0, &destination);
        }
    }

    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT, controller_asset_set, puppeter_point_set);
    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT, controller_asset_set, puppeter_point_set);
}

// Keylogging is apparently difficult, so this one might not get filled.
// It's easy enough to do on Windows with an LL hook.
//
// Look into RECORD extension for X11.
//
void draw_keyboard(SDL_Renderer* renderer, const OverlaySettings& g_settings)
{
    assert(0 && "Not implemented.");
}
