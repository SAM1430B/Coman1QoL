// Side bar continued down to the bottom of the screen.
#include "game.hpp"
#include <cstring>

using namespace game;

namespace {

DrawFn g_side_orig;
HitFn g_side_hit_orig;

// Side bar (the vertical leather strip): a window whose art is two images
// stacked, the notepad and the belt, sized for the UI pack. On taller
// screens the strip stops part way down and nothing ever draws in the column
// below it (old pixels stay, e.g. after TAB moves the bar to the other side).
// With the 640 pack there is one image (notepad and belt) and the lower one is empty.
//
// The screen is not repainted every frame: a window repaints everything only
// when its redraw_all flag is set (its draw clears it), otherwise only its
// dirty rect (the area the map just redrew). Windows in front of the bar
// (the bag, the cursor) also repaint only their dirty parts, so drawing the
// extra part ourselves every frame would cover them. Instead we make the
// bar's window reach the bottom of the screen and draw the extra part through
// the bar's own draw function, called again with its two image pointers
// swapped for edited copies (and the redraw_all flag it came in with).
//
// The belt ends with eyelets, the strap's clasp and more eyelets. We keep that
// end at the bottom, like on the native screen size, and fill the gap above it
// by repeating a plain part of the belt (no eyelets). The clasp is wider than
// the strip and transparent around it, so the belt is never drawn in full
// (the clasp would stay visible in the middle).
//
//   belt rows 0..cut        where the game draws them
//   rows sy..cut, repeated  down to the tail
//   rows cut..end (tail)    at the bottom of the screen
//
// Plain rows: lower image 800 pack 0%..50%, 1024 pack 18%..64%; single 640
// image (notepad to 52%) 52%..69%.
// The side bar window.
struct SideBar : Window {
    int& redraw_all() const { return field<int>(0x78); }   // repaint everything, not only the dirty part
    Tile*& upper() const    { return field<Tile*>(0x90); }  // notepad (640 pack: notepad and belt)
    Tile*& lower() const    { return field<Tile*>(0x94); }  // belt (640 pack: empty)
};

// Copies of the bar's images, edited for one part of the column.
alignas(4) std::uint8_t g_spacer_mem[0x80], g_belt_part_mem[0x80];
Tile* const g_spacer = reinterpret_cast<Tile*>(g_spacer_mem);
Tile* const g_belt_part = reinterpret_cast<Tile*>(g_belt_part_mem);
int g_redraw_all;  // redraw_all() as the current draw came in

// A tile that draws nothing and only moves the next image down to `y`.
Tile* spacer(const Tile* top, int y)
{
    std::memcpy(g_spacer, top, sizeof g_spacer_mem);
    g_spacer->width() = 0;
    g_spacer->height() = y;
    return g_spacer;
}

// The belt image cut to `rows` rows starting at row `first`.
Tile* belt_rows(const Tile* belt, int first, int rows)
{
    std::memcpy(g_belt_part, belt, sizeof g_belt_part_mem);
    g_belt_part->height() = rows;
    g_belt_part->source()[1] += first;
    return g_belt_part;
}

// Runs the bar's own draw with its two images replaced by `upper` and `lower`.
int draw_with(SideBar* self, void* edx, void* ctx, Tile* upper, Tile* lower)
{
    Tile* const saved_upper = self->upper();
    Tile* const saved_lower = self->lower();
    self->upper() = upper;
    self->lower() = lower;
    self->redraw_all() = g_redraw_all;
    const int r = g_side_orig(self, edx, ctx);
    self->upper() = saved_upper;
    self->lower() = saved_lower;
    return r;
}

int __fastcall hook_side_draw(SideBar* self, void* edx, void* ctx)
{
    Tile* top = self->upper();
    Tile* low = self->lower();
    Display d = Display::get();
    if (!self->has_rect() || !top || !low || !d) return g_side_orig(self, edx, ctx);
    Rect& r = self->rect();

    // Reach the bottom of the screen.
    const int art_h = top->height() + low->height();
    if (d.height() - r.y > r.h) r.h = d.height() - r.y;

    const bool two_images = low->height() >= 16 && low->image();
    Tile* belt = two_images ? low : top;
    const int belt_h = belt->height();
    if (r.h <= art_h || belt_h < 16) return g_side_orig(self, edx, ctx);

    const int cut = belt_h * (two_images ? 48 : 68) / 100;
    const int sy = belt_h * (two_images ? 20 : 56) / 100;
    const int belt_y = two_images ? top->height() : 0;  // where the belt image starts
    const int tail_y = r.h - (belt_h - cut);

    g_redraw_all = self->redraw_all();
    // The art down to the cut.
    const int result = two_images ? draw_with(self, edx, ctx, top, belt_rows(belt, 0, cut))
                                  : draw_with(self, edx, ctx, belt_rows(belt, 0, cut), low);
    // The plain part, repeated.
    for (int y = belt_y + cut; y < tail_y; y += cut - sy) {
        const int rows = tail_y - y < cut - sy ? tail_y - y : cut - sy;
        draw_with(self, edx, ctx, spacer(top, y), belt_rows(belt, sy, rows));
    }
    // The tail at the bottom.
    draw_with(self, edx, ctx, spacer(top, tail_y), belt_rows(belt, cut, belt_h - cut));
    return result;
}

// The bar's own hit test asks each image whether it is opaque at the point, but
// it passes the lower image the point unshifted, so the lower image (below the
// notepad) never matches and the cursor disappears over it, even at the native
// screen size. Accept points below the top image (the belt and the extra part).
void* __fastcall hook_side_hit(SideBar* self, void* edx, int* pt)
{
    const Tile* top = self->upper();
    if (pt && top && self->has_rect()) {
        const Rect& r = self->rect();
        const Rect below_top = { 0, top->height(), r.w, r.h - top->height() };
        if (below_top.contains(pt[0], pt[1])) return self;
    }
    return g_side_hit_orig(self, edx, pt);
}

}  // namespace

namespace side_bar {

void install()
{
    const Target& t = target();
    if (!t.side_vtable || !t.side_draw_fn) return;
    if (install_hook("side bar draw", mem::vtable_slot(t.side_vtable + 0x3C, t.side_draw_fn),
                     &hook_side_draw, &g_side_orig)) {
        const addr_t hit_slot = t.side_vtable + 0x50;
        install_hook("side bar hit test", mem::vtable_slot(hit_slot, mem::at<addr_t>(hit_slot)),
                     &hook_side_hit, &g_side_hit_orig);
    }
}

}  // namespace side_bar
