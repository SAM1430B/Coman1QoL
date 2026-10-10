// What winmm.dll needs from a module (a *.c1m file in the game folder: a DLL
// with its own extension).
//
// Each module exports:
//     extern "C" __declspec(dllexport) void __cdecl c1qol_start();
// winmm.dll calls it once, when the game's code can be patched and before any
// of it has run. The module checks game::detect_build() itself.
#pragma once

namespace c1qol {

constexpr char kStartName[] = "c1qol_start";
using StartFn = void(__cdecl*)();

}  // namespace c1qol
