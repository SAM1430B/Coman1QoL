// Small helpers for reading and patching the game's memory in place.
#pragma once
#include "compat.hpp"
#include <windows.h>
#include <cstdint>
#include <cstring>

namespace mem {

using addr_t = std::uintptr_t;

// Typed view of an absolute address in the exe.
template <typename T> inline T& at(addr_t a) { return *reinterpret_cast<T*>(a); }

// Typed view of a field at a byte offset inside a game object.
template <typename T> inline T& field(const void* obj, std::size_t off)
{
    return *reinterpret_cast<T*>(reinterpret_cast<std::uintptr_t>(obj) + off);
}

// Pointer stored in a global (e.g. the display object at 0x7B72E4).
template <typename T = std::uint8_t> inline T* global_ptr(addr_t a) { return at<T*>(a); }

// Cast an address to a function pointer type.
template <typename Fn> inline Fn fn(addr_t a) { return reinterpret_cast<Fn>(a); }

// Virtual function `index` (byte offset / 4) of a game object.
template <typename Fn> inline Fn vfunc(const void* obj, std::size_t byte_off)
{
    return reinterpret_cast<Fn>((*reinterpret_cast<void* const* const*>(obj))[byte_off / 4]);
}

// Makes a code range writable for the lifetime of the object.
class Unprotect {
public:
    Unprotect(addr_t a, std::size_t n) : a_(a), n_(n)
    {
        VirtualProtect(reinterpret_cast<void*>(a_), n_, PAGE_EXECUTE_READWRITE, &old_);
    }
    ~Unprotect()
    {
        VirtualProtect(reinterpret_cast<void*>(a_), n_, old_, &old_);
        FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(a_), n_);
    }
    Unprotect(const Unprotect&) = delete;
    Unprotect& operator=(const Unprotect&) = delete;

private:
    addr_t a_;
    std::size_t n_;
    DWORD old_ = 0;
};

inline bool bytes_equal(addr_t a, const void* expected, std::size_t n)
{
    return std::memcmp(reinterpret_cast<const void*>(a), expected, n) == 0;
}

template <std::size_t N> inline bool bytes_equal(addr_t a, const std::uint8_t (&expected)[N])
{
    return bytes_equal(a, expected, N);
}

constexpr std::uint8_t kCall = 0xE8, kJmp = 0xE9;

// Is there a `call`/`jmp rel32` at `a` that goes to `target`?
inline bool rel_targets(addr_t a, std::uint8_t op, addr_t target)
{
    return at<std::uint8_t>(a) == op && a + 5 + at<std::int32_t>(a + 1) == target;
}

// Write `call`/`jmp rel32` to `dest` at `a`.
inline void write_rel(addr_t a, std::uint8_t op, const void* dest)
{
    Unprotect u(a, 5);
    at<std::uint8_t>(a) = op;
    at<std::int32_t>(a + 1) =
        static_cast<std::int32_t>(reinterpret_cast<addr_t>(dest) - (a + 5));
}

template <typename T> inline void write(addr_t a, T value)
{
    Unprotect u(a, sizeof(T));
    at<T>(a) = value;
}

// The exe's import address table slot for dll!name, or nullptr.
inline void** find_import(const char* dll, const char* name)
{
    auto* base = reinterpret_cast<std::uint8_t*>(GetModuleHandleA(nullptr));
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        base + reinterpret_cast<const IMAGE_DOS_HEADER*>(base)->e_lfanew);
    const auto& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return nullptr;
    for (auto* d = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(base + dir.VirtualAddress); d->Name; ++d) {
        if (lstrcmpiA(reinterpret_cast<const char*>(base + d->Name), dll) != 0) continue;
        if (!d->OriginalFirstThunk) continue;
        auto* names = reinterpret_cast<const IMAGE_THUNK_DATA*>(base + d->OriginalFirstThunk);
        auto* slots = reinterpret_cast<void**>(base + d->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++slots) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            const auto* by_name = reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char*>(by_name->Name), name) == 0) return slots;
        }
    }
    return nullptr;
}

