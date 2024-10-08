#include <SDL2/SDL.h>
#include "config.h"

#include <fstream>

#include "constants.h"

#include "mouse_asset_id.h"
#include "keyboard_asset_id.h"
#include "xbox_controller_asset_id.h"

#include "mouse_asset_set.h"
#include "keyboard_asset_set.h"
#include "controller_asset_set.h"
#include "device_mode.h"

#define DEFAULT_INPUT_FILE ("./inputoverlay.cfg")

static int clamp(int v, int a, int b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

OverlaySettings::OverlaySettings(void) :
    controller_color({CONTROLLER_COLOR}),
    button_color({PUPPETPIECE_COLOR}),
    activated_color({ACTIVATED_COLOR}),
    image_scale_ratio(IMAGE_SCALE_RATIO),
    autodetect_controller(false),
    last_device_mode_id(DEVICE_MODE_USING_CONTROLLER),
    last_device_asset_set_id(CONTROLLER_ASSET_SET_XBOX),
    always_on_top(true),
    use_mouse_move(false)
{
    
}

// This stuff is not very error tolerant, but for a private
// tool basically, this is okay, and also not sure why you would
// want to break this thing in particular.
void write_config(const OverlaySettings& settings)
{
    std::ofstream output(DEFAULT_INPUT_FILE);
    output << "ControllerColor = " << color_into_hex_string(settings.controller_color) << std::endl;
    output << "ButtonColor = " << color_into_hex_string(settings.button_color) << std::endl;
    output << "ActivatedColor = " << color_into_hex_string(settings.activated_color) << std::endl;
    output << "ImageScaleRatio = " << settings.image_scale_ratio << std::endl;
    output << "AutoDetectController = " << (int)settings.autodetect_controller << std::endl;
    output << "LastDeviceType = " << (int)settings.last_device_mode_id << std::endl;
    output << "LastDeviceAssetType = " << (int)settings.last_device_asset_set_id << std::endl;
    output << "AlwaysOnTop = " << (int)settings.always_on_top << std::endl;
    output << "UseMouseMove = " << (int)settings.use_mouse_move << std::endl;
    output.close();
}

static bool try_read_config_line(std::ifstream& stream, OverlaySettings& settings) {
    // Only fail if there is no more text.
    if (stream.eof())
        return false;

    std::string fieldname;
    std::string equals;
    std::string textvalue;
    std::string garbage;

    stream >> fieldname;
    if (stream.fail()) return false;
    stream >> equals;
    if (stream.fail()) return false;
    if (equals != "=") std::getline(stream, garbage); // eliminate line hopefully.
    stream >> textvalue;
    if (stream.fail()) return false;

    if (fieldname == "ControllerColor") {
        settings.controller_color = color_from_hex_string(textvalue.c_str());
    } else if (fieldname == "ButtonColor") {
        settings.button_color = color_from_hex_string(textvalue.c_str());
    } else if (fieldname == "ActivatedColor") {
        settings.activated_color = color_from_hex_string(textvalue.c_str());
    } else if (fieldname == "ImageScaleRatio") {
        settings.image_scale_ratio = std::max(std::atoi(textvalue.c_str()), 1);
    } else if (fieldname == "AutoDetectController") {
        settings.autodetect_controller = (bool)std::atoi(textvalue.c_str());
    } else if (fieldname == "LastDeviceType") {
        settings.last_device_mode_id = clamp(std::atoi(textvalue.c_str()), 0, DEVICE_MODE_COUNT-1);
    } else if (fieldname == "LastDeviceAssetType") {
        settings.last_device_asset_set_id = std::max(std::atoi(textvalue.c_str()), 0);
    } else if (fieldname == "AlwaysOnTop") {
        settings.always_on_top = (bool)std::atoi(textvalue.c_str());
    } else if (fieldname == "UseMouseMove") {
        settings.use_mouse_move = (bool)std::atoi(textvalue.c_str());
    }

    return true;
}

void load_config(OverlaySettings& settings)
{
    std::string _rdtmp;
    std::ifstream input(DEFAULT_INPUT_FILE);

    if (input.is_open()) {
        while (try_read_config_line(input, settings))
            continue;
    }
}

static inline Uint8 hexdigit_value(char d)
{
    switch (d)
    {
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7':
        case '8': case '9':
            return d - '0';
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
            return (d - 'a') + 10;
    }

    return (Uint8)(-1);
}

static inline char nibble_to_hex(Uint8 nibble)
{
    static const char* _lookup = "0123456789abcdef";
    nibble &= 0xF;
    return _lookup[nibble];
}

static Uint8 parse_hex_pair(const char byte[2])
{
    char byte_normalized[2] = {
        (char)tolower(byte[0]),
        (char)tolower(byte[1])
    };

    return hexdigit_value(byte_normalized[0]) * 16 + hexdigit_value(byte_normalized[1]);
}

SDL_Color color_from_hex_string(const char* hexstr)
{
    // assume string length is 7
    // starts with hash.
    SDL_Color result = {};

    if (strlen(hexstr) != 7) {
        return result;   
    }

    if (hexstr[0] != '#') {
        return result;
    }

    hexstr++;
    result.r = parse_hex_pair(hexstr);
    hexstr += 2;
    result.g = parse_hex_pair(hexstr);
    hexstr += 2;
    result.b = parse_hex_pair(hexstr);

    return result;
}

static size_t hexout(char* buf, Uint8 byte)
{
    buf[0] = nibble_to_hex(byte >> 4);
    buf[1] = nibble_to_hex(byte >> 0);
    return 2; 
}

// formatted as standard #AABBCC
// hexcodes.
char* color_into_hex_string(SDL_Color color)
{
    static char buf[16];
    buf[0] = '#';
    hexout(buf+1,   (Uint8)color.r);
    hexout(buf+3, (Uint8)color.g);
    hexout(buf+5, (Uint8)color.b);
    return buf;
}
