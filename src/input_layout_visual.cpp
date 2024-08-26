#include "constants.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "controller_puppet_point_ids.h"

#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"
#include "controller_asset_set.h"

#include "keyboard_puppet_point_ids.h"

#include "keyboard_asset_id.h"
#include "keyboard_asset_set.h"

#include "config.h"

#include <assert.h>

#include "input_layout_visual.h"

extern SDL_Renderer*      g_renderer;
extern ControllerAssetSet g_asset_set;
extern KeyboardAssetSet   g_keyboard_asset_set;
extern bool               g_using_keyboard;

SDL_Texture* g_xbox_controller_assets[XBOXCONTROLLER_ASSET_COUNT]               = {};
SDL_Texture* g_playstation_controller_assets[PLAYSTATIONCONTROLLER_ASSET_COUNT] = {};
SDL_Texture* g_keyboard_alphanumeric_assets[KEYBOARD_ASSET_COUNT]               = {};
SDL_Texture* g_keyboard_tenkeyless_assets[KEYBOARD_ASSET_COUNT]                 = {};
SDL_Texture* g_keyboard_fullsize_assets[KEYBOARD_ASSET_COUNT]                   = {};

// TODO: make tunable?
// NOTE: centered coordinates
SDL_Point g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {373, 416},
    {907, 620},
    {0, 0},
    {0, 0},

    // Face buttons
    {1091, 319},
    {995, 408},
    {1087, 495},
    {1185, 402},

    // Small buttons
    {628, 412},
    {830, 412},

    {550, 552},
    {550, 680},
    {482, 620},
    {617, 620},
    
    {395, 139},
    {1066, 140},
};

SDL_Point g_playstation_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {519, 634},
    {931, 634},
    {0, 0},
    {0, 0},

    // Face buttons
    {1156, 357},
    {1051, 448},
    {1154, 528},
    {1256, 445},

    // Small buttons
    {411, 311},
    {1046, 311},

    {303, 385},
    {309, 506},
    {246, 446},
    {372, 446},
    
    {304, 184},
    {1152, 184},
};

static SDL_Point g_keyboard_puppet_piece_placements[KEYBOARD_PUPPET_POINT_COUNT] = {
    
};

static int g_keyboard_puppet_piece_to_asset_id[KEYBOARD_PUPPET_POINT_COUNT] = {
    
};

static SDL_Point* get_controller_point_set(ControllerAssetSet asset_set)
{
    switch (asset_set) {
        case CONTROLLER_ASSET_SET_XBOX: {
            return g_xbox_controller_puppet_piece_placements;
        } break;
        case CONTROLLER_ASSET_SET_PLAYSTATION: {
            return g_playstation_controller_puppet_piece_placements;
        } break;
    }

    return nullptr;
}

static SDL_Texture** get_controller_asset_set(ControllerAssetSet asset_set)
{
    switch (asset_set) {
        case CONTROLLER_ASSET_SET_XBOX: {
            return g_xbox_controller_assets;
        } break;
        case CONTROLLER_ASSET_SET_PLAYSTATION: {
            return g_playstation_controller_assets;
        } break;
    }

    return nullptr;
}

static SDL_Texture** get_keyboard_asset_set(KeyboardAssetSet asset_set)
{
    switch (asset_set) {
        case KEYBOARD_ASSET_SET_ALPHANUMERIC: {
            return g_keyboard_alphanumeric_assets;
        } break;
        case KEYBOARD_ASSET_SET_TENKEYLESS: {
            return g_keyboard_tenkeyless_assets;
        } break;
        case KEYBOARD_ASSET_SET_FULLSIZE: {
            return g_keyboard_fullsize_assets;
        } break;
    }

    return nullptr;
}

extern Uint8 g_keystate[256];

extern int g_window_width;
extern int g_window_height;

static int g_part_id_to_button[64] = {};
static int g_part_id_to_key[256] = {}; // yikes!

// NOTE: input is obtained here directly,
// ideally input state is retrieved else where though as part of a refactor!

