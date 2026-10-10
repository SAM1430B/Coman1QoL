#include "game_build.hpp"
#include "memory.hpp"

namespace game {
namespace {

// UI pack file name format, in .rdata (never encrypted): tells the builds apart.
const char kWadFormat[] = "%s%ldX%ld.WAD";
bool has_wad_format(mem::addr_t a) { return mem::bytes_equal(a, kWadFormat, sizeof kWadFormat); }

// BCD's SetResolution prologue: readable once Steam has decrypted the code.
const std::uint8_t kBcdSetResPrologue[] = { 0x6A, 0xFF, 0x68, 0xFB, 0x82, 0x66, 0x00 };

}  // namespace

Build detect_build()
{
    auto* exe = reinterpret_cast<std::uint8_t*>(GetModuleHandleA(nullptr));
    if (reinterpret_cast<mem::addr_t>(exe) != 0x00400000) return Build::Unknown;
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(exe);
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(exe + dos->e_lfanew);
    const DWORD stamp = nt->FileHeader.TimeDateStamp;

    if (stamp == 0x4A9FC6C5 && has_wad_format(0x006F38B4)) return Build::Bel;
    if (stamp == 0x4AA9066E && has_wad_format(0x0068E598)) return Build::Bcd;
    if (stamp == 0x36F83B67 && has_wad_format(0x006889C4)) return Build::Bcd99;
    if (stamp == 0x3572D0D3 && has_wad_format(0x005F0B08)) return Build::Bel98;
    return Build::Unknown;
}

const char* build_name(Build b)
{
    switch (b) {
    case Build::Bel:   return "Commandos: Behind Enemy Lines (Steam/GOG)";
    case Build::Bcd:   return "Commandos: Beyond the Call of Duty (Steam/GOG)";
    case Build::Bcd99: return "Commandos: Beyond the Call of Duty (1999 retail)";
    case Build::Bel98: return "Commandos: Behind Enemy Lines (1998 retail)";
    default:           return "unknown";
    }
}

bool code_ready(Build b)
{
    return b != Build::Bcd || mem::bytes_equal(0x0051F400, kBcdSetResPrologue);
}

}  // namespace game
