// mem::install, logging the hook that couldn't be installed (Log=1).
#pragma once
#include "memory.hpp"

bool install_hook_ptr(const char* what, const mem::HookSite& site, const void* hook, void** original);

template <typename Hook> bool install_hook(const char* what, const mem::HookSite& site, Hook* hook)
{
    return install_hook_ptr(what, site, reinterpret_cast<const void*>(hook), nullptr);
}

// Same, setting *original to a function pointer that calls the original.
template <typename Hook, typename Fn>
bool install_hook(const char* what, const mem::HookSite& site, Hook* hook, Fn* original)
{
    void* p = nullptr;
    if (!install_hook_ptr(what, site, reinterpret_cast<const void*>(hook), &p)) return false;
    *original = reinterpret_cast<Fn>(p);
    return true;
}
