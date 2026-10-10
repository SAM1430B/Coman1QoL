// c1m.log next to the game exe, written when [Misc] Log=1.
// All the DLLs write to the same file.
#pragma once
#include "compat.hpp"

namespace logging {

// Each DLL names itself for its log lines, e.g. const char logging::kModule[] = "Resolution";
extern const char kModule[];

bool enabled();

// printf-style line, starting with the process id and kModule.
void line(const char* fmt, ...) C1QOL_PRINTF(1, 2);

}  // namespace logging
