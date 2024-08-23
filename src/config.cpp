#include <SDL2/SDL.h>
#include "config.h"

#include <fstream>

#include "constants.h"

#define DEFAULT_INPUT_FILE ("./inputoverlay.cfg")

OverlaySettings::OverlaySettings(void) :
    controller_color({CONTROLLER_COLOR}),
    button_color({PUPPETPIECE_COLOR}),
    activated_color({ACTIVATED_COLOR}),
    image_scale_ratio(IMAGE_SCALE_RATIO)
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
    output << "NOTE: please do not shuffle any of the fields in this file." << std::endl;
    output.close();
}

void load_config(OverlaySettings& settings)
{
    std::string _rdtmp;
    std::ifstream input(DEFAULT_INPUT_FILE);

    if (input.is_open()) {
        // NOTE: I assume a fixed format, preferably as above.
        // do not shuffle the order of anything.

        input >> _rdtmp; input >> _rdtmp; // Name, Equal Sign
        input >> _rdtmp; settings.controller_color = color_from_hex_string(_rdtmp.c_str());

        input >> _rdtmp; input >> _rdtmp; // Name, Equal Sign
        input >> _rdtmp; settings.button_color = color_from_hex_string(_rdtmp.c_str());

        input >> _rdtmp; input >> _rdtmp; // Name, Equal Sign
        input >> _rdtmp; settings.activated_color = color_from_hex_string(_rdtmp.c_str());

        input >> _rdtmp; input >> _rdtmp; // Name, Equal Sign
        input >> _rdtmp;
        settings.image_scale_ratio = std::max(std::atoi(_rdtmp.c_str()), 1);
        // input >> settings.image_scale_ratio;
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
