// Which supported game exe is running. Each module has its own addresses for
// these builds (see docs/ADDRESSES.md).
#pragma once

namespace game {

enum class Build {
    Unknown,
    Bel,    // Behind Enemy Lines, Steam/GOG
    Bcd,    // Beyond the Call of Duty, Steam/GOG
    Bcd99,  // Beyond the Call of Duty, 1999 retail
    Bel98,  // Behind Enemy Lines, 1998 retail
};

Build detect_build();
const char* build_name(Build b);

// False while Steam's coman_mp.exe is still encrypted (see WinmmProxy/dllmain.cpp).
bool code_ready(Build b);

}  // namespace game
