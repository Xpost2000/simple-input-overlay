/*
 * Simple InputOverlay
 *
 * TODO: Keyboard view
 * TODO: X11 Port.
 * TODO: refactor?
 *
 * A handmade stream overlay tool I guess.
 *
 * Small stuff, this uses SDL2 to abstract across controller types
 * and also to be crossplatform.
 *
 * NOTE: Unfortunately this is not possible to do normally, so I might've
 * actually just been better off doing this raw!
 *
 * But SDL2 as a controller operator is still useful.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>

#include <assert.h>
#include "config.h"

// NOTE: need to work on this later.
// to change the size more.
#define IMAGE_SCALE_RATIO (2)
#define XBOX_IMAGE_WIDTH  (1452)
#define XBOX_IMAGE_HEIGHT (940)
#define XBOX_JOYSTICK_SZ  (169)

#define MAX_JOYSTICK_DISPLACEMENT_PX (80)
#define MAX_TRIGGER_DISPLACEMENT_PX (25)

#define SCALED_MAX_JOYSTICK_DISPLACEMENT_PX (MAX_JOYSTICK_DISPLACEMENT_PX / IMAGE_SCALE_RATIO)
#define SCALED_MAX_TRIGGER_DISPLACEMENT_PX  (MAX_TRIGGER_DISPLACEMENT_PX / IMAGE_SCALE_RATIO)

#define SCALED_JOYSTICK_SZ                  (XBOX_JOYSTICK_SZ / IMAGE_SCALE_RATIO)
#define SCALED_WINDOW_WIDTH                 (XBOX_IMAGE_WIDTH / IMAGE_SCALE_RATIO)
#define SCALED_WINDOW_HEIGHT                (XBOX_IMAGE_HEIGHT / IMAGE_SCALE_RATIO)

// This color is unlikely to ever be naturally selected...
#define CHROMA_KEY_COLOR  (RGB(255, 255, 254))

// Obviously this is a quick toy program so I'm not really trying write something
// with quality from scratch for real, so we're just going to use the existing SDL2 renderer
// which is more than enough for this.

// Alternate control schemes should also be supported in the future, but generally controllers
// and keyboards are the main sources of controlling that I would like to be dealing with.

// Otherwise the program would be like 1k+ lines.
bool                g_quit                   = false;
bool                g_borderless             = false;
SDL_Window*         g_window                 = nullptr;
SDL_Renderer*       g_renderer               = nullptr;
SDL_GameController* g_focused_gamecontroller = nullptr;

// Win32 Menus
HMENU g_context_menu;
// End Win32 Menus

#include "controller_puppet_point_ids.h"
#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"
#include "controller_asset_set.h"

static ControllerAssetSet g_asset_set = CONTROLLER_ASSET_SET_XBOX;

static SDL_Texture* g_xbox_controller_assets[20];
static SDL_Texture* g_playstation_controller_assets[20];

// NOTE: centered coordinates
static SDL_Point g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {373, 416},
    {912, 618},
    {0, 0},
    {0, 0},
};

static SDL_Point g_playstation_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {519, 634},
    {936, 634},
    {0, 0},
    {0, 0},
};

static OverlaySettings g_settings;

static SDL_Texture* load_image_from_file(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surface = IMG_Load(path);
    SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(result, SDL_ScaleModeLinear);
    SDL_FreeSurface(surface);
    return result;
}

HWND SDL_get_hwnd(SDL_Window* window)
{
    SDL_SysWMinfo wm_info = {};
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(window, &wm_info);
    return wm_info.info.win.window;
}

// This requires a platform native solution.
static void make_window_transparent(SDL_Window* window)
{
#ifdef _WIN32
    HWND windows_handle = SDL_get_hwnd(window);

    assert(SUCCEEDED(
               SetWindowLong(windows_handle,
                             GWL_EXSTYLE,
                             GetWindowLong(windows_handle, GWL_EXSTYLE) | WS_EX_LAYERED)
           ));

    assert(SUCCEEDED(
               SetLayeredWindowAttributes(windows_handle, CHROMA_KEY_COLOR, 0, LWA_COLORKEY)
           ));
#endif
}

static void load_xbox_controller_assets(void);
static void load_playstation_controller_assets(void);
static void load_keyboard_key_assets(void);
static void load_controller_assets(void)
{
    load_xbox_controller_assets();
    load_playstation_controller_assets();
#if 0
    load_keyboard_key_assets();
#endif
}

SDL_Color lerp_color(SDL_Color a, SDL_Color b, float t) {
    SDL_Color result;
    result.r = (a.r/255.0f * (1.0 - t) + t * b.r/255.0f) * 255;
    result.g = (a.g/255.0f * (1.0 - t) + t * b.g/255.0f) * 255;
    result.b = (a.b/255.0f * (1.0 - t) + t * b.b/255.0f) * 255;
    result.a = (a.a/255.0f * (1.0 - t) + t * b.a/255.0f) * 255;
    return result;
}

static int g_part_id_to_button[64] = {};

static inline void draw_controller_part_joystick(SDL_Renderer* renderer, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    auto point = point_set[part_id];
    SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, point.y / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, SCALED_JOYSTICK_SZ, SCALED_JOYSTICK_SZ};

    bool  is_left = (part_id == CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT);
    short axis_x  = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTX : SDL_CONTROLLER_AXIS_RIGHTX) : 0;
    short axis_y  = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, (is_left) ? SDL_CONTROLLER_AXIS_LEFTY : SDL_CONTROLLER_AXIS_RIGHTY) : 0;

    destination.x += axis_x/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;
    destination.y += axis_y/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;

    if (SDL_GameControllerGetButton(g_focused_gamecontroller, (is_left) ? SDL_CONTROLLER_BUTTON_LEFTSTICK : SDL_CONTROLLER_BUTTON_RIGHTSTICK)) {
        SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], g_settings.activated_color.r, g_settings.activated_color.g, g_settings.activated_color.b);
    } else {
        SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], g_settings.button_color.r, g_settings.button_color.g, g_settings.button_color.b);
    }

    SDL_RenderCopy(renderer, asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], 0, &destination);

    SDL_SetTextureColorMod(asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 255, 255, 255);
    SDL_RenderCopy(renderer, asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 0, &destination);
}

static inline void draw_controller_part_trigger(SDL_Renderer* renderer, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    auto point = point_set[part_id];
    SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO, point.y / IMAGE_SCALE_RATIO, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT};

    bool is_left       = (part_id == CONTROLLER_PUPPET_POINT_LEFT_TRIGGER);
    int  fillmask_id   = (is_left) ? XBOXCONTROLLER_ASSET_LT_FILL : XBOXCONTROLLER_ASSET_RT_FILL;
    int  partsprite_id = (is_left) ? XBOXCONTROLLER_ASSET_LT : XBOXCONTROLLER_ASSET_RT;

    short axis_y = (g_focused_gamecontroller) ?
        SDL_GameControllerGetAxis(g_focused_gamecontroller,
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

static void draw_controller_puppet_part(SDL_Renderer* renderer, int part_id, SDL_Texture** asset_set, SDL_Point* point_set)
{
    switch (part_id) {
        case CONTROLLER_PUPPET_POINT_LEFT_TRIGGER:
        case CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER: {
            draw_controller_part_trigger(renderer, part_id, asset_set, point_set);
        } break;

        case CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT:
        case CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT: {
            draw_controller_part_joystick(renderer, part_id, asset_set, point_set);
        } break;
    }
}

static void set_controller_visual_focus(SDL_Texture** asset_set)
{
    for (unsigned part_index = 0; part_index < XBOXCONTROLLER_ASSET_COUNT; ++part_index) {
        if (g_focused_gamecontroller) {
            SDL_SetTextureAlphaMod(asset_set[part_index], 255);
        } else {
            SDL_SetTextureAlphaMod(asset_set[part_index], 64);
        }
    }
}

static void draw_controller(SDL_Renderer* renderer)
{
    SDL_Texture** controller_asset_set = g_xbox_controller_assets;
    SDL_Point*    puppeter_point_set   = g_xbox_controller_puppet_piece_placements;

    switch (g_asset_set) {
        case CONTROLLER_ASSET_SET_XBOX: {
            controller_asset_set = g_xbox_controller_assets;
            puppeter_point_set = g_xbox_controller_puppet_piece_placements;
        } break;
        case CONTROLLER_ASSET_SET_PLAYSTATION: {
            controller_asset_set = g_playstation_controller_assets;
            puppeter_point_set = g_playstation_controller_puppet_piece_placements;
        } break;
    }

    set_controller_visual_focus(controller_asset_set);
    draw_controller_puppet_part(renderer, CONTROLLER_PUPPET_POINT_LEFT_TRIGGER, controller_asset_set, puppeter_point_set);
    draw_controller_puppet_part(renderer, CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER, controller_asset_set, puppeter_point_set);

    // NOTE: enums are Xbox to be more obvious to read, but they are aligned for all controller types.
    // Draw_All_Controller_Parts.
    {
        SDL_Rect destination = { 0, 0, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT };
        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], g_settings.controller_color.r, g_settings.controller_color.g, g_settings.controller_color.b);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], 0, &destination);

        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 255, 255, 255);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 0, &destination);

        for (unsigned part_index = XBOXCONTROLLER_ASSET_BUTTON_A_FILL; part_index < XBOXCONTROLLER_ASSET_JOYSTICK; ++part_index) {
            if (g_focused_gamecontroller && SDL_GameControllerGetButton(g_focused_gamecontroller, (SDL_GameControllerButton)g_part_id_to_button[part_index])) {
                SDL_SetTextureColorMod(controller_asset_set[part_index], g_settings.activated_color.r, g_settings.activated_color.g, g_settings.activated_color.b);
            } else {
                SDL_SetTextureColorMod(controller_asset_set[part_index], g_settings.button_color.r, g_settings.button_color.g, g_settings.button_color.b);
            }

            SDL_RenderCopy(renderer, controller_asset_set[part_index], 0, &destination);
        }
    }

    draw_controller_puppet_part(renderer, CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT, controller_asset_set, puppeter_point_set);
    draw_controller_puppet_part(renderer, CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT, controller_asset_set, puppeter_point_set);
}

static void initialize_context_menu(void);
static int  do_context_menu(int x, int y);

static void assign_initial_controller_asset_set(SDL_GameController* game_controller)
{
    auto controller_type = SDL_GameControllerGetType(game_controller); 
    switch (controller_type) {
        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE: {
            g_asset_set = CONTROLLER_ASSET_SET_XBOX;
        } break;

        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5: {
            g_asset_set = CONTROLLER_ASSET_SET_PLAYSTATION;
        } break;

        default:
        case SDL_CONTROLLER_TYPE_UNKNOWN:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
        case SDL_CONTROLLER_TYPE_VIRTUAL:
        case SDL_CONTROLLER_TYPE_AMAZON_LUNA:
        case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:
        case SDL_CONTROLLER_TYPE_NVIDIA_SHIELD:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
        case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR: {
            g_asset_set = CONTROLLER_ASSET_SET_XBOX;
        } break;
    }
}

// WIN32...
#ifdef _WIN32
static void WIN32_color_selector(SDL_Window* window, SDL_Color* color)
{
    CHOOSECOLORA color_selector = {};
    static COLORREF custom_color_set[16];
    color_selector.lStructSize = sizeof(color_selector);
    color_selector.hwndOwner = SDL_get_hwnd(window);
    color_selector.rgbResult = RGB(color->r, color->g, color->b);
    color_selector.lpCustColors = (LPDWORD) custom_color_set;
    color_selector.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&color_selector) == TRUE) {
        color->r = (color_selector.rgbResult & 0x000000FF) >> 0;
        color->g = (color_selector.rgbResult & 0x0000FF00) >> 8;
        color->b = (color_selector.rgbResult & 0x00FF0000) >> 16;
    }

    write_config(g_settings);
}
#endif

static int application_main(int argc, char** argv)
{
    load_controller_assets();
    load_config(g_settings);
    write_config(g_settings);
    initialize_context_menu();

    // setup keymap.
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

    while (!g_quit) {
        SDL_Event event;

        // These events aren't really "needed per say..."
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT: {
                    g_quit = true;
                } break;
                case SDL_MOUSEBUTTONDOWN: {
                    if (event.button.button == SDL_BUTTON_RIGHT) {
                        int selected_option = 0;
                        switch (selected_option = do_context_menu(event.button.x, event.button.y)) {
                            case 4: { // Xbox
                                g_asset_set = CONTROLLER_ASSET_SET_XBOX;
                            } break;
                            case 5: { // Playstation
                                g_asset_set = CONTROLLER_ASSET_SET_PLAYSTATION;
                            } break;

                            // Color selectors.
                            case 9: {
                                WIN32_color_selector(g_window, &g_settings.controller_color);
                            } break;
                            case 10: {
                                WIN32_color_selector(g_window, &g_settings.button_color);
                            } break;
                            case 11: {
                                WIN32_color_selector(g_window, &g_settings.activated_color);
                            } break;
                            case 12: { // borderless toggle
                                if (!g_borderless) {
                                    SDL_SetWindowBordered(g_window, SDL_FALSE);
                                    g_borderless = true;
                                } else {
                                    SDL_SetWindowBordered(g_window, SDL_TRUE);
                                    g_borderless = false;
                                }
                            } break;
                        }
                    }
                } break;
                case SDL_KEYUP:
                case SDL_KEYDOWN: {} break;
                case SDL_CONTROLLERDEVICEADDED:
                case SDL_CONTROLLERDEVICEREMOVED: {
                    auto& event_data = event.cdevice;

                    if (event_data.type == SDL_CONTROLLERDEVICEADDED) {
                        if (g_focused_gamecontroller == nullptr) {
                            g_focused_gamecontroller = SDL_GameControllerOpen(event_data.which);
                            char tmp[255];
                            snprintf(tmp, 255, "Input Overlay : Controller - %s (%d)",
                                     SDL_GameControllerName(g_focused_gamecontroller), event_data.which);
                            SDL_SetWindowTitle(g_window, tmp);
                            assign_initial_controller_asset_set(g_focused_gamecontroller);
                        }
                    } else {
                        SDL_GameControllerClose(g_focused_gamecontroller);
                        SDL_SetWindowTitle(g_window, "Input Overlay : No Controller");
                        g_focused_gamecontroller = nullptr;
                    }
                } break;
                case SDL_CONTROLLERBUTTONUP:
                case SDL_CONTROLLERBUTTONDOWN: {} break;
                case SDL_CONTROLLERAXISMOTION: {} break;
            }
        }

        SDL_SetRenderDrawColor(g_renderer, 255, 255, 254, 200);
        SDL_RenderClear(g_renderer);

        // NOTE: drawing a keyboard is different!
        SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
        draw_controller(g_renderer);

        SDL_RenderPresent(g_renderer);
    }

    write_config(g_settings);
    return 0;
}

int main(int argc, char** argv)
{
    int status_code = 0;
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);

    // The keyboard version is a bit hard cause it's basically a keylogger.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    {
        g_window = SDL_CreateWindow(
            "InputOverlay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 10, 10,
            SDL_WINDOW_SHOWN);  // Resizable needs to check for multiple size.

        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);

        SDL_SetWindowSize(g_window, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT);
        SDL_SetWindowAlwaysOnTop(g_window, SDL_TRUE);
        make_window_transparent(g_window);

        status_code = application_main(argc, argv);
    }
    IMG_Quit();
    SDL_Quit();
    return status_code; 
}

// This would be an okay case to do an inherited class or something
// or I would totally be okay with just turning this into an enum class situation again.
static void load_xbox_controller_assets(void)
{
    for (unsigned index = 0; index < XBOXCONTROLLER_ASSET_COUNT; ++index) {
        g_xbox_controller_assets[index] =
            load_image_from_file(g_renderer, asset_image_list[CONTROLLER_ASSET_SET_XBOX][index]);
    }
}

static void load_playstation_controller_assets(void)
{
    for (unsigned index = 0; index < PLAYSTATIONCONTROLLER_ASSET_COUNT; ++index) {
        g_playstation_controller_assets[index] =
            load_image_from_file(g_renderer, asset_image_list[CONTROLLER_ASSET_SET_PLAYSTATION][index]);
    }
}

static void load_keyboard_key_assets(void)
{
    assert(0 && "Not done.");
}

// This is mostly win32.
static void insert_menu_text_item(HMENU menu_parent, const char* text, int id, bool selectable=true)
{
    MENUITEMINFOA menu_item_info = {};
    {
        menu_item_info.cbSize = sizeof(menu_item_info);
        menu_item_info.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
        menu_item_info.fType = MFT_STRING;

        if (!selectable) {
            menu_item_info.fState = MFS_DISABLED;   
        }

        menu_item_info.wID    = id;
        menu_item_info.hSubMenu = menu_parent;
        {
            const char* str = text;
            menu_item_info.dwTypeData = (LPSTR)str;
            menu_item_info.cch        = strlen(str);
        }
    }
    InsertMenuItemA(menu_parent, id, false, &menu_item_info);
}

static void insert_menu_divider_item(HMENU menu_parent, int id)
{
    MENUITEMINFOA menu_item_info = {};
    {
        menu_item_info.cbSize = sizeof(menu_item_info);
        menu_item_info.fMask = MIIM_TYPE | MIIM_ID | MIIM_STATE;
        menu_item_info.fType = MFT_MENUBREAK;
        menu_item_info.wID    = id;
        menu_item_info.hSubMenu = menu_parent;
    }
    InsertMenuItemA(menu_parent, id, false, &menu_item_info);
}

static void initialize_context_menu(void)
{
    g_context_menu = CreatePopupMenu();
    int id = 1;
    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Input Layout Presets", id++, false);
    insert_menu_divider_item(g_context_menu, id++); // 3 (controller types start at 4)
    insert_menu_text_item(g_context_menu, "Preset: Xbox Controller", id++);
    insert_menu_text_item(g_context_menu, "Preset: Dualsense Controller", id++);

    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Customization", id++, false);
    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Controller Color", id++); // 9
    insert_menu_text_item(g_context_menu, "Button Color", id++);     // 10
    insert_menu_text_item(g_context_menu, "Activation Color", id++); // 11
    insert_menu_text_item(g_context_menu, "Borderless", id++);       // 12

    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Etc.", id++, false);
    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Version 0", id++, false);
    insert_menu_text_item(g_context_menu, "Author: xpost2000", id++, false);
}

static int do_context_menu(int x, int y)
{
    POINT client_point = {x, y};
    HWND window = SDL_get_hwnd(g_window);

    ClientToScreen(window, &client_point);
    return TrackPopupMenu(g_context_menu,
                          TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_LEFTALIGN | TPM_BOTTOMALIGN,
                          client_point.x, client_point.y, 0, window, 0);
}
