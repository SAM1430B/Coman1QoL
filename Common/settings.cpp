#include "settings.hpp"
#include <windows.h>
#include <cstdio>
#include <cstring>

extern "C" IMAGE_DOS_HEADER __ImageBase;  // this DLL

namespace settings {

const char* game_dir()
{
    static char dir[MAX_PATH];
    if (!dir[0]) {
        GetModuleFileNameA(reinterpret_cast<HMODULE>(&__ImageBase), dir, sizeof dir);
        if (char* slash = std::strrchr(dir, '\\')) slash[1] = 0; else dir[0] = 0;
    }
    return dir;
}

int get_int(const char* section, const char* key, int fallback)
{
    char ini[MAX_PATH];
    std::snprintf(ini, sizeof ini, "%sc1m.ini", game_dir());
    return static_cast<int>(GetPrivateProfileIntA(section, key, fallback, ini));
}

}  // namespace settings
