// Settings: [Resolution] in c1m.ini, and "-res WxH" on the command line.
#include "config.hpp"
#include "log.hpp"
#include "settings.hpp"
#include <windows.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {

Config g_cfg;

// "-res 960x1080" (or "-res=960x1080") on the game's command line. The command
// line starts with the exe's path, which may contain "-res" too: keep looking.
bool size_from_command_line(int& w, int& h)
{
    for (const char* p = GetCommandLineA(); (p = std::strstr(p, "-res")) != nullptr;) {
        p += 4;
        if (*p != ' ' && *p != '=' && *p != ':') continue;
        while (*p == ' ' || *p == '=' || *p == ':') ++p;
        char* end;
        const long a = std::strtol(p, &end, 10);
        if (*end != 'x' && *end != 'X') continue;
        const long b = std::strtol(end + 1, &end, 10);
        if (a > 0 && b > 0) {
            w = static_cast<int>(a);
            h = static_cast<int>(b);
            return true;
        }
    }
    return false;
}

}  // namespace

const Config& cfg() { return g_cfg; }

void load_config()
{
    int w = settings::get_int("Resolution", "Width", 0);
    int h = settings::get_int("Resolution", "Height", 0);
    const char* source = "c1m.ini";
    if (size_from_command_line(w, h)) {
        source = "command line";
    } else if (w <= 0 || h <= 0) {  // 0 = the primary monitor's size
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
        source = "desktop";
    }
    // At least 640x480 (the menus are drawn for it), even width (some blitters work on pixel pairs).
    g_cfg.width = std::clamp(w & ~1, 640, 4096);
    g_cfg.height = std::clamp(h, 480, 4096);

    logging::line("%dx%d (from %s)", g_cfg.width, g_cfg.height, source);
}
