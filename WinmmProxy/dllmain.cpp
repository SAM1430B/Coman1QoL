// Coman1QoL: quality-of-life mods for Commandos.
// This is winmm.dll. The game loads it from its own folder, before any of the
// game's code runs. It forwards the winmm calls to the real winmm.dll
// (proxy.cpp), then loads every *.c1m module next to the exe and calls
// its c1qol_start() (see Common/module.hpp). Settings: c1m.ini next to the exe.
//
// Steam's coman_mp.exe (Beyond the Call of Duty) is encrypted on disk and is
// only decrypted after DllMain returns. So we hook the exe's GetCommandLineA
// import, which the C runtime calls first thing, and start the modules on the
// first call made once the code is ready. A background thread polls as a fallback.
#include "game_build.hpp"
#include "log.hpp"
#include "memory.hpp"
#include "module.hpp"
#include "proxy.hpp"
#include "settings.hpp"
#include <windows.h>
#include <cstdio>
#include <cstring>

const char logging::kModule[] = "winmm";

namespace {

game::Build g_build = game::Build::Unknown;
volatile LONG g_started = 0;
LPSTR(WINAPI* g_get_cmdline)() = nullptr;
LPSTR* g_iat_slot = nullptr;

// Is `name` a module: does it end in exactly ".c1m"? (Windows also matches
// "*.c1m" to longer extensions such as ".c1mx".)
bool is_module_name(const char* name)
{
    const std::size_t n = std::strlen(name);
    return n > 4 && lstrcmpiA(name + n - 4, ".c1m") == 0;
}

// Loads every *.c1m module in the game folder and starts it.
void start_modules()
{
    if (InterlockedExchange(&g_started, 1)) return;
    char pattern[MAX_PATH];
    std::snprintf(pattern, sizeof pattern, "%s*.c1m", settings::game_dir());
    WIN32_FIND_DATAA found;
    const HANDLE search = FindFirstFileA(pattern, &found);
    if (search == INVALID_HANDLE_VALUE) {
        logging::line("no *.c1m modules found");
        return;
    }
    do {
        if (!is_module_name(found.cFileName)) continue;
        char path[MAX_PATH];
        std::snprintf(path, sizeof path, "%s%s", settings::game_dir(), found.cFileName);
        const HMODULE module = LoadLibraryA(path);
        const auto start = module ? reinterpret_cast<c1qol::StartFn>(GetProcAddress(module, c1qol::kStartName))
                                  : nullptr;
        if (!start) {
            logging::line("%s is not a Coman1QoL module, skipped", found.cFileName);
            if (module) FreeLibrary(module);
            continue;
        }
        logging::line("starting %s", found.cFileName);
        start();
    } while (FindNextFileA(search, &found));
    FindClose(search);
}

void restore_iat()
{
    if (g_iat_slot && *g_iat_slot != reinterpret_cast<LPSTR>(g_get_cmdline))
        mem::write<void*>(reinterpret_cast<mem::addr_t>(g_iat_slot), reinterpret_cast<void*>(g_get_cmdline));
}

void try_start()
{
    if (!g_started && game::code_ready(g_build)) {
        start_modules();
        restore_iat();
    }
}

LPSTR WINAPI hook_get_cmdline()
{
    try_start();
    return g_get_cmdline();
}

DWORD WINAPI poll_thread(LPVOID)
{
    for (int i = 0; i < 1200 && !g_started; i++) {  // up to 60 s
        Sleep(50);
        try_start();
    }
    if (!g_started) logging::line("game code never became ready - not patching");
    return 0;
}

// Logs what identifies an unsupported exe, so it can be reported and added.
void log_unknown_exe()
{
    char path[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, path, sizeof path);
    DWORD size = 0;
    const HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                    OPEN_EXISTING, 0, nullptr);
    if (file != INVALID_HANDLE_VALUE) {
        size = GetFileSize(file, nullptr);
        CloseHandle(file);
    }
    const auto* exe = reinterpret_cast<const std::uint8_t*>(GetModuleHandleA(nullptr));
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        exe + reinterpret_cast<const IMAGE_DOS_HEADER*>(exe)->e_lfanew);
    const char* name = std::strrchr(path, '\\');
    name = name ? name + 1 : path;
    logging::line("game exe not recognised - not patching");
    logging::line("  %s, %lu bytes, PE timestamp 0x%08lX", name,
                  static_cast<unsigned long>(size),
                  static_cast<unsigned long>(nt->FileHeader.TimeDateStamp));
}

void start()
{
    g_build = game::detect_build();
    if (g_build == game::Build::Unknown) {
        log_unknown_exe();
        return;
    }
    logging::line("detected %s", game::build_name(g_build));
    if (game::code_ready(g_build)) {
        start_modules();
        return;
    }
    logging::line("game code is still encrypted, starting the modules once it is decrypted");
    auto** slot = mem::find_import("KERNEL32.dll", "GetCommandLineA");
    if (slot) {
        g_iat_slot = reinterpret_cast<LPSTR*>(slot);
        g_get_cmdline = reinterpret_cast<LPSTR(WINAPI*)()>(*slot);
        mem::write<void*>(reinterpret_cast<mem::addr_t>(slot), reinterpret_cast<void*>(&hook_get_cmdline));
    }
    if (HANDLE h = CreateThread(nullptr, 0, poll_thread, nullptr, 0, nullptr)) CloseHandle(h);
}

}  // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH) return TRUE;
    DisableThreadLibraryCalls(inst);
    proxy::load_real_winmm();
    start();
    return TRUE;
}