// grrr... this shouldn't exist! NOTE: some ids are covered by the specific trigger and joystick renderers.
static ControllerPuppetPointId xbox_controller_asset_id_to_puppet_point_id(XboxControllerAssetId asset_id)
{
    switch (asset_id) {
        case XBOXCONTROLLER_ASSET_BUTTON_A_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_A;
        case XBOXCONTROLLER_ASSET_BUTTON_B_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_B;
        case XBOXCONTROLLER_ASSET_BUTTON_X_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_X;
        case XBOXCONTROLLER_ASSET_BUTTON_Y_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_Y;
        case XBOXCONTROLLER_ASSET_DPAD_UP_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_DPAD_UP;
        case XBOXCONTROLLER_ASSET_DPAD_DOWN_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_DPAD_DOWN;
        case XBOXCONTROLLER_ASSET_DPAD_LEFT_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_DPAD_LEFT;
        case XBOXCONTROLLER_ASSET_DPAD_RIGHT_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_DPAD_RIGHT;
        case XBOXCONTROLLER_ASSET_BUTTON_MENU_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_MENU;
        case XBOXCONTROLLER_ASSET_BUTTON_START_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_START;
        case XBOXCONTROLLER_ASSET_RB_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_RB;
        case XBOXCONTROLLER_ASSET_LB_FILL:
            return CONTROLLER_PUPPET_POINT_BUTTON_LB;
    }
}

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

static void _query_asset(SDL_Texture** asset_set, int id, int* w, int* h)
{
    SDL_QueryTexture(asset_set[id], 0, 0, w, h);
}

