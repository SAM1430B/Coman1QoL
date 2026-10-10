// Stretched top bar. The bar is laid out for the UI pack width: BARRA01
// (portraits) on the left, then the right-hand group (dots, help/camera buttons,
// eye) placed after it by FUN_0060AB20. We move that group to the right edge and
// fill the gap with a repeated slice of BARRA01's leather texture.
#include "game.hpp"

using namespace game;

namespace {

void(__fastcall* g_pos_orig)(void* self, void* edx, int mode, int x);
DrawFn g_draw_orig;
HitFn g_hit_orig;

int current_shift()
{
    Display d = Display::get();
    if (!d) return 0;
    const int sw = d.width();
    return sw > state::ui_pack_w ? sw - state::ui_pack_w : 0;
}

// BEL FUN_0060AB20 / BCD FUN_005F8520: places the right-hand group at x (the BARRA01 right edge).
void __fastcall hook_bar_pos(void* self, void* edx, int mode, int x)
{
    state::bar_shift = current_shift();
    g_pos_orig(self, edx, mode, x + state::bar_shift);
}

// The BARRA01 window (portraits).
struct BarWindow : Window {
    Tile* image() const   { return field<Tile*>(0x84); }
    bool dirty() const    { return field<int>(0x88) != 0; }  // needs repainting
    int& cached_width() const { return field<int>(0x94); }   // the window's own copy of rect().w
};

// BARRA01 window draw (BEL FUN_00609BC0, BCD FUN_005FA7E0).
int __fastcall hook_bar_draw(BarWindow* self, void* edx, void* ctx)
{
    const int shift = state::bar_shift;
    Tile* tile = self->image();
    if (shift > 0 && tile && self->has_rect()) {
        Rect& r = self->rect();
        const int tw = tile->width(), th = tile->height();
        // Widen the window over the gap: the game only draws the mouse cursor over windows.
        r.w = tw + shift;
        self->cached_width() = r.w;
        // Fill the gap with a repeated leather slice, 60%..75% of the bar image
        // (between the portraits and the dots).
        if (self->dirty()) {
            const int slice = tw * 15 / 100 < 8 ? 8 : tw * 15 / 100;
            const int x0 = r.x + tw, x1 = x0 + shift;
            for (int x = x0; x < x1; x += slice) {
                int source[2] = { tw * 60 / 100, 0 };
                blit_tile(tile, x, r.y, x1 - x < slice ? x1 - x : slice, th, source);
            }
        }
    }
    // Filler first: the original draw paints the bar and the cursor on top.
    return g_draw_orig(self, edx, ctx);
}

// BARRA01 hit test (BEL FUN_00606690, BCD FUN_005F79F0) checks the point against
// the bar image, so the widened part never "has" the mouse: accept it ourselves.
void* __fastcall hook_bar_hit(BarWindow* self, void* edx, int* pt)
{
    if (const Tile* tile = self->image(); tile && pt && state::bar_shift > 0) {
        const Rect gap = { tile->width(), 0, state::bar_shift, tile->height() };
        if (gap.contains(pt[0], pt[1])) return self;
    }
    return g_hit_orig(self, edx, pt);
}

}  // namespace

namespace top_bar {

void install()
{
    // bar_shift stays 0 unless the position hook runs, so the others are inert without it.
    const Target& t = target();
    install_hook("top bar position", t.bar_pos, &hook_bar_pos, &g_pos_orig);
    install_hook("top bar draw", t.bar_draw, &hook_bar_draw, &g_draw_orig);
    install_hook("top bar hit test", t.bar_hit, &hook_bar_hit, &g_hit_orig);
}

}  // namespace top_bar
