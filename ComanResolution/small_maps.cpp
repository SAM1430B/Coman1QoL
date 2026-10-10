// Maps smaller than the screen. The game never draws the area past the map's
// edge, so whatever was there before stays on screen and gets smeared around
// when scrolling. We centre such a map in its window and paint the rest black.
#include "game.hpp"

using namespace game;

namespace {

void(__fastcall* g_clamp_orig)(Camera* cam, void* edx);
void(__fastcall* g_zoom_orig)(Camera* cam, void* edx, float zoom);
DrawFn g_draw_orig;
HitFn g_parent_hit_orig;

int g_map_w, g_map_h;           // current mission map size in screen pixels (0 = no map)
Window* g_map_window;           // the main map view
Window* g_map_parent;           // its parent, whose hit test we extend
Window* g_placed;               // the window we last centred
Rect g_game_rect;               // its rect as the game laid it out
Rect g_our_rect;                // as we last set it

// The mission map's size in screen pixels at `zoom`: the map's size through
// its matrix (map -> world) and the camera projection (world -> screen).
bool map_size_on_screen(const Camera* cam, float zoom, int& w, int& h)
{
    const Map* map = Map::get();
    const float* m = map ? map->matrix() : nullptr;
    const float* p = cam->projection();
    if (!m || !p) return false;
    const float W = static_cast<float>(map->width()), H = static_cast<float>(map->height());
    const float x = W * m[0x64 / 4] + H * m[0x68 / 4];
    const float y = W * m[0x6C / 4] + H * m[0x70 / 4];
    w = static_cast<int>((x * p[0] + y * p[1]) * zoom);
    h = static_cast<int>((x * p[3] + y * p[4]) * zoom);
    return true;
}

// Camera clamp (BEL FUN_0061BD30, BCD FUN_00613090): runs whenever the camera
// moves or zooms, so the map size is kept up to date here.
void __fastcall hook_camclamp(Camera* cam, void* edx)
{
    if (!cam || !cam->has_map() || !map_size_on_screen(cam, cam->zoom(), g_map_w, g_map_h))
        g_map_w = g_map_h = 0;
    g_clamp_orig(cam, edx);
}

// Camera zoom setter (BEL FUN_0061BF30, BCD FUN_00613290). Zooming out below
// 1.0 while the map is smaller than the view makes the game jump to the zoom
// where the map fills the view, and the zoom-out animation then runs down to
// 1.0 again, over and over. The map is centred instead (below), so stay at 1.0.
void __fastcall hook_zoom_set(Camera* cam, void* edx, float zoom)
{
    int w, h;
    if (zoom < 1.0f && cam && cam->has_map() && map_size_on_screen(cam, 1.0f, w, h) &&
        (w < cam->view_width() || h < cam->view_height()))
        zoom = 1.0f;
    g_zoom_orig(cam, edx, zoom);
}

// Centres the map along one axis: moves the window's start by half the free
// space and shortens it by the same amount, so the map (drawn from the
// window's start) ends up in the middle.
void centre(int& pos, int& len, int game_pos, int game_len, int map_len)
{
    const int free = map_len > 0 && map_len < game_len ? (game_len - map_len) / 2 : 0;
    pos = game_pos + free;
    len = game_len - free;
}

// Moving the map window leaves the black bars on its left and top outside it,
// so they belong to its parent, whose hit test says no and the cursor is hidden
// there. Points in the map window's original area are given to the map window.
void* __fastcall hook_parent_hit(Window* self, void* edx, int* pt)
{
    if (self == g_map_parent && pt && self->has_child(g_map_window)) {
        const Rect& pr = self->rect();
        if (g_game_rect.contains(pr.x + pt[0], pr.y + pt[1])) return g_map_window;  // screen coordinates
    }
    return g_parent_hit_orig(self, edx, pt);
}

// Remembers the map window and its parent, and hooks the parent's hit test (once).
void track_map_window(Window* map_window)
{
    Window* parent = map_window->parent();
    if (!parent || !parent->has_child(map_window)) return;
    g_map_window = map_window;
    g_map_parent = parent;
    if (g_parent_hit_orig) return;
    const addr_t slot = parent->vtable() + 0x50;
    install_hook("map parent hit test", mem::vtable_slot(slot, mem::at<addr_t>(slot)), &hook_parent_hit,
                 &g_parent_hit_orig);
}

bool same(const Rect& a, const Rect& b) { return a.x == b.x && a.y == b.y && a.w == b.w && a.h == b.h; }

// Map window draw (BEL FUN_0059E0D0, BCD FUN_0052ABA0). The map is redrawn
// completely every frame, so we clear the window's area first and whatever
// the map doesn't cover stays black.
int __fastcall hook_map_draw(Window* self, void* edx, void* ctx)
{
    Display d = Display::get();
    if (!self->has_rect() || !d || !Map::get()) return g_draw_orig(self, edx, ctx);
    Rect& r = self->rect();
    const bool main_view = r.w * 2 >= d.width() && r.h * 2 >= d.height();  // not a small view
    if (!main_view) return g_draw_orig(self, edx, ctx);

    // A different window, or the game laid it out again (e.g. a new screen size): start from its rect.
    if (self != g_placed || !same(r, g_our_rect)) g_game_rect = r;
    g_placed = self;
    track_map_window(self);
    const Rect& g = g_game_rect;
    centre(r.x, r.w, g.x, g.w, g_map_w);
    centre(r.y, r.h, g.y, g.h, g_map_h);
    g_our_rect = r;

    if ((g_map_w > 0 && g.w > g_map_w) || (g_map_h > 0 && g.h > g_map_h)) clear_rect(g.x, g.y, g.w, g.h);
    return g_draw_orig(self, edx, ctx);
}

}  // namespace

namespace small_maps {

void install()
{
    const Target& t = target();
    if (install_hook("camera clamp", t.cam_clamp, &hook_camclamp, &g_clamp_orig) &&
        install_hook("map window draw", t.map_draw, &hook_map_draw, &g_draw_orig))
        install_hook("camera zoom", t.zoom_set, &hook_zoom_set, &g_zoom_orig);
}

}  // namespace small_maps
