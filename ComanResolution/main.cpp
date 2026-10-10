// Resolution.c1m: play Commandos at any resolution.
// winmm.dll (WinmmProxy) calls c1qol_start() before the game's code runs.
// Settings: [Resolution] in c1m.ini.
#include "config.hpp"
#include "game.hpp"
#include "log.hpp"

const char logging::kModule[] = "Resolution";

extern "C" __declspec(dllexport) void __cdecl c1qol_start()
{
    const game::Build build = game::detect_build();
    if (!game::select_target(build)) {
        logging::line("no addresses for %s - not patching", game::build_name(build));
        return;
    }
    load_config();
    if (!resolution::install()) return;  // unexpected code: leave everything alone
    assets::install();
    top_bar::install();
    side_bar::install();
    small_maps::install();
    cursor::install();
    logging::line("patches applied");
}
