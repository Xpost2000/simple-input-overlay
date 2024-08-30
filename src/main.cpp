/*
 * Simple InputOverlay
 *
 * A very simple handmade streaming overlay tool designed mainly to run on win32,
 * with other platform ports pending...
 *
 * Intended to be used with OBS.
*
 * Runs on SDL2 and uses some OS libraries to do some stuff.
 * NOTE: only on win32.
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

// important menu key ids.
static int controller_menu_option_start_index = -1;
static int controller_menu_option_end_index = -1;

static int keyboard_menu_option_start_index = -1;
static int keyboard_menu_option_end_index = -1;

#define DEVICE_COLOR_MENU_ID     1119
#define BUTTON_COLOR_MENU_ID     1120
#define ACTIVATION_COLOR_MENU_ID 1121
#define ZOOM_SCALE_0_MENU_ID     1122
#define ZOOM_SCALE_1_MENU_ID     1123
#define ZOOM_SCALE_2_MENU_ID     1124
#define ZOOM_SCALE_3_MENU_ID     1125
#define EXIT_QUIT_MENU_ID        1126
// End Win32 Menus

#include "controller_puppet_point_ids.h"

#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"

#include "keyboard_asset_id.h"

#include "keyboard_asset_set.h"
#include "controller_asset_set.h"

#include "input_layout_visual.h"

ControllerAssetSet g_asset_set          = CONTROLLER_ASSET_SET_UNKNOWN;
KeyboardAssetSet   g_keyboard_asset_set = KEYBOARD_ASSET_SET_UNKNOWN;
bool               g_using_keyboard     = false;

Uint8 g_keystate[256]; // for the keymap that will be read later, translated into private key codes.

static OverlaySettings g_settings;

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

static int vk_code_to_sdl_scancode(DWORD vk_code, bool extended_key)
{
    switch (vk_code) {
        case '0': { return SDL_SCANCODE_0; } break;
        case '1': { return SDL_SCANCODE_1; } break;
        case '2': { return SDL_SCANCODE_2; } break;
        case '3': { return SDL_SCANCODE_3; } break;
        case '4': { return SDL_SCANCODE_4; } break;
        case '5': { return SDL_SCANCODE_5; } break;
        case '6': { return SDL_SCANCODE_6; } break;
        case '7': { return SDL_SCANCODE_7; } break;
        case '8': { return SDL_SCANCODE_8; } break;
        case '9': { return SDL_SCANCODE_9; } break;

        case VK_OEM_MINUS: { return SDL_SCANCODE_MINUS; } break;
        case VK_OEM_PLUS: { return SDL_SCANCODE_EQUALS; } break;

        case 'A': { return SDL_SCANCODE_A; } break;
        case 'B': { return SDL_SCANCODE_B; } break;
        case 'C': { return SDL_SCANCODE_C; } break;
        case 'D': { return SDL_SCANCODE_D; } break;
        case 'E': { return SDL_SCANCODE_E; } break;
        case 'F': { return SDL_SCANCODE_F; } break;
        case 'G': { return SDL_SCANCODE_G; } break;
        case 'H': { return SDL_SCANCODE_H; } break;
        case 'I': { return SDL_SCANCODE_I; } break;
        case 'J': { return SDL_SCANCODE_J; } break;
        case 'K': { return SDL_SCANCODE_K; } break;
        case 'L': { return SDL_SCANCODE_L; } break;
        case 'M': { return SDL_SCANCODE_M; } break;
        case 'N': { return SDL_SCANCODE_N; } break;
        case 'O': { return SDL_SCANCODE_O; } break;
        case 'P': { return SDL_SCANCODE_P; } break;
        case 'Q': { return SDL_SCANCODE_Q; } break;
        case 'R': { return SDL_SCANCODE_R; } break;
        case 'S': { return SDL_SCANCODE_S; } break;
        case 'T': { return SDL_SCANCODE_T; } break;
        case 'U': { return SDL_SCANCODE_U; } break;
        case 'V': { return SDL_SCANCODE_V; } break;
        case 'W': { return SDL_SCANCODE_W; } break;
        case 'X': { return SDL_SCANCODE_X; } break;
        case 'Y': { return SDL_SCANCODE_Y; } break;
        case 'Z': { return SDL_SCANCODE_Z; } break;

        case VK_CAPITAL: { return SDL_SCANCODE_CAPSLOCK; } break;

        case VK_OEM_5: { return SDL_SCANCODE_BACKSLASH; } break;
        case VK_OEM_2: { return SDL_SCANCODE_SLASH; } break;
        case VK_OEM_3: { return SDL_SCANCODE_GRAVE; } break;

        case VK_OEM_1: { return SDL_SCANCODE_SEMICOLON; } break;
        case VK_OEM_4: { return SDL_SCANCODE_LEFTBRACKET; } break;
        case VK_OEM_6: { return SDL_SCANCODE_RIGHTBRACKET; } break;
        case VK_OEM_7: { return SDL_SCANCODE_APOSTROPHE; } break;

        case VK_OEM_COMMA: { return SDL_SCANCODE_COMMA; } break;
        case VK_OEM_PERIOD: { return SDL_SCANCODE_PERIOD; } break;

        case VK_BACK: { return SDL_SCANCODE_BACKSPACE; } break;
        case VK_SPACE: { return SDL_SCANCODE_SPACE; } break;

        case VK_LSHIFT: { return SDL_SCANCODE_LSHIFT; } break;
        case VK_RSHIFT: { return SDL_SCANCODE_RSHIFT; } break;

        case VK_F1: { return SDL_SCANCODE_F1; } break;
        case VK_F2: { return SDL_SCANCODE_F2; } break;
        case VK_F3: { return SDL_SCANCODE_F3; } break;
        case VK_F4: { return SDL_SCANCODE_F4; } break;
        case VK_F5: { return SDL_SCANCODE_F5; } break;
        case VK_F6: { return SDL_SCANCODE_F6; } break;
        case VK_F7: { return SDL_SCANCODE_F7; } break;
        case VK_F8: { return SDL_SCANCODE_F8; } break;
        case VK_F9: { return SDL_SCANCODE_F9; } break;
        case VK_F10: { return SDL_SCANCODE_F10; } break;
        case VK_F11: { return SDL_SCANCODE_F11; } break;
        case VK_F12: { return SDL_SCANCODE_F12; } break;

        case VK_LCONTROL: { return SDL_SCANCODE_LCTRL; } break;
        case VK_RCONTROL: { return SDL_SCANCODE_RCTRL; } break;

        case VK_SCROLL: { return SDL_SCANCODE_SCROLLLOCK; } break;

        case VK_LMENU: { return SDL_SCANCODE_LALT; } break;
        case VK_RMENU: { return SDL_SCANCODE_RALT; } break;

        case VK_LWIN: { return SDL_SCANCODE_LGUI; } break;
        case VK_RWIN: { return SDL_SCANCODE_RGUI; } break;

        case VK_APPS: { return SDL_SCANCODE_MENU; } break;

        case VK_PRINT: { return SDL_SCANCODE_PRINTSCREEN; } break;
        case VK_NUMLOCK: { return SDL_SCANCODE_NUMLOCKCLEAR; } break;
        case VK_PAUSE: { return SDL_SCANCODE_PAUSE; } break;

        case VK_INSERT: { return SDL_SCANCODE_INSERT; } break;
        case VK_DELETE: { return SDL_SCANCODE_DELETE; } break;
        case VK_HOME: { return SDL_SCANCODE_HOME; } break;
        case VK_END: { return SDL_SCANCODE_END; } break;
        case VK_PRIOR: { return SDL_SCANCODE_PAGEUP; } break;
        case VK_NEXT: { return SDL_SCANCODE_PAGEDOWN; } break;

        case VK_UP: { return SDL_SCANCODE_UP; } break;
        case VK_DOWN: { return SDL_SCANCODE_DOWN; } break;
        case VK_LEFT: { return SDL_SCANCODE_LEFT; } break;
        case VK_RIGHT: { return SDL_SCANCODE_RIGHT; } break;

        case VK_NUMPAD0: { return SDL_SCANCODE_KP_0; } break;
        case VK_NUMPAD1: { return SDL_SCANCODE_KP_1; } break;
        case VK_NUMPAD2: { return SDL_SCANCODE_KP_2; } break;
        case VK_NUMPAD3: { return SDL_SCANCODE_KP_3; } break;
        case VK_NUMPAD4: { return SDL_SCANCODE_KP_4; } break;
        case VK_NUMPAD5: { return SDL_SCANCODE_KP_5; } break;
        case VK_NUMPAD6: { return SDL_SCANCODE_KP_6; } break;
        case VK_NUMPAD7: { return SDL_SCANCODE_KP_7; } break;
        case VK_NUMPAD8: { return SDL_SCANCODE_KP_8; } break;
        case VK_NUMPAD9: { return SDL_SCANCODE_KP_9; } break;

        case VK_TAB: { return SDL_SCANCODE_TAB; } break;

        case VK_DECIMAL: { return SDL_SCANCODE_KP_PERIOD; } break;
        case VK_SUBTRACT: { return SDL_SCANCODE_KP_MINUS; } break;
        case VK_MULTIPLY: { return SDL_SCANCODE_KP_MULTIPLY; } break;
        case VK_ADD: { return SDL_SCANCODE_KP_PLUS; } break;

            // Some keys are not here because they're not part of the preset.
        case VK_RETURN: {
            if (extended_key) {
                return SDL_SCANCODE_RETURN2;
            } else {
                return SDL_SCANCODE_RETURN;   
            }
        } break;
        
    }

    return -1;
}

static LRESULT keyboard_input_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT* hook_data = (KBDLLHOOKSTRUCT*)lParam;
    DWORD vk_code = hook_data->vkCode;
    DWORD scan_code = hook_data->scanCode;

    int translated_code = vk_code_to_sdl_scancode(vk_code, (hook_data->flags & LLKHF_EXTENDED) > 0);

    switch (wParam) {
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN: {
            if (translated_code != -1) {
                g_keystate[translated_code] = true;
            }
        } break;
        case WM_SYSKEYUP:
        case WM_KEYUP: {
            if (translated_code != -1) {
                g_keystate[translated_code] = false;
            }
        } break;
    }

    return CallNextHookEx(0, nCode, wParam, lParam);
}

static void resize_window_correctly(void);
static int application_main(int argc, char** argv)
{
    load_config(g_settings);
    write_config(g_settings);
    initialize_context_menu();

    DragEventData drag_data = {};

    // Install low level keyboard hook
    memset(g_keystate, 256, 0);
    SetWindowsHookExA(WH_KEYBOARD_LL, keyboard_input_hook, NULL, 0);

    set_global_controller_asset_set(CONTROLLER_ASSET_SET_XBOX);
    resize_window_correctly();

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
                            // Color selectors.
                            case DEVICE_COLOR_MENU_ID: {
                                WIN32_color_selector(g_window, &g_settings.controller_color);
                            } break;
                            case BUTTON_COLOR_MENU_ID: {
                                WIN32_color_selector(g_window, &g_settings.button_color);
                            } break;
                            case ACTIVATION_COLOR_MENU_ID: {
                                WIN32_color_selector(g_window, &g_settings.activated_color);
                            } break;

                            case ZOOM_SCALE_0_MENU_ID: {
                                g_settings.image_scale_ratio = 1;
                                resize_window_correctly();
                            } break;
                            case ZOOM_SCALE_1_MENU_ID: {
                                g_settings.image_scale_ratio = 2;
                                resize_window_correctly();
                            } break;
                            case ZOOM_SCALE_2_MENU_ID: {
                                g_settings.image_scale_ratio = 3;
                                resize_window_correctly();
                            } break;
                            case ZOOM_SCALE_3_MENU_ID: {
                                g_settings.image_scale_ratio = 4;
                                resize_window_correctly();
                            } break;
                            case EXIT_QUIT_MENU_ID: {
                                SDL_Event ev = { .type = SDL_QUIT };
                                SDL_PushEvent(&ev);
                            } break;

                            default: {
                                if (selected_option >= controller_menu_option_start_index && selected_option < controller_menu_option_end_index) {
                                    ControllerAssetSet asset_id = (ControllerAssetSet)(selected_option - controller_menu_option_start_index);
                                    set_global_controller_asset_set(asset_id);
                                    resize_window_correctly();
                                } else if (selected_option >= keyboard_menu_option_start_index && selected_option < keyboard_menu_option_end_index) {
                                    KeyboardAssetSet asset_id = (KeyboardAssetSet)(selected_option - keyboard_menu_option_start_index);
                                    set_global_keyboard_asset_set(asset_id);
                                    resize_window_correctly();
                                }
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

        if (g_using_keyboard) {
            draw_keyboard(g_renderer, g_settings, g_keystate, g_keyboard_asset_set);
        } else {
            draw_controller(g_renderer, g_focused_gamecontroller, g_settings, g_asset_set);
        }

        SDL_RenderPresent(g_renderer);
    }

    write_config(g_settings);
    return 0;
}

static void resize_window_correctly(void)
{
    // check asset type to determine resolution
    get_current_recommended_screen_size(g_settings, &g_window_width, &g_window_height);
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

        SDL_SetWindowAlwaysOnTop(g_window, SDL_TRUE);
        make_window_transparent(g_window);

        status_code = application_main(argc, argv);
    }
    IMG_Quit();
    SDL_Quit();
    return status_code; 
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
    {
        controller_menu_option_start_index = id;
        for (unsigned index = 0; index < CONTROLLER_ASSET_SET_COUNT; ++index) {
            insert_menu_text_item(g_context_menu, asset_set_strings[index], id++);
        }
        controller_menu_option_end_index = id;
    }
    {
        keyboard_menu_option_start_index = id;
        for (unsigned index = 0; index < KEYBOARD_ASSET_SET_COUNT; ++index) {
            insert_menu_text_item(g_context_menu, keyboard_asset_set_strings[index], id++);
        }
        keyboard_menu_option_end_index = id;
    }

    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Customization", id++, false);
    insert_menu_divider_item(g_context_menu, id++);
    insert_menu_text_item(g_context_menu, "Device Color",     DEVICE_COLOR_MENU_ID);
    insert_menu_text_item(g_context_menu, "Button/Key Color", BUTTON_COLOR_MENU_ID);
    insert_menu_text_item(g_context_menu, "Activation Color", ACTIVATION_COLOR_MENU_ID);
    insert_menu_text_item(g_context_menu, "Zoom Scale: 100%", ZOOM_SCALE_0_MENU_ID);
    insert_menu_text_item(g_context_menu, "Zoom Scale: 50%",  ZOOM_SCALE_1_MENU_ID);
    insert_menu_text_item(g_context_menu, "Zoom Scale: 33%",  ZOOM_SCALE_2_MENU_ID);
    insert_menu_text_item(g_context_menu, "Zoom Scale: 25%",  ZOOM_SCALE_3_MENU_ID);

    insert_menu_divider_item(g_context_menu, id++); // 16
    insert_menu_text_item(g_context_menu, "Etc.", id++, false); // 17
    insert_menu_text_item(g_context_menu, "Exit / Quit", EXIT_QUIT_MENU_ID); // 18
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
