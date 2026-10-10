#include "log.hpp"
#include "settings.hpp"
#include <windows.h>
#include <cstdarg>
#include <cstdio>

namespace logging {

bool enabled()
{
    static int on = -1;  // not read yet
    if (on < 0) on = settings::get_int("Misc", "Log", 0) != 0;
    return on != 0;
}

void line(const char* fmt, ...)
{
    static FILE* f = nullptr;
    if (!enabled()) return;
    if (!f) {
        char path[MAX_PATH];
        std::snprintf(path, sizeof path, "%sc1m.log", settings::game_dir());
        f = std::fopen(path, "a");
        if (!f) return;
    }
    std::fprintf(f, "[%5lu] %s: ", GetCurrentProcessId(), kModule);
    va_list ap;
    va_start(ap, fmt);
    std::vfprintf(f, fmt, ap);
    va_end(ap);
    std::fputc('\n', f);
    std::fflush(f);
}

}  // namespace logging