// Where and how a game function is hooked.
struct HookSite {
    enum Kind {
        None,        // not available in this build
        ThunkJmp,    // `jmp target` (incremental-link thunk) at `at`: retarget the jmp
        CallSite,    // `call target` at `at`: retarget the call
        VtableSlot,  // function pointer `target` stored at `at`: replace it
        Detour,      // function `target` starts with `prologue`: jmp to the hook, keep a trampoline
        Replace,     // function `target` starts with `prologue`: jmp to the hook, no way back
        InlineEsi,   // inlined code block at `at` (starts with `prologue`), object in esi:
                     // replaced by `mov ecx, esi; call hook; jmp target`
    };
    Kind kind = None;
    addr_t at = 0;
    addr_t target = 0;
    const std::uint8_t* prologue = nullptr;
    std::size_t prologue_len = 0;
};

// Shorthands for the address tables (targets.cpp).
constexpr HookSite thunk(addr_t at, addr_t fn)       { return { HookSite::ThunkJmp, at, fn }; }
constexpr HookSite call_site(addr_t at, addr_t fn)   { return { HookSite::CallSite, at, fn }; }
constexpr HookSite vtable_slot(addr_t at, addr_t fn) { return { HookSite::VtableSlot, at, fn }; }
template <std::size_t N> constexpr HookSite detour(addr_t fn, const std::uint8_t (&prologue)[N])
{
    return { HookSite::Detour, 0, fn, prologue, N };
}
template <std::size_t N> constexpr HookSite replace(addr_t fn, const std::uint8_t (&prologue)[N])
{
    return { HookSite::Replace, 0, fn, prologue, N };
}
template <std::size_t N> constexpr HookSite inline_esi(addr_t at, addr_t resume, const std::uint8_t (&block)[N])
{
    return { HookSite::InlineEsi, at, resume, block, N };
}

// Installs `hook` at `site`. On success returns true and sets *original to a
// pointer that calls the original function (nullptr for Replace).
inline bool install(const HookSite& site, const void* hook, void** original = nullptr)
{
    void* orig = nullptr;
    switch (site.kind) {
    case HookSite::ThunkJmp:
    case HookSite::CallSite: {
        const std::uint8_t op = site.kind == HookSite::ThunkJmp ? kJmp : kCall;
        if (!rel_targets(site.at, op, site.target)) return false;
        write_rel(site.at, op, hook);
        orig = reinterpret_cast<void*>(site.target);
        break;
    }
    case HookSite::VtableSlot:
        if (at<addr_t>(site.at) != site.target) return false;
        write<const void*>(site.at, hook);
        orig = reinterpret_cast<void*>(site.target);
        break;
    case HookSite::InlineEsi: {
        if (!site.prologue || site.prologue_len < 12 || !bytes_equal(site.at, site.prologue, site.prologue_len))
            return false;
        Unprotect u(site.at, 12);
        at<std::uint8_t>(site.at) = 0x8B;      // mov ecx, esi
        at<std::uint8_t>(site.at + 1) = 0xCE;
        at<std::uint8_t>(site.at + 2) = kCall;
        at<std::int32_t>(site.at + 3) =
            static_cast<std::int32_t>(reinterpret_cast<addr_t>(hook) - (site.at + 7));
        at<std::uint8_t>(site.at + 7) = kJmp;
        at<std::int32_t>(site.at + 8) = static_cast<std::int32_t>(site.target - (site.at + 12));
        break;
    }
    case HookSite::Detour:
    case HookSite::Replace: {
        if (!site.prologue || site.prologue_len < 5 ||
            !bytes_equal(site.target, site.prologue, site.prologue_len))
            return false;
        if (site.kind == HookSite::Detour) {
            auto* t = static_cast<std::uint8_t*>(VirtualAlloc(
                nullptr, site.prologue_len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
            if (!t) return false;
            std::memcpy(t, site.prologue, site.prologue_len);
            t[site.prologue_len] = kJmp;
            *reinterpret_cast<std::int32_t*>(t + site.prologue_len + 1) = static_cast<std::int32_t>(
                (site.target + site.prologue_len) -
                (reinterpret_cast<addr_t>(t) + site.prologue_len + 5));
            orig = t;
        }
        write_rel(site.target, kJmp, hook);
        break;
    }
    default:
        return false;
    }
    if (original) *original = orig;
    return true;
}

}  // namespace mem
