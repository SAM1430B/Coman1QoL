// Compiler differences between MSVC (Visual Studio) and GCC/Clang (MinGW).
//
// Game functions use __thiscall (this in ecx, args on the stack, callee pops).
// MSVC doesn't allow __thiscall on free functions, so every hook and every call
// into the game uses __fastcall instead, which has the same stack layout and
// cleanup. The second register argument (edx) is unused:
//     int __fastcall hook(void* self, void* /*edx*/, int a, int b);
#pragma once
#include <cstdint>

#if defined(_MSC_VER) || defined(C1QOL_MSVC_ASM)
#  include <intrin.h>
#  define C1QOL_MS_ASM 1
#  define C1QOL_NAKED __declspec(naked)
#  define C1QOL_RETURN_ADDRESS() reinterpret_cast<std::uintptr_t>(_ReturnAddress())
#else
#  define C1QOL_NAKED __attribute__((naked))
#  define C1QOL_RETURN_ADDRESS() reinterpret_cast<std::uintptr_t>(__builtin_return_address(0))
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define C1QOL_PRINTF(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#  define C1QOL_PRINTF(fmt_idx, arg_idx)
#endif

#if !defined(_M_IX86) && !defined(__i386__)
#  error "Build for 32-bit x86 (Win32): the game is a 32-bit program."
#endif
