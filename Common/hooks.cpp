#include "hooks.hpp"
#include "log.hpp"

bool install_hook_ptr(const char* what, const mem::HookSite& site, const void* hook, void** original)
{
    if (site.kind == mem::HookSite::None) return false;
    const bool ok = mem::install(site, hook, original);
    if (!ok) logging::line("%s: code not as expected, not hooked", what);
    return ok;
}
