// Resolution-specific art: UI pack (NNNxNNN.WAD) and main-menu background (MENUnnn.BMP).
#include "game.hpp"
#include "config.hpp"
#include "log.hpp"

using namespace game;

namespace {

// Replaces sprintf(buf, "%s%ldX%ld.WAD", dir, w, h): the largest pack that fits the screen.
int __cdecl hook_wad_sprintf(char* buf, const char* fmt, const char* dir, int w, int h)
{
    const int i = largest_mode_within(w, h);
    const int pw = kModeW[i], ph = kModeH[i];
    logging::line("UI pack for %dx%d -> %dX%d.WAD", w, h, pw, ph);
    state::ui_pack_w = pw;
    return mem::fn<SprintfFn>(target().sprintf_fn)(buf, fmt, dir, pw, ph);
}

// Replaces sprintf(buf, "MENU%ld.BMP", width): only 512/640/800/1024 exist.
int __cdecl hook_menu_sprintf(char* buf, const char* fmt, int w)
{
    Display d = Display::get();
    const int h = d ? d.height() : 480;
    const int pick = largest_mode_within(w, h);
    logging::line("menu background for %dx%d -> MENU%d.BMP", w, h, kModeW[pick]);
    return mem::fn<SprintfFn>(target().sprintf_fn)(buf, fmt, kModeW[pick]);
}

// The main menu screen.
struct Menu : GameObject {
    Tile* background() const { return field<Tile*>(0x34); }
    int snapshot() const     { return field<int>(0xB8); }  // != 0: dimmed in-game picture instead
};

// Draws the menu background centred, clearing the screen around it.
void draw_menu_bg_centred(Display d, const Tile* tile)
{
    const int sw = d.width(), sh = d.height(), bw = tile->width(), bh = tile->height();
    if (bw < sw || bh < sh) clear_screen();
    blit_tile(tile, sw > bw ? (sw - bw) / 2 : 0, sh > bh ? (sh - bh) / 2 : 0, bw, bh, tile->source());
}

ThisFn g_menu_bg_orig;

// Main-menu background draw (BEL FUN_005A15E0, BCD FUN_0054BAF0). It draws the
// background at 0,0, so one smaller than the screen sat in the corner and the
// rest kept the last game frame (e.g. after leaving a multiplayer game). We
// clear the screen and draw it centred instead. The in-game snapshot path is
// left to the game.
void __fastcall hook_menu_bg(Menu* self, void* edx)
{
    Display d = Display::get();
    const Tile* tile = self->background();
    if (d && tile && !self->snapshot() && (tile->width() < d.width() || tile->height() < d.height())) {
        draw_menu_bg_centred(d, tile);
        return;
    }
    g_menu_bg_orig(self, edx);
}

// Same, for builds where the background draw is inlined into the menu frame
// (1999 BCD): replaces the whole block, so it also does the snapshot path.
void __fastcall hook_menu_bg_inline(Menu* self, void*)
{
    Display d = Display::get();
    if (!d) return;
    if (const int snapshot = self->snapshot()) {
        mem::vfunc<void(__fastcall*)(void*, void*, int)>(d.p, target().menu_snapshot_vt)(d.p, nullptr, snapshot);
        return;
    }
    if (const Tile* tile = self->background()) draw_menu_bg_centred(d, tile);
}

ThisFn g_menu_init_orig;

// Main menu setup (1998 BEL): that build never applies a size at startup. After
// the first menu is set up, run its own "640x480" resolution command once, as if
// the player picked it in the options: the SetResolution hook turns that into
// the custom size and the game rebuilds the menu (and its cursor) by itself.
// (Switching before the menu was built left the cursor invisible.)
void __fastcall hook_menu_init(void* self, void* edx)
{
    g_menu_init_orig(self, edx);
    static bool switched = false;
    const Target& t = target();
    if (switched || !t.menu_command) return;
    switched = true;
    Display d = Display::get();
    const Config& c = cfg();
    if (!d || (d.width() == c.width && d.height() == c.height)) return;
    logging::line("startup: applying the custom size through the menu's resolution command");
    mem::fn<void(__fastcall*)(void*, void*, int)>(t.menu_command)(self, nullptr, t.menu_command_640);
}

}  // namespace

namespace assets {

void install()
{
    const Target& t = target();
    for (const addr_t call : t.wad_calls)
        install_hook("UI pack file name", { HookSite::CallSite, call, t.sprintf_fn }, &hook_wad_sprintf);
    install_hook("menu background file name", { HookSite::CallSite, t.menu_bmp_call, t.sprintf_fn }, &hook_menu_sprintf);
    if (t.menu_init.kind != HookSite::None)
        install_hook("main menu setup", t.menu_init, &hook_menu_init, &g_menu_init_orig);
    if (t.menu_bg.kind == HookSite::InlineEsi) {
        if (t.menu_snapshot_vt) install_hook("menu background", t.menu_bg, &hook_menu_bg_inline);
    } else {
        install_hook("menu background", t.menu_bg, &hook_menu_bg, &g_menu_bg_orig);
    }
}

}  // namespace assets
