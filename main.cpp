/*
 * Simple InputOverlay
 *
 * A very simple handmade streaming overlay tool designed mainly to run on win32,
 * with other platform ports pending...
 *
 * Intended to be used with OBS.
 *
 * NOTE: due to the method used to composite the super image, it's a bit expensive admittedly!
 *       should be optimized to avoid so many fillmask parts! (I should be specifying part locations.)
 *
 * Runs on SDL2 and uses some OS libraries to do some stuff.
 *
 * TODO: Keyboard view
 * TODO: X11 Port.
 */
#include "constants.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>

#include <assert.h>
#include "config.h"

bool                g_quit                   = false;
SDL_Window*         g_window                 = nullptr;
SDL_Renderer*       g_renderer               = nullptr;
SDL_GameController* g_focused_gamecontroller = nullptr;

int g_window_width = SCALED_WINDOW_WIDTH;
int g_window_height = SCALED_WINDOW_HEIGHT;

// Win32 Menus
HMENU g_context_menu;
// End Win32 Menus

#include "controller_puppet_point_ids.h"
#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"
#include "controller_asset_set.h"

#include "input_layout_visual.h"

// Flexible theming would be nice, but isn't really simple,
// so I can only support what I officially provide here.
//
// It's not super hard though
static ControllerAssetSet g_asset_set = CONTROLLER_ASSET_SET_UNKNOWN;
static void set_global_controller_asset_set(ControllerAssetSet controller_asset_set);

SDL_Texture* g_xbox_controller_assets[20] = {};
SDL_Texture* g_playstation_controller_assets[20] = {};

Uint8 g_keystate[256]; // for the keymap that will be read later, translated into private key codes.

// NOTE: centered coordinates
SDL_Point g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {373, 416},
    {912, 618},
    {0, 0},
    {0, 0},
};

SDL_Point g_playstation_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
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

void unload_controller_assets(void)
{
    unload_xbox_controller_assets();
    unload_playstation_controller_assets();
}

void load_controller_assets(void)
{
    load_xbox_controller_assets();
    load_playstation_controller_assets();
}

static void initialize_context_menu(void);
static int  do_context_menu(int x, int y);

