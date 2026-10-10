// c1m.ini, next to the game exe. Each module reads its own section.
#pragma once

namespace settings {

// The game folder (where winmm.dll and the modules are), with a trailing backslash.
const char* game_dir();

// A whole number from c1m.ini, or `fallback` if it isn't there.
int get_int(const char* section, const char* key, int fallback);

}  // namespace settings