static inline void draw_controller_part_joystick(SDL_Renderer* renderer, SDL_GameController* controller, const OverlaySettings& g_settings, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    auto point = point_set[part_id];

    int part_w; int part_h;
    _query_asset(asset_set, XBOXCONTROLLER_ASSET_JOYSTICK_FILL, &part_w, &part_h);

    SDL_Rect destination = {point.x / g_settings.image_scale_ratio - (part_w/g_settings.image_scale_ratio)/2, point.y / g_settings.image_scale_ratio - (part_h/g_settings.image_scale_ratio)/2, (part_w/g_settings.image_scale_ratio), (part_h/g_settings.image_scale_ratio)};

    bool  is_left = (part_id == CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT);
    short axis_x  = (controller) ? SDL_GameControllerGetAxis(controller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX) : 0;
    short axis_y  = (controller) ? SDL_GameControllerGetAxis(controller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY) : 0;

    destination.x += axis_x/32767.0f * (MAX_JOYSTICK_DISPLACEMENT_PX/g_settings.image_scale_ratio);
    destination.y += axis_y/32767.0f * (MAX_JOYSTICK_DISPLACEMENT_PX/g_settings.image_scale_ratio);

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

    bool is_left       = (part_id == CONTROLLER_PUPPET_POINT_LEFT_TRIGGER);
    int  fillmask_id   = (is_left) ? XBOXCONTROLLER_ASSET_LT_FILL : XBOXCONTROLLER_ASSET_RT_FILL;
    int  partsprite_id = (is_left) ? XBOXCONTROLLER_ASSET_LT : XBOXCONTROLLER_ASSET_RT;

    int part_w; int part_h;
    _query_asset(asset_set, fillmask_id, &part_w, &part_h);

    SDL_Rect destination = {point.x / g_settings.image_scale_ratio, point.y / g_settings.image_scale_ratio, part_w / g_settings.image_scale_ratio, part_h / g_settings.image_scale_ratio};

    short axis_y = (controller) ?
        SDL_GameControllerGetAxis(controller,
                                  (is_left) ? SDL_CONTROLLER_AXIS_TRIGGERLEFT : SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        : 0;

    destination.y += axis_y/32767.0f * (MAX_TRIGGER_DISPLACEMENT_PX/g_settings.image_scale_ratio);

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
    SDL_Texture** controller_asset_set = get_controller_asset_set(asset_set);
    SDL_Point*    puppeter_point_set   = get_controller_point_set(asset_set);

    set_controller_visual_focus(controller_asset_set, controller);
    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_LEFT_TRIGGER, controller_asset_set, puppeter_point_set);
    draw_controller_puppet_part(renderer, controller, g_settings, CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER, controller_asset_set, puppeter_point_set);

    // Draw_All_Controller_Parts.
    {
        {
            SDL_Rect destination = {0, 0, g_window_width, g_window_height};
            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], g_settings.controller_color.r, g_settings.controller_color.g, g_settings.controller_color.b);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], 0, &destination);

            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 255, 255, 255);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 0, &destination);
        }


        for (unsigned part_index = XBOXCONTROLLER_ASSET_BUTTON_A_FILL; part_index < XBOXCONTROLLER_ASSET_JOYSTICK; ++part_index) {
            int part_w; int part_h;
            _query_asset(controller_asset_set, part_index, &part_w, &part_h);
            auto point = puppeter_point_set[xbox_controller_asset_id_to_puppet_point_id((XboxControllerAssetId)part_index)];
            SDL_Rect destination = {point.x / g_settings.image_scale_ratio - (part_w/g_settings.image_scale_ratio)/2, point.y / g_settings.image_scale_ratio - (part_h/g_settings.image_scale_ratio)/2, (part_w/g_settings.image_scale_ratio), (part_h/g_settings.image_scale_ratio)};
            // SDL_Rect destination = { 0, 0, part_w/g_settings.image_scale_ratio, part_h/g_settings.image_scale_ratio };

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

void draw_keyboard(SDL_Renderer* renderer, const OverlaySettings& g_settings, Uint8* keystate, KeyboardAssetSet asset_set)
{
    SDL_Texture** keyboard_asset_set = get_keyboard_asset_set(asset_set);
    SDL_Point*    puppeter_point_set = g_keyboard_puppet_piece_placements;
    {
        {
            SDL_Rect destination = {0, 0, g_window_width, g_window_height};
            SDL_SetTextureColorMod(keyboard_asset_set[KEYBOARD_ASSET_BASE], 255, 255, 255);
            SDL_RenderCopy(renderer, keyboard_asset_set[KEYBOARD_ASSET_BASE], 0, &destination);
        }

        for (unsigned part_index = 0; part_index < KEYBOARD_PUPPET_POINT_COUNT; ++part_index) {
            unsigned part_asset_id = g_keyboard_puppet_piece_to_asset_id[part_index];
            int part_w; int part_h;

            _query_asset(keyboard_asset_set, part_asset_id, &part_w, &part_h);
            auto point = puppeter_point_set[part_index];
            SDL_Rect destination = {point.x / g_settings.image_scale_ratio - (part_w/g_settings.image_scale_ratio)/2, point.y / g_settings.image_scale_ratio - (part_h/g_settings.image_scale_ratio)/2, (part_w/g_settings.image_scale_ratio), (part_h/g_settings.image_scale_ratio)};

            if (keystate[part_index]) {
                SDL_SetTextureColorMod(keyboard_asset_set[part_asset_id], g_settings.activated_color.r, g_settings.activated_color.g, g_settings.activated_color.b);
            } else {
                SDL_SetTextureColorMod(keyboard_asset_set[part_asset_id], g_settings.button_color.r, g_settings.button_color.g, g_settings.button_color.b);
            }

            SDL_RenderCopy(renderer, keyboard_asset_set[part_asset_id], 0, &destination);
        }
    }
    assert(0 && "Not implemented.");
}

// Asset loading
static SDL_Texture* load_image_from_file(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surface = IMG_Load(path);
    SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(result, SDL_ScaleModeLinear);
    SDL_FreeSurface(surface);
    return result;
}

void load_xbox_controller_assets(void)
{
    for (unsigned index = 0; index < XBOXCONTROLLER_ASSET_COUNT; ++index) {
        g_xbox_controller_assets[index] =
            load_image_from_file(g_renderer, asset_image_list[CONTROLLER_ASSET_SET_XBOX][index]);
    }
}

void load_playstation_controller_assets(void)
{
    for (unsigned index = 0; index < PLAYSTATIONCONTROLLER_ASSET_COUNT; ++index) {
        g_playstation_controller_assets[index] =
            load_image_from_file(g_renderer, asset_image_list[CONTROLLER_ASSET_SET_PLAYSTATION][index]);
    }
}

void load_keyboard_alphanumeric_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        g_keyboard_alphanumeric_assets[index] =
            load_image_from_file(g_renderer, keyboard_asset_image_list[KEYBOARD_ASSET_SET_ALPHANUMERIC][index]);
    }
}

