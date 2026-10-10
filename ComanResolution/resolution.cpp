// Resolution control: maps the game's four fixed modes to the custom size.
#include "game.hpp"
#include "config.hpp"
#include "log.hpp"
#include <cstring>

using namespace game;

// Fills a rectangle of the display's back buffer with black (clipped to the screen).
void clear_rect(int x, int y, int w, int h)
{
    Display d = Display::get();
    if (!d) return;
    const int sw = d.width(), sh = d.height();
    if (sw <= 0 || sh <= 0 || sw > 4096 || sh > 4096) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > sw) w = sw - x;
    if (y + h > sh) h = sh - y;
    if (w <= 0 || h <= 0) return;

    const Target& t = target();
    auto unlock = mem::vfunc<void(__fastcall*)(void*, void*)>(d.p, t.unlock_vt);
    void* pixels = nullptr;
    int pitch = sw;  // in pixels (16-bit)
    if (t.lock_style == LockStyle::PointerNoArgs) {
        pixels = mem::vfunc<void*(__fastcall*)(void*, void*)>(d.p, t.lock_vt)(d.p, nullptr);
    } else {
        auto lock = mem::vfunc<int(__fastcall*)(void*, void*, void**, int*)>(d.p, t.lock_vt);
        if (!lock(d.p, nullptr, &pixels, &pitch)) return;
    }
    if (pixels && pitch >= sw) {
        auto* row = static_cast<std::uint8_t*>(pixels) + (static_cast<std::size_t>(y) * pitch + x) * 2;
        for (int i = 0; i < h; i++, row += static_cast<std::size_t>(pitch) * 2)
            std::memset(row, 0, static_cast<std::size_t>(w) * 2);
    }
    if (pixels || t.lock_style != LockStyle::PointerNoArgs) unlock(d.p, nullptr);
}

void clear_screen()
{
    clear_rect(0, 0, 4096, 4096);
}

namespace {

SetResFn g_setres_orig;
SetResFn g_lowmode_orig;

void ensure_mode(int w, int h, int bpp)
{
    // The display class only accepts modes DirectDraw enumerated. Add ours to
    // the list; the DirectDraw wrapper has to accept the size itself.
    const Target& t = target();
    int* list = &mem::at<int>(t.mode_list);
    int& count = mem::at<int>(t.mode_count);
    if (count < 0 || count >= t.mode_max) return;
    for (int i = 0; i < count; i++)
        if (list[i * 3] == w && list[i * 3 + 1] == h && list[i * 3 + 2] == bpp) return;
    list[count * 3] = w;
    list[count * 3 + 1] = h;
    list[count * 3 + 2] = bpp;
    ++count;
    logging::line("added %dx%dx%d to the display mode list (%d modes)", w, h, bpp, count);
}

// The startup call when COMANDO.CFG has no .SIZE yet (asks for -1 x -1).
bool is_first_start(int w, int h, addr_t ret)
{
    return (w == -1 || h == -1) && ret == target().startup_ret;
}

struct Size {
    int w, h;
    const char* why;  // for the log
};

// The size to use when the game asks for w x h. `ret` tells who is asking.
Size choose_size(int w, int h, addr_t ret)
{
    const Config& c = cfg();
    const Target& t = target();

    if (is_first_start(w, h, ret)) return { c.width, c.height, "game (no saved size)" };
    if (!is_std_mode(w, h)) return { w, h, "passthrough" };  // e.g. our own size coming back
    // Credits and movies are drawn for their own size: keep it (the wrapper scales them).
    if (t.credits.contains(ret)) return { w, h, "credits" };
    if (t.video.contains(ret)) return { w, h, "movie" };
    return { c.width, c.height, "game" };
}

// Engine SetResolution (Target::set_res): the game's four fixed sizes become the custom size.
int __fastcall hook_setres(void* self, void* edx, int w, int h, int bpp)
{
    const auto ret = static_cast<addr_t>(C1QOL_RETURN_ADDRESS());
    // Callers pass 16-bit values; the upper halves can be garbage.
    w = static_cast<short>(w);
    h = static_cast<short>(h);
    bpp = static_cast<short>(bpp);

    const Size s = choose_size(w, h, ret);
    if (bpp == 0 || is_first_start(w, h, ret)) bpp = 16;
    ensure_mode(s.w, s.h, bpp);
    logging::line("SetRes %dx%dx%d from %08lX -> %dx%d (%s)", w, h, bpp,
                  static_cast<unsigned long>(ret), s.w, s.h, s.why);
    const int r = g_setres_orig(self, edx, s.w, s.h, bpp);
    clear_screen();  // don't carry the previous screen's pixels into the new size
    return r;
}

// Low-level mode set; detoured only for logging (Log=1).
int __fastcall hook_lowmode(void* self, void* edx, int w, int h, int bpp)
{
    const auto ret = C1QOL_RETURN_ADDRESS();
    const int r = g_lowmode_orig(self, edx, w, h, bpp);
    Display d = Display::get();
    logging::line("  mode set %dx%dx%d from %08lX -> result %d, display now %dx%d",
                  static_cast<short>(w), static_cast<short>(h), static_cast<short>(bpp),
                  static_cast<unsigned long>(ret), r, d ? d.width() : -1, d ? d.height() : -1);
    return r;
}

}  // namespace

namespace resolution {

bool install()
{
    const Target& t = target();
    if (!install_hook("SetResolution", t.set_res, &hook_setres, &g_setres_orig)) {
        logging::line("not patching");
        return false;
    }
    if (logging::enabled()) install_hook("low-level mode set", t.low_mode, &hook_lowmode, &g_lowmode_orig);

    // Startup skips SetResolution when COMANDO.CFG has no .SIZE (-1): NOP both `je`.
    static const std::uint8_t je1[] = { 0x74, 0x10 }, je2[] = { 0x74, 0x0B };
    if (t.skip_size_je1 && t.skip_size_je2 &&
        mem::bytes_equal(t.skip_size_je1, je1) && mem::bytes_equal(t.skip_size_je2, je2)) {
        mem::write<std::uint16_t>(t.skip_size_je1, 0x9090);
        mem::write<std::uint16_t>(t.skip_size_je2, 0x9090);
    }
    return true;
}

}  // namespace resolution
