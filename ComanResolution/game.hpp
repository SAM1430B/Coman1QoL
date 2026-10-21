// What we need to know about a supported game executable (see targets.cpp and
// docs/ADDRESSES.md):
//   - Commandos: Behind Enemy Lines, Steam commandos.exe / GOG comandos.exe and 1998 comandos.exe
//   - Commandos: Beyond the Call of Duty, Steam/GOG coman_mp.exe and 1999 coman_mp.exe
#pragma once
#include "game_build.hpp"
#include "hooks.hpp"
#include "memory.hpp"

namespace game {

using mem::addr_t;
using mem::HookSite;

struct Range {
    addr_t begin = 0, end = 0;
    bool contains(addr_t a) const { return a >= begin && a < end; }
};

enum class LockStyle {
    PointerNoArgs,   // void* lock()                      (BEL)
    PtrAndPitch,     // bool lock(void** p, int* pitch_px) (BCD)
};

struct Target {
    // Engine SetResolution: thiscall(w, h, bpp), ret 0xC, ecx = engine.
    HookSite set_res{};
    // Callers of SetResolution, told apart by return address.
    Range credits{};                 // forces its own size (800x600 / 640x480)
    Range video{};                   // movie player (ClassModoVideo) forces 640x480
    addr_t startup_ret = 0;          // return address of the config/startup call
    addr_t skip_size_je1 = 0;        // `je` pair skipping it when COMANDO.CFG has no .SIZE
    addr_t skip_size_je2 = 0;

    HookSite low_mode{};             // low-level mode set (logging only)

    addr_t mode_list = 0;            // display modes DirectDraw enumerated: {w, h, bpp}[mode_max]
    addr_t mode_count = 0;
    int mode_max = 0;

    addr_t display_ptr = 0;          // global holding the display object
    int disp_w = 0, disp_h = 0;      // offsets of width / height in it
    LockStyle lock_style = LockStyle::PointerNoArgs;
    int lock_vt = 0, unlock_vt = 0;  // vtable byte offsets

    addr_t blitter_ptr = 0;          // global blitter, vtable +0x18 = blit
    addr_t map_ptr = 0;              // current mission map

    addr_t sprintf_fn = 0;
    addr_t wad_calls[2] = {};        // sprintf("%s%ldX%ld.WAD", dir, w, h)
    addr_t menu_bmp_call = 0;        // sprintf("MENU%ld.BMP", w)
    HookSite menu_bg{};              // main-menu background draw, thiscall(), tile at +0x34

    HookSite cam_clamp{};            // camera clamp, thiscall() on the camera
    HookSite zoom_set{};             // camera zoom setter, thiscall(float zoom) on the camera
    HookSite map_draw{};             // map window draw: thiscall(ctx), rect ptr at +8

    HookSite bar_pos{};              // places the right-hand top bar group: thiscall(mode, x)
    HookSite bar_draw{};             // BARRA01 window draw: thiscall(ctx)
    HookSite bar_hit{};              // BARRA01 window hit test: thiscall(int* pt)

    // Only for an inlined menu background (menu_bg.kind == InlineEsi): display
    // vtable offset that shows the dimmed in-game snapshot, thiscall(snapshot).
    int menu_snapshot_vt = 0;

    // Only for builds with no startup SetResolution (1998 BEL): main menu setup,
    // thiscall(), and the menu's command handler thiscall(code) with the code of
    // its "640x480" option; run once after the first menu setup.
    HookSite menu_init{};
    addr_t menu_command = 0;
    int menu_command_640 = 0;

    // Side bar (vertical leather strip) window class: its vtable and the draw
    // function in slot +0x3C (DrawFn). Rect ptr at +8, images +0x90 (top) and
    // +0x94 (below it).
    addr_t side_vtable = 0;
    addr_t side_draw_fn = 0;

    // Mouse cursor: the call in the cursor-move function that redraws the
    // cursor straight onto the screen, thiscall(1); and the window manager's
    // frame present, thiscall(), which shows the frame and the cursor.
    HookSite cursor_redraw{};
    HookSite present{};
    // BCD only: the cursor-move function redraws only while this int field of
    // the window manager is non-zero (present sets it around its own cursor
    // draw + flip, then clears it). 0 = no such gate (BEL).
    int cursor_redraw_gate = 0;
};

// Picks the addresses for `build`; nullptr if there are none.
const Target* select_target(Build build);
const Target& target();  // valid after select_target() succeeded

// The display object.
struct Display {
    std::uint8_t* p;
    static Display get() { return { mem::global_ptr(target().display_ptr) }; }
    explicit operator bool() const { return p != nullptr; }
    int width() const  { return mem::field<int>(p, target().disp_w); }
    int height() const { return mem::field<int>(p, target().disp_h); }
};

// ---- Game objects ---------------------------------------------------------
// Views of the game's own objects: a pointer into game memory is used as one of
// these, and each accessor reads the field at its offset (found with Ghidra).

// Base for the views: field<T>(offset) is the T at that byte offset.
struct GameObject {
    template <typename T> T& field(std::size_t off) const { return mem::field<T>(this, off); }
};

struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};