void load_keyboard_tenkeyless_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        g_keyboard_tenkeyless_assets[index] =
            load_image_from_file(g_renderer, keyboard_asset_image_list[KEYBOARD_ASSET_SET_TENKEYLESS][index]);
    }
}

void load_keyboard_fullsize_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        g_keyboard_fullsize_assets[index] =
            load_image_from_file(g_renderer, keyboard_asset_image_list[KEYBOARD_ASSET_SET_FULLSIZE][index]);
    }
}

void unload_xbox_controller_assets(void)
{
    for (unsigned index = 0; index < XBOXCONTROLLER_ASSET_COUNT; ++index) {
        if (g_xbox_controller_assets[index]) {
            SDL_DestroyTexture(g_xbox_controller_assets[index]);
            g_xbox_controller_assets[index] = nullptr;
        }
    }
}

void unload_playstation_controller_assets(void)
{
    for (unsigned index = 0; index < PLAYSTATIONCONTROLLER_ASSET_COUNT; ++index) {
        if (g_playstation_controller_assets[index]) {
            SDL_DestroyTexture(g_playstation_controller_assets[index]);
            g_playstation_controller_assets[index] = nullptr;
        }
    }
}

void unload_keyboard_alphanumeric_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        if (g_keyboard_alphanumeric_assets[index]) {
            SDL_DestroyTexture(g_keyboard_alphanumeric_assets[index]);
            g_keyboard_alphanumeric_assets[index] = nullptr;
        }
    }
}

void unload_keyboard_tenkeyless_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        if (g_keyboard_tenkeyless_assets[index]) {
            SDL_DestroyTexture(g_keyboard_tenkeyless_assets[index]);
            g_keyboard_tenkeyless_assets[index] = nullptr;
        }
    }
}

void unload_keyboard_fullsize_assets(void)
{
    for (unsigned index = 0; index < KEYBOARD_ASSET_COUNT; ++index) {
        if (g_keyboard_fullsize_assets[index]) {
            SDL_DestroyTexture(g_keyboard_fullsize_assets[index]);
            g_keyboard_fullsize_assets[index] = nullptr;
        }
    }
}

void unload_keyboard_key_assets(void)
{
    unload_keyboard_alphanumeric_assets();
    unload_keyboard_tenkeyless_assets();
    unload_keyboard_fullsize_assets();
}

void set_global_controller_asset_set(ControllerAssetSet controller_asset_set)
{
    g_using_keyboard = false;
    if (g_asset_set != controller_asset_set) {
        unload_controller_assets();

        g_asset_set = controller_asset_set;
        switch (controller_asset_set) {
            case CONTROLLER_ASSET_SET_XBOX: {
                load_xbox_controller_assets();
            } break;
            case CONTROLLER_ASSET_SET_PLAYSTATION: {
                load_playstation_controller_assets();
            } break;
        }
    }
}

void set_global_keyboard_asset_set(KeyboardAssetSet keyboard_asset_set)
{
    g_using_keyboard = true;
    if (g_keyboard_asset_set != keyboard_asset_set) {
        unload_keyboard_key_assets();

        g_keyboard_asset_set = keyboard_asset_set;

        // TODO;
        switch (keyboard_asset_set) {
            case KEYBOARD_ASSET_SET_ALPHANUMERIC: {
                load_keyboard_alphanumeric_assets();
            } break;
            case KEYBOARD_ASSET_SET_TENKEYLESS: {
                load_keyboard_tenkeyless_assets();
            } break;
            case KEYBOARD_ASSET_SET_FULLSIZE: {
                load_keyboard_fullsize_assets();
            } break;
        }
    }
}

void get_current_recommended_screen_size(const OverlaySettings& g_settings, int* window_width, int* window_height)
{
    int scale = g_settings.image_scale_ratio;
    SDL_Texture** asset_set;

    if (g_using_keyboard) {
        asset_set = get_keyboard_asset_set(g_keyboard_asset_set);
    } else {
        asset_set = get_controller_asset_set(g_asset_set);
    }

    // 0 is the base image index.
    SDL_QueryTexture(asset_set[0], 0, 0, window_width, window_height);
}
