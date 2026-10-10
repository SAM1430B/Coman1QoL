#pragma once
// winmm.dll proxy: resolves the real system winmm exports for the jump stubs in proxy_stubs.cpp.
namespace proxy { void load_real_winmm(); }