// An image (ClassTile, 0x54 bytes).
struct Tile : GameObject {
    void*& image() const   { return field<void*>(0x08); }
    int& flags() const     { return field<int>(0x14); }
    int& width() const     { return field<int>(0x20); }
    int& height() const    { return field<int>(0x24); }
    int& palette() const   { return field<int>(0x38); }
    int* source() const    { return &field<int>(0x3C); }  // {x, y}: where in the image drawing starts (wraps)
};

// A window (ClassVentana): everything on screen is one, in a tree.
struct Window : GameObject {
    Rect& rect() const          { return *field<Rect*>(0x08); }  // screen position and size
    bool has_rect() const       { return field<Rect*>(0x08) != nullptr; }
    Window* parent() const      { return field<Window*>(0x14); }
    Window* next_sibling() const { return field<Window*>(0x18); }
    Window* first_child() const { return field<Window*>(0x20); }
    addr_t vtable() const       { return field<addr_t>(0x00); }  // +0x3C draw (DrawFn), +0x50 hit test (HitFn)

    bool has_child(const Window* w) const
    {
        for (Window* c = first_child(); c; c = c->next_sibling())
            if (c == w) return true;
        return false;
    }
};

// The mission map.
struct Map : GameObject {
    static Map* get() { return mem::global_ptr<Map>(target().map_ptr); }
    int& width() const          { return field<int>(0x100); }
    int& height() const         { return field<int>(0x104); }
    const float* matrix() const { return field<float*>(0x140); }  // map -> world, floats at +0x64..+0x70
};

// A camera (the one for the main map view, or others).
struct Camera : GameObject {
    bool has_map() const              { return field<int>(0x00) != 0; }
    int& view_width() const           { return field<int>(0x0C); }
    int& view_height() const          { return field<int>(0x10); }
    const float* projection() const   { return field<float*>(0x14); }  // world -> screen
    // Zoom: 1.0 = fully zoomed out, larger = zoomed in (projection +0x2C).
    float zoom() const                { return projection() ? projection()[11] : 1.0f; }
};

// The game's own screen sizes; each has a UI pack (NNNxNNN.WAD) and a MENUnnn.BMP.
constexpr int kModeCount = 4;
constexpr int kModeW[kModeCount] = { 512, 640, 800, 1024 };
constexpr int kModeH[kModeCount] = { 384, 480, 600, 768 };

inline bool is_std_mode(int w, int h)
{
    for (int i = 0; i < kModeCount; i++)
        if (kModeW[i] == w && kModeH[i] == h) return true;
    return false;
}

// Index of the largest standard size that fits in w x h (0 if none does).
inline int largest_mode_within(int w, int h)
{
    int best = 0;
    for (int i = 0; i < kModeCount; i++)
        if (kModeW[i] <= w && kModeH[i] <= h) best = i;
    return best;
}

using SetResFn = int(__fastcall*)(void* self, void* edx, int w, int h, int bpp);
using ThisFn = void(__fastcall*)(void* self, void* edx);                 // thiscall()
// Window draw, vtable +0x3C.
using DrawFn = int(__fastcall*)(void* self, void* edx, void* ctx);
// Window hit test, vtable +0x50: the window that "has" the point (window-relative
// {x, y}), or nullptr. The game draws the mouse cursor only over such a window.
using HitFn = void*(__fastcall*)(void* self, void* edx, int* pt);
using SprintfFn = int(__cdecl*)(char*, const char*, ...);
// Blitter vtable +0x18: (img, rect{x,y,w,h}, source offset{x,y} (wraps), palette, flags, 0, 0).
using BlitFn = void(__fastcall*)(void* self, void* edx, void* img, int* rect, int* src, int pal,
                                 int flags, int, int);

// Draws `w` x `h` pixels of a tile at x, y with the global blitter, starting at
// `source` {x, y} in the image.
void blit_tile(const Tile* tile, int x, int y, int w, int h, int* source);

}  // namespace game

// State shared between the .cpp files.
namespace state {
inline int ui_pack_w = 640;       // width of the UI art pack currently loaded
inline int bar_shift = 0;         // pixels the right-hand bar group was moved by
}  // namespace state

// Implemented in resolution.cpp, used by the other files.
void clear_screen();
void clear_rect(int x, int y, int w, int h);

// Installers, one per file (each checks the bytes it patches first).
namespace resolution { bool install(); }
namespace assets     { void install(); }
namespace top_bar    { void install(); }
namespace side_bar   { void install(); }
namespace small_maps { void install(); }
namespace cursor     { void install(); }
