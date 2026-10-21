// Addresses for the supported executables. Found with Ghidra; see docs/ADDRESSES.md.
#include "game.hpp"

namespace game {
namespace {

using namespace mem;  // thunk, call_site, vtable_slot, detour, replace, inline_esi

// ---------------------------------------------------------------- BEL
// Commandos: Behind Enemy Lines, Steam "Legacy" and GOG (same file).
// PE timestamp 0x4A9FC6C5, 3,715,072 bytes:
//   md5 2debb1ed8e12df01ac0f75796dcd5835  Steam commandos.exe, GOG comandos.exe / comandos_w10.exe
const std::uint8_t kBelLowModePrologue[] = { 0x6A, 0xFF, 0x68, 0xF6, 0xA1, 0x6C, 0x00 };
const std::uint8_t kCamClampPrologue[]   = { 0x83, 0xEC, 0x28, 0x57, 0x8B, 0xF9 };  // BEL and BCD
const std::uint8_t kZoomSetPrologue[]    = { 0xD9, 0xEE, 0x81, 0xEC, 0x84, 0x00, 0x00, 0x00 };  // BEL and BCD

const Target kBel = {
    .set_res = thunk(0x00407FEA, 0x0059B260),
    .credits = { 0x00477E00, 0x00478180 },
    .video = { 0x005B1FA0, 0x005B2350 },          // ClassModoVideo start
    .startup_ret = 0x0059BA07,
    .skip_size_je1 = 0x0059B9F5, .skip_size_je2 = 0x0059B9FA,
    .low_mode = detour(0x0062E9E0, kBelLowModePrologue),
    .mode_list = 0x007D1588, .mode_count = 0x007D2A28, .mode_max = 440,
    .display_ptr = 0x007B72E4, .disp_w = 0x2C, .disp_h = 0x30,
    .lock_style = LockStyle::PointerNoArgs, .lock_vt = 0x2C, .unlock_vt = 0x34,
    .blitter_ptr = 0x007B72EC,
    .map_ptr = 0x007B72F0,
    .sprintf_fn = 0x00651689,
    .wad_calls = { 0x00599661, 0x0059B30D },
    .menu_bmp_call = 0x005A5277,
    .menu_bg = thunk(0x0040DC79, 0x005A15E0),
    .cam_clamp = detour(0x0061BD30, kCamClampPrologue),
    .zoom_set = detour(0x0061BF30, kZoomSetPrologue),
    .map_draw = thunk(0x004091EC, 0x0059E0D0),
    .bar_pos = thunk(0x00409944, 0x0060AB20),
    .bar_draw = thunk(0x00409F0C, 0x00609BC0),
    .bar_hit = thunk(0x0040AD85, 0x00606690),
    .side_vtable = 0x006FB6C4, .side_draw_fn = 0x0040CF81,  // thunk
    .cursor_redraw = call_site(0x00637796, 0x00637390),
    .present = vtable_slot(0x006FC7CC, 0x006371B0),
};

// ---------------------------------------------------------------- BCD
// Commandos: Beyond the Call of Duty, Steam "Legacy" and GOG coman_mp.exe
// (same build, same addresses), PE timestamp 0x4AA9066E:
//   Steam: 3,536,400 bytes, md5 5a7699e054f503aa4e0a170c806aa6cd. Its .text is
//          encrypted on disk and decrypted before the game's entry point.
//   GOG:   3,133,440 bytes, md5 433467d6dfee506ce8c16ae89cd1c2fa, not encrypted.
// No incremental-link thunks here, so hooks go on call sites, vtables or prologues.
const std::uint8_t kBcdSetResPrologue[]  = { 0x6A, 0xFF, 0x68, 0xFB, 0x82, 0x66, 0x00 };
const std::uint8_t kBcdLowModePrologue[] = { 0x6A, 0xFF, 0x68, 0xB6, 0x6F, 0x67, 0x00 };

const Target kBcd = {
    .set_res = detour(0x0051F400, kBcdSetResPrologue),
    .credits = { 0x004195B0, 0x00419660 },        // 640x480
    .video = { 0x00556430, 0x00556580 },          // ClassModoVideo start
    .startup_ret = 0x0052080B,
    .skip_size_je1 = 0x005207F9, .skip_size_je2 = 0x005207FE,
    .low_mode = detour(0x00618350, kBcdLowModePrologue),
    .mode_list = 0x007308A8, .mode_count = 0x00730D5C, .mode_max = 100,
    .display_ptr = 0x00731034, .disp_w = 0x20, .disp_h = 0x24,
    .lock_style = LockStyle::PtrAndPitch, .lock_vt = 0x3C, .unlock_vt = 0x40,
    .blitter_ptr = 0x0073103C,
    .map_ptr = 0x00731040,
    .sprintf_fn = 0x0062B2B4,
    .wad_calls = { 0x0051F4E6, 0x005249DE },
    .menu_bmp_call = 0x0054EE62,
    .menu_bg = call_site(0x0054F00F, 0x0054BAF0),
    .cam_clamp = detour(0x00613090, kCamClampPrologue),
    .zoom_set = detour(0x00613290, kZoomSetPrologue),
    .map_draw = vtable_slot(0x0068EC58, 0x0052ABA0),  // map window vtable +0x3C
    .bar_pos = call_site(0x005B7CC2, 0x005F8520),
    .bar_draw = vtable_slot(0x0069DE10, 0x005FA7E0),  // ClassComandoMenuArriba +0x3C
    .bar_hit = vtable_slot(0x0069DE24, 0x005F79F0),   // ClassComandoMenuArriba +0x50
    .side_vtable = 0x0069DE2C, .side_draw_fn = 0x005F7D00,
    .cursor_redraw = call_site(0x0057A302, 0x00579EF0),
    .present = vtable_slot(0x006918D4, 0x00579C90),
    .cursor_redraw_gate = 0x68,  // cursor move 0x57A2FA: cmp [ecx+0x68], 0
};

// ---------------------------------------------------------------- BCD 1999
// Commandos: Beyond the Call of Duty, original 1999 retail coman_mp.exe
// 2,968,576 bytes, md5 dd3590205569aadef2a2fad661c6cc81, PE timestamp 0x36F83B67.
// MSVC 5, no incremental-link thunks; the menu background draw is inlined.
const std::uint8_t kBcd99SetResPrologue[]  = { 0xA1, 0xC0, 0x93, 0x6A, 0x00,   // mov eax, [0x6A93C0]
                                               0x81, 0xEC, 0x10, 0x04, 0x00, 0x00 };
const std::uint8_t kBcd99LowModePrologue[] = { 0x64, 0xA1, 0x00, 0x00, 0x00, 0x00 };  // mov eax, fs:[0]
const std::uint8_t kBcd99CamClampPrologue[] = { 0x83, 0xEC, 0x24, 0x56, 0x8B, 0xF1 };
const std::uint8_t kBcd99ZoomSetPrologue[]  = { 0x81, 0xEC, 0x88, 0x00, 0x00, 0x00 };
// mov eax, [esi+0xB8] ; cmp eax, ebx ; je +0x0E ; mov ecx, [0x6E1038]
const std::uint8_t kBcd99MenuBgBlock[] = { 0x8B, 0x86, 0xB8, 0x00, 0x00, 0x00, 0x3B, 0xC3, 0x74, 0x0E,
                                           0x8B, 0x0D, 0x38, 0x10, 0x6E, 0x00 };

const Target kBcd99 = {
    .set_res = detour(0x005404D0, kBcd99SetResPrologue),
    .credits = { 0x004DAB90, 0x004DAC40 },        // 640x480
    .video = { 0x005729F0, 0x00572AD0 },
    .startup_ret = 0x0053E65D,
    .skip_size_je1 = 0x0053E64B, .skip_size_je2 = 0x0053E650,
    .low_mode = detour(0x006453D0, kBcd99LowModePrologue),
    .mode_list = 0x006E0438, .mode_count = 0x006E08EC, .mode_max = 100,
    .display_ptr = 0x006E1038, .disp_w = 0x20, .disp_h = 0x24,
    .lock_style = LockStyle::PtrAndPitch, .lock_vt = 0x38, .unlock_vt = 0x3C,
    .blitter_ptr = 0x006E1040,
    .map_ptr = 0x006E1044,
    .sprintf_fn = 0x0065BD00,
    .wad_calls = { 0x0053B80C, 0x00540580 },
    .menu_bmp_call = 0x0056947F,
    .menu_bg = inline_esi(0x00568DA7, 0x00568E13, kBcd99MenuBgBlock),
    .cam_clamp = detour(0x00646C10, kBcd99CamClampPrologue),
    .zoom_set = detour(0x00646F60, kBcd99ZoomSetPrologue),
    .map_draw = vtable_slot(0x0067328C, 0x00547A30),
    .bar_pos = call_site(0x005DE542, 0x00630CD0),
    .bar_draw = vtable_slot(0x0067AD34, 0x0062ED70),
    .bar_hit = vtable_slot(0x0067AD48, 0x0062EC50),
    .menu_snapshot_vt = 0x40,
    .side_vtable = 0x0067AD50, .side_draw_fn = 0x0062FB80,
};

// ---------------------------------------------------------------- BEL 1998
// Commandos: Behind Enemy Lines, original 1998 retail comandos.exe
// 2,469,376 bytes, md5 b2d82494effbabf2d4ef96aa69d0bd79, PE timestamp 0x3572D0D3.
// Old display class (vtable lock 0x24 / unlock 0x2C); no startup SetResolution
// from COMANDO.CFG (the display starts at 640x480).
const std::uint8_t kBel98SetResPrologue[]  = { 0x81, 0xEC, 0xC8, 0x03, 0x00, 0x00,   // sub esp, 0x3C8
                                               0xA1, 0x44, 0xD3, 0x60, 0x00 };       // mov eax, [display]
const std::uint8_t kBel98LowModePrologue[] = { 0x83, 0xEC, 0x04, 0xA1, 0x48, 0xD3, 0x60, 0x00 };
const std::uint8_t kBel98CamClampPrologue[] = { 0x83, 0xEC, 0x1C, 0x53, 0x56, 0x8B, 0x01 };
const std::uint8_t kBel98ZoomSetPrologue[]  = { 0x8B, 0x44, 0x24, 0x04, 0x81, 0xEC, 0x8C, 0x00, 0x00, 0x00 };

const Target kBel98 = {
    .set_res = detour(0x0044C140, kBel98SetResPrologue),
    .credits = { 0x005089F0, 0x00508D30 },        // 800x600
    .video = { 0x004295F0, 0x004296D0 },
    // no startup_ret / skip_size_je: the menu's own 640x480 command is used (menu_init)
    .low_mode = detour(0x005A5950, kBel98LowModePrologue),
    .mode_list = 0x0065F140, .mode_count = 0x0065F038, .mode_max = 100,
    .display_ptr = 0x0060D344, .disp_w = 0x2C, .disp_h = 0x30,
    .lock_style = LockStyle::PointerNoArgs, .lock_vt = 0x24, .unlock_vt = 0x2C,
    .blitter_ptr = 0x0060D34C,
    .map_ptr = 0x0060D350,
    .sprintf_fn = 0x005CEEE0,
    .wad_calls = { 0x0044629B, 0x0044C1BF },
    .menu_bmp_call = 0x004A6958,
    .menu_bg = call_site(0x004A62F3, 0x004A65A0),
    .cam_clamp = detour(0x00599500, kBel98CamClampPrologue),
    .zoom_set = detour(0x00599950, kBel98ZoomSetPrologue),
    .map_draw = vtable_slot(0x005EB3B4, 0x0054A9F0),
    .bar_pos = call_site(0x004BE9B0, 0x00494030),
    .bar_draw = vtable_slot(0x005E6804, 0x00492120),
    .bar_hit = vtable_slot(0x005E6818, 0x00492020),
    .menu_init = call_site(0x004A62DE, 0x004A6650),
    .menu_command = 0x004A6D10, .menu_command_640 = 0x25,
    .side_vtable = 0x005E6820, .side_draw_fn = 0x00492E90,
};

// TODO: add other exe builds here.

const Target* g_target = nullptr;

}  // namespace

const Target* select_target(Build build)
{
    switch (build) {
    case Build::Bel:   g_target = &kBel; break;
    case Build::Bcd:   g_target = &kBcd; break;
    case Build::Bcd99: g_target = &kBcd99; break;
    case Build::Bel98: g_target = &kBel98; break;
    default:           g_target = nullptr; break;
    }
    return g_target;
}

const Target& target() { return *g_target; }

void blit_tile(const Tile* tile, int x, int y, int w, int h, int* source)
{
    auto* blitter = mem::global_ptr(target().blitter_ptr);
    if (!blitter || !tile->image()) return;
    int rect[4] = { x, y, w, h };
    mem::vfunc<BlitFn>(blitter, 0x18)(blitter, nullptr, tile->image(), rect, source,
                                      tile->palette() + mem::field<int>(blitter, 0x24),  // + blitter palette base
                                      tile->flags(), 0, 0);
}

}  // namespace game
