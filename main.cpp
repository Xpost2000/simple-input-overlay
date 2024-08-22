/*
 * InputOverlay
 *
 * TODO: PSX controller view
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
 *
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_image.h>

#include <assert.h>

// Obviously this is a quick toy program so I'm not really trying write something
// with quality from scratch for real, so we're just going to use the existing SDL2 renderer
// which is more than enough for this.

// Alternate control schemes should also be supported in the future, but generally controllers
// and keyboards are the main sources of controlling that I would like to be dealing with.

// Otherwise the program would be like 1k+ lines.
bool                g_quit                   = false;
SDL_Window*         g_window                 = nullptr;
SDL_Renderer*       g_renderer               = nullptr;
SDL_GameController* g_focused_gamecontroller = nullptr;

#include "controller_puppet_point_ids.h"
#include "playstation_controller_asset_id.h"
#include "xbox_controller_asset_id.h"

SDL_Texture* g_xbox_controller_assets[20];
SDL_Texture* g_playstation_controller_assets[20];

// NOTE: centered coordinates
SDL_Point g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
    {373, 416},
    {912, 618},
    {0, 0},
    {0, 0},
};

// TODO: fill in
SDL_Point g_playstation_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_COUNT] = {
};

// NOTE: need to work on this later.
// to change the size more.
#define IMAGE_SCALE_RATIO (2)
#define XBOX_IMAGE_WIDTH  (1452)
#define XBOX_IMAGE_HEIGHT (940)
#define XBOX_JOYSTICK_SZ  (169)

#define CONTROLLER_COLOR  0, 71, 171
#define PUPPETPIECE_COLOR 15,  30,  30
#define ACTIVATED_COLOR   200, 30,  50

// unscaled
#define MAX_JOYSTICK_DISPLACEMENT_PX (80)
#define MAX_TRIGGER_DISPLACEMENT_PX (25)

#define SCALED_MAX_JOYSTICK_DISPLACEMENT_PX (MAX_JOYSTICK_DISPLACEMENT_PX / IMAGE_SCALE_RATIO)
#define SCALED_MAX_TRIGGER_DISPLACEMENT_PX  (MAX_TRIGGER_DISPLACEMENT_PX / IMAGE_SCALE_RATIO)

#define SCALED_JOYSTICK_SZ                  (XBOX_JOYSTICK_SZ / IMAGE_SCALE_RATIO)
#define SCALED_WINDOW_WIDTH                 (XBOX_IMAGE_WIDTH / IMAGE_SCALE_RATIO)
#define SCALED_WINDOW_HEIGHT                (XBOX_IMAGE_HEIGHT / IMAGE_SCALE_RATIO)

// This color is unlikely to ever be naturally selected...
#define CHROMA_KEY_COLOR  (RGB(255, 255, 254))

static SDL_Texture* load_image_from_file(SDL_Renderer* renderer, const char* path)
{
    SDL_Surface* surface = IMG_Load(path);
    SDL_Texture* result = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(result, SDL_ScaleModeLinear);
    SDL_FreeSurface(surface);
    return result;
}

// This requires a platform native solution.
static void make_window_transparent(SDL_Window* window)
{
    SDL_SysWMinfo wm_info = {};
    SDL_VERSION(&wm_info.version);
    SDL_GetWindowWMInfo(window, &wm_info);

    HWND windows_handle = wm_info.info.win.window;

    assert(SUCCEEDED(
               SetWindowLong(windows_handle,
                             GWL_EXSTYLE,
                             GetWindowLong(windows_handle, GWL_EXSTYLE) | WS_EX_LAYERED)
           ));

    assert(SUCCEEDED(
               SetLayeredWindowAttributes(windows_handle, CHROMA_KEY_COLOR, 0, LWA_COLORKEY)
           ));
}

static void load_xbox_controller_assets(void);
static void load_playstation_controller_assets(void);
static void load_keyboard_key_assets(void);
static void load_controller_assets(void)
{
    load_xbox_controller_assets();
#if 0
    load_playstation_controller_assets();
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
static void draw_controller(SDL_Renderer* renderer)
{
    SDL_Texture** controller_asset_set = g_xbox_controller_assets;
    SDL_Point*    puppeter_point_set   = g_xbox_controller_puppet_piece_placements;

    // adjust asset appearance.
    // I know it shouldn't be in this loop, idc.
    {
        for (unsigned part_index = 0; part_index < XBOXCONTROLLER_ASSET_COUNT; ++part_index) {
            if (g_focused_gamecontroller) {
                SDL_SetTextureAlphaMod(controller_asset_set[part_index], 255);
            } else {
                SDL_SetTextureAlphaMod(controller_asset_set[part_index], 64);
            }
        }
    }

    // draw puppets background. (triggers)
    {
        {
            auto point = g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_LEFT_TRIGGER];
            SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO, point.y / IMAGE_SCALE_RATIO, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT};

            short axis_y = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) : 0;
            destination.y += axis_y/32767.0f * SCALED_MAX_TRIGGER_DISPLACEMENT_PX;

            {
                auto color = lerp_color(SDL_Color{ PUPPETPIECE_COLOR, 255 }, SDL_Color{ ACTIVATED_COLOR, 255 }, axis_y / 32767.0f);
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_LT_FILL], color.r, color.g, color.b);
            }

            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_LT_FILL], 0, &destination);

            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_LT], 255, 255, 255);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_LT], 0, &destination);
        }

        {
            auto point = g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_RIGHT_TRIGGER];
            SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO, point.y / IMAGE_SCALE_RATIO, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT};

            short axis_y = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) : 0;
            destination.y += axis_y/32767.0f * SCALED_MAX_TRIGGER_DISPLACEMENT_PX;

            {
                auto color = lerp_color(SDL_Color{ PUPPETPIECE_COLOR, 255 }, SDL_Color{ ACTIVATED_COLOR, 255 }, axis_y / 32767.0f);
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_RT_FILL], color.r, color.g, color.b);
            }

            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_RT_FILL], 0, &destination);

            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_RT], 255, 255, 255);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_RT], 0, &destination);
        }
    }

    // NOTE: enums are Xbox to be more obvious to read, but they are aligned for all controller types.
    {
        SDL_Rect destination = { 0, 0, SCALED_WINDOW_WIDTH, SCALED_WINDOW_HEIGHT };
        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], CONTROLLER_COLOR);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE_FILL], 0, &destination);

        SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 255, 255, 255);
        SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_BASE], 0, &destination);

        for (unsigned part_index = XBOXCONTROLLER_ASSET_BUTTON_A_FILL; part_index < XBOXCONTROLLER_ASSET_JOYSTICK; ++part_index) {
            if (g_focused_gamecontroller && SDL_GameControllerGetButton(g_focused_gamecontroller, (SDL_GameControllerButton)g_part_id_to_button[part_index])) {
                SDL_SetTextureColorMod(controller_asset_set[part_index], ACTIVATED_COLOR);
            } else {
                SDL_SetTextureColorMod(controller_asset_set[part_index], PUPPETPIECE_COLOR);
            }

            SDL_RenderCopy(renderer, controller_asset_set[part_index], 0, &destination);
        }
    }

    // draw puppet parts foreground. (joysticks)
    {
        {
            auto point = g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_JOYSTICK_LEFT];
            SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, point.y / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, SCALED_JOYSTICK_SZ, SCALED_JOYSTICK_SZ};

            short axis_x = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_LEFTX) : 0;
            short axis_y = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_LEFTY) : 0;

            destination.x += axis_x/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;
            destination.y += axis_y/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;

            if (SDL_GameControllerGetButton(g_focused_gamecontroller, SDL_CONTROLLER_BUTTON_LEFTSTICK)) {
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], ACTIVATED_COLOR);
            } else {
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], PUPPETPIECE_COLOR);
            }

            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], 0, &destination);

            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 255, 255, 255);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 0, &destination);
        }

        {
            auto point = g_xbox_controller_puppet_piece_placements[CONTROLLER_PUPPET_POINT_JOYSTICK_RIGHT];
            SDL_Rect destination = {point.x / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, point.y / IMAGE_SCALE_RATIO - SCALED_JOYSTICK_SZ/2, SCALED_JOYSTICK_SZ, SCALED_JOYSTICK_SZ};

            short axis_x = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_RIGHTX) : 0;
            short axis_y = (g_focused_gamecontroller) ? SDL_GameControllerGetAxis(g_focused_gamecontroller, SDL_CONTROLLER_AXIS_RIGHTY) : 0;

            destination.x += axis_x/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;
            destination.y += axis_y/32767.0f * SCALED_MAX_JOYSTICK_DISPLACEMENT_PX;

            if (SDL_GameControllerGetButton(g_focused_gamecontroller, SDL_CONTROLLER_BUTTON_RIGHTSTICK)) {
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], ACTIVATED_COLOR);
            } else {
                SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], PUPPETPIECE_COLOR);
            }

            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK_FILL], 0, &destination);

            SDL_SetTextureColorMod(controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 255, 255, 255);
            SDL_RenderCopy(renderer, controller_asset_set[XBOXCONTROLLER_ASSET_JOYSTICK], 0, &destination);
        }
    }
}

static int application_main(int argc, char** argv)
{
    load_controller_assets();

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
    static const char* asset_image_list[] = {
        "Controllers/Xbox/Base.png",
        "Controllers/Xbox/BaseFillmask.png",
        "Controllers/Xbox/ButtonAFillmask.png",
        "Controllers/Xbox/ButtonBFillmask.png",
        "Controllers/Xbox/ButtonXFillmask.png",
        "Controllers/Xbox/ButtonYFillmask.png",
        "Controllers/Xbox/ButtonDpadUpFillmask.png",
        "Controllers/Xbox/ButtonDpadDownFillmask.png",
        "Controllers/Xbox/ButtonDpadLeftFillmask.png",
        "Controllers/Xbox/ButtonDpadRightFillmask.png",
        "Controllers/Xbox/ButtonMenuFillmask.png",
        "Controllers/Xbox/ButtonStartFillmask.png",
        "Controllers/Xbox/ButtonRBFillmask.png",
        "Controllers/Xbox/ButtonLBFillmask.png",
        "Controllers/Xbox/Joystick.png",
        "Controllers/Xbox/JoystickFillMask.png",
        "Controllers/Xbox/RightTrigger.png",
        "Controllers/Xbox/RightTriggerFillmask.png",
        "Controllers/Xbox/LeftTrigger.png",
        "Controllers/Xbox/LeftTriggerFillmask.png",
    };

    for (unsigned index = 0; index < XBOXCONTROLLER_ASSET_COUNT; ++index) {
        g_xbox_controller_assets[index] = load_image_from_file(g_renderer, asset_image_list[index]);
    }
}

static void load_playstation_controller_assets(void)
{
    static const char* asset_image_list[] = {
        "Controllers/Playstation/Base.png",
        "Controllers/Playstation/BaseFillmask.png",
        "Controllers/Playstation/ButtonCrossFillmask.png",
        "Controllers/Playstation/ButtonCircleFillmask.png",
        "Controllers/Playstation/ButtonSquareFillmask.png",
        "Controllers/Playstation/ButtonTriangleFillmask.png",
        "Controllers/Playstation/ButtonDpadUpFillmask.png",
        "Controllers/Playstation/ButtonDpadDownFillmask.png",
        "Controllers/Playstation/ButtonDpadLeftFillmask.png",
        "Controllers/Playstation/ButtonDpadRightFillmask.png",
        "Controllers/Playstation/ButtonMenuFillmask.png",
        "Controllers/Playstation/ButtonStartFillmask.png",
        "Controllers/Playstation/ButtonRBFillmask.png",
        "Controllers/Playstation/ButtonLBFillmask.png",
        "Controllers/Playstation/Joystick.png",
        "Controllers/Playstation/JoystickFillMask.png",
        "Controllers/Playstation/RightTrigger.png",
        "Controllers/Playstation/RightTriggerFillmask.png",
        "Controllers/Playstation/LeftTrigger.png",
        "Controllers/Playstation/LeftTriggerFillmask.png",
    };

    for (unsigned index = 0; index < PLAYSTATIONCONTROLLER_ASSET_COUNT; ++index) {
        g_playstation_controller_assets[index] = load_image_from_file(g_renderer, asset_image_list[index]);
    }
}

static void load_keyboard_key_assets(void)
{
    assert(0 && "Not done.");
}