static void assign_initial_controller_asset_set(SDL_GameController* game_controller)
{
    auto controller_type = SDL_GameControllerGetType(game_controller); 
    switch (controller_type) {
        case SDL_CONTROLLER_TYPE_XBOX360:
        case SDL_CONTROLLER_TYPE_XBOXONE: {
            set_global_controller_asset_set(CONTROLLER_ASSET_SET_XBOX);
        } break;

        case SDL_CONTROLLER_TYPE_PS3:
        case SDL_CONTROLLER_TYPE_PS4:
        case SDL_CONTROLLER_TYPE_PS5: {
            set_global_controller_asset_set(CONTROLLER_ASSET_SET_PLAYSTATION);
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
            set_global_controller_asset_set(CONTROLLER_ASSET_SET_XBOX);
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

struct DragEventData {
    bool dragging;
    int window_top_left_x;
    int window_top_left_y;

    int offset_x;
    int offset_y;

    DragEventData() : dragging(false)
    {
        
    }

    void start_dragging(SDL_Window* window)
    {
        if (dragging == false) {
            dragging = true;

            // top left
            SDL_GetWindowPosition(window, &window_top_left_x, &window_top_left_y);

            POINT screen_cursor;
            GetCursorPos(&screen_cursor);
            offset_x = window_top_left_x - screen_cursor.x;
            offset_y = window_top_left_y - screen_cursor.y;
        }
    }

    bool get_new_final_position(int* pos)
    {
        if (dragging)
        {
            POINT screen_cursor;
            GetCursorPos(&screen_cursor);
            *pos     = offset_x + screen_cursor.x;
            *(pos+1) = offset_y + screen_cursor.y;

            return true;
        }

        return false;
    }

    void stop_dragging(void)
    {
        dragging = false;
    }
};

#if 0
static LRESULT keyboard_input_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* hook_data = (KBDLLHOOKSTRUCT*)lParam;

    switch (wParam) {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
        } break;
        case WM_SYSKEYUP:
        case WM_KEYUP: {
        } break;
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}
#endif

static void resize_window_correctly(void);
static int application_main(int argc, char** argv)
{
    load_config(g_settings);
    write_config(g_settings);
    initialize_context_menu();
    init_controller_keymap();

    DragEventData drag_data = {};

#if 0
    // Install low level keyboard hook
    memset(g_keystate, 256, 0);
    SetWindowsHookExA(WH_KEYBOARD_LL, keyboard_input_hook, NULL, 0);
#endif

    set_global_controller_asset_set(CONTROLLER_ASSET_SET_XBOX);
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
                                set_global_controller_asset_set(CONTROLLER_ASSET_SET_XBOX);
                            } break;
                            case 5: { // Playstation
                                set_global_controller_asset_set(CONTROLLER_ASSET_SET_PLAYSTATION);
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

                            case 12: {
                                g_settings.image_scale_ratio = 1;
                                resize_window_correctly();
                            } break;
                            case 13: {
                                g_settings.image_scale_ratio = 2;
                                resize_window_correctly();
                            } break;
                            case 14: {
                                g_settings.image_scale_ratio = 3;
                                resize_window_correctly();
                            } break;
                            case 15: {
                                g_settings.image_scale_ratio = 4;
                                resize_window_correctly();
                            } break;

                            case 18: {
                                SDL_Event ev = { .type = SDL_QUIT };
                                SDL_PushEvent(&ev);
                            } break;
                        }
                    } else if (event.button.button == SDL_BUTTON_LEFT) {
                        drag_data.start_dragging(g_window);
                    }
                } break;
                case SDL_MOUSEBUTTONUP: {
                    if (event.button.button == SDL_BUTTON_LEFT)  {
                        drag_data.stop_dragging();
                    }
                } break;
                case SDL_MOUSEMOTION: {
                    int position[2];
                    if (drag_data.get_new_final_position(position)) {
                        SDL_SetWindowPosition(g_window, position[0], position[1]);
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
        draw_controller(g_renderer, g_focused_gamecontroller, g_settings, g_asset_set);

        SDL_RenderPresent(g_renderer);
    }

    write_config(g_settings);
    return 0;
}

static void resize_window_correctly(void)
{
    // check asset type to determine resolution
    g_window_width = XBOX_IMAGE_WIDTH / g_settings.image_scale_ratio;
    g_window_height = XBOX_IMAGE_HEIGHT / g_settings.image_scale_ratio;
    SDL_SetWindowSize(g_window, g_window_width, g_window_height);
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
            "InputOverlay", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0,
            SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS);  // Resizable needs to check for multiple size.

        g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_ACCELERATED);
        // g_renderer = SDL_CreateRenderer(g_window, -1, SDL_RENDERER_SOFTWARE);

        resize_window_correctly();
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

void load_keyboard_key_assets(void)
{
    assert(0 && "Not done.");
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

void unload_keyboard_key_assets(void)
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
    insert_menu_text_item(g_context_menu, "Zoom Scale: 100%", id++); // 12
    insert_menu_text_item(g_context_menu, "Zoom Scale: 50%", id++); // 13
    insert_menu_text_item(g_context_menu, "Zoom Scale: 33%", id++); // 14
    insert_menu_text_item(g_context_menu, "Zoom Scale: 25%", id++); // 15

    insert_menu_divider_item(g_context_menu, id++); // 16
    insert_menu_text_item(g_context_menu, "Etc.", id++, false); // 17
    insert_menu_text_item(g_context_menu, "Exit / Quit", id++); // 18
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

// makes sure we only load the specific assets we want to load.
static void set_global_controller_asset_set(ControllerAssetSet controller_asset_set)
{
    if (g_asset_set != controller_asset_set) {
        // unload_keyboard_key_assets();
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
