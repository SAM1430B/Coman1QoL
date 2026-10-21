// Mouse cursor. Every mouse move redraws the cursor straight onto the screen
// (copy the area from the screen, put the background back, draw the cursor,
// copy it back), between frames. At a big size each of those screen copies
// can be slow, and a mouse sends hundreds of moves a second, so the game froze
// while the mouse was moving. Here those redraws happen at most 60 times a
// second, and a skipped one is done right after the next frame.
#include "game.hpp"
#include <windows.h>

using namespace game;

namespace {

void(__fastcall* g_redraw_orig)(void* windows, void* edx, int moved);
void(__fastcall* g_present_orig)(void* windows, void* edx);
LARGE_INTEGER g_freq, g_last;
bool g_pending;  // the cursor moved but wasn't redrawn

void redraw(void* windows, void* edx)
{
    QueryPerformanceCounter(&g_last);
    g_pending = false;
    g_redraw_orig(windows, edx, 1);
}

void __fastcall hook_redraw(void* windows, void* edx, int moved)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (now.QuadPart - g_last.QuadPart < g_freq.QuadPart / 60) {
        g_pending = true;
        return;
    }
    redraw(windows, edx);
}

// In BCD only:
// While present has its gate field set; outside it, present has just drawn the
// cursor at its latest position into the frame, and a redraw would put a stale
// background over it (the cursor vanished while moving fast).
bool redraw_allowed(void* windows)
{
    const int gate = target().cursor_redraw_gate;
    return gate == 0 || mem::field<int>(windows, gate) != 0;
}

void __fastcall hook_present(void* windows, void* edx)
{
    g_present_orig(windows, edx);
    if (!g_pending) return;
    if (redraw_allowed(windows)) redraw(windows, edx);
    else g_pending = false;
}

}  // namespace

namespace cursor {

void install()
{
    QueryPerformanceFrequency(&g_freq);
    const Target& t = target();
    if (install_hook("frame present", t.present, &hook_present, &g_present_orig))
        install_hook("cursor redraw", t.cursor_redraw, &hook_redraw, &g_redraw_orig);
}

}  // namespace cursor
