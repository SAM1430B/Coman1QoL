# Game addresses

Found with Ghidra. The code that uses them is in `Common/game_build.cpp` (which build)
and `ComanResolution/targets.cpp`.

## Builds

| Build | File | Size | md5 | PE timestamp |
|---|---|---|---|---|
| BEL Steam/GOG | Steam `commandos.exe`, GOG `comandos.exe` / `comandos_w10.exe` | 3,715,072 | `2debb1ed8e12df01ac0f75796dcd5835` | `0x4A9FC6C5` |
| BCD Steam | `coman_mp.exe` (`.text` encrypted) | 3,536,400 | `5a7699e054f503aa4e0a170c806aa6cd` | `0x4AA9066E` |
| BCD GOG | `coman_mp.exe` | 3,133,440 | `433467d6dfee506ce8c16ae89cd1c2fa` | `0x4AA9066E` |
| BCD 1999 retail | `coman_mp.exe` | 2,968,576 | `dd3590205569aadef2a2fad661c6cc81` | `0x36F83B67` |
| BEL 1998 retail | `comandos.exe` | 2,469,376 | `b2d82494effbabf2d4ef96aa69d0bd79` | `0x3572D0D3` |

## Addresses

thunk / call = the `jmp` / `call` there is retargeted, vtable = the slot is replaced,
detour = `jmp` over the listed prologue bytes, by ret = SetResolution caller
recognised by its return address.

| What | BEL Steam/GOG | BCD Steam/GOG | BCD 1999 | BEL 1998 |
|---|---|---|---|---|
| build check: `%s%ldX%ld.WAD` string | 0x6F38B4 | 0x68E598 | 0x6889C4 | 0x5F0B08 |
| SetResolution | thunk 0x407FEA → 0x59B260 | detour 0x51F400 `6A FF 68 FB 82 66 00` | detour 0x5404D0 `A1 C0 93 6A 00 81 EC 10 04 00 00` | detour 0x44C140 `81 EC C8 03 00 00 A1 44 D3 60 00` |
| credits (by ret) | 0x477E00..0x478180 (800x600) | 0x4195B0..0x419660 (640x480) | 0x4DAB90..0x4DAC40 (640x480) | 0x5089F0..0x508D30 (800x600) |
| movie player (by ret) | 0x5B1FA0..0x5B2350 | 0x556430..0x556580 | 0x5729F0..0x572AD0 | 0x4295F0..0x4296D0 |
| startup ret, `je` pair (NOP) | 0x59BA07, 0x59B9F5 / 0x59B9FA | 0x52080B, 0x5207F9 / 0x5207FE | 0x53E65D, 0x53E64B / 0x53E650 | none |
| low-level mode set | detour 0x62E9E0 `6A FF 68 F6 A1 6C 00` | detour 0x618350 `6A FF 68 B6 6F 67 00` | detour 0x6453D0 `64 A1 00 00 00 00` | detour 0x5A5950 `83 EC 04 A1 48 D3 60 00` |
| mode list / count (max) | 0x7D1588 / 0x7D2A28 (440) | 0x7308A8 / 0x730D5C (100) | 0x6E0438 / 0x6E08EC (100) | 0x65F140 / 0x65F038 (100) |
| display, w / h | [0x7B72E4], +0x2C / +0x30 | [0x731034], +0x20 / +0x24 | [0x6E1038], +0x20 / +0x24 | [0x60D344], +0x2C / +0x30 |
| display lock / unlock vt | 0x2C / 0x34 | 0x3C / 0x40 | 0x38 / 0x3C (snapshot 0x40) | 0x24 / 0x2C |
| blitter / map | [0x7B72EC] / [0x7B72F0] | [0x73103C] / [0x731040] | [0x6E1040] / [0x6E1044] | [0x60D34C] / [0x60D350] |
| sprintf; WAD calls; MENU call | 0x651689; 0x599661, 0x59B30D; 0x5A5277 | 0x62B2B4; 0x51F4E6, 0x5249DE; 0x54EE62 | 0x65BD00; 0x53B80C, 0x540580; 0x56947F | 0x5CEEE0; 0x44629B, 0x44C1BF; 0x4A6958 |
| menu background | thunk 0x40DC79 → 0x5A15E0 | call 0x54F00F → 0x54BAF0 | inlined at 0x568DA7..0x568E13 (block replaced) | call 0x4A62F3 → 0x4A65A0 |
| camera clamp | detour 0x61BD30 `83 EC 28 57 8B F9` | detour 0x613090 (same bytes) | detour 0x646C10 `83 EC 24 56 8B F1` | detour 0x599500 `83 EC 1C 53 56 8B 01` |
| camera zoom setter | detour 0x61BF30 `D9 EE 81 EC 84 00 00 00` | detour 0x613290 (same bytes) | detour 0x646F60 `81 EC 88 00 00 00` | detour 0x599950 `8B 44 24 04 81 EC 8C 00 00 00` |
| map window draw | thunk 0x4091EC → 0x59E0D0 | vtable 0x68EC58 → 0x52ABA0 | vtable 0x67328C → 0x547A30 | vtable 0x5EB3B4 → 0x54A9F0 |
| top bar position | thunk 0x409944 → 0x60AB20 | call 0x5B7CC2 → 0x5F8520 | call 0x5DE542 → 0x630CD0 | call 0x4BE9B0 → 0x494030 |
| top bar draw | thunk 0x409F0C → 0x609BC0 | vtable 0x69DE10 → 0x5FA7E0 | vtable 0x67AD34 → 0x62ED70 | vtable 0x5E6804 → 0x492120 |
| top bar hit test | thunk 0x40AD85 → 0x606690 | vtable 0x69DE24 → 0x5F79F0 | vtable 0x67AD48 → 0x62EC50 | vtable 0x5E6818 → 0x492020 |
| side bar vtable; draw (+0x3C) | 0x6FB6C4; thunk 0x40CF81 → 0x609F40 | 0x69DE2C; 0x5F7D00 | 0x67AD50; 0x62FB80 | 0x5E6820; 0x492E90 |
| cursor redraw on mouse move | call 0x637796 → 0x637390 | call 0x57A302 → 0x579EF0 | | |
| frame present (window manager) | vtable 0x6FC7CC → 0x6371B0 | vtable 0x6918D4 → 0x579C90 | | |
| main menu setup / command | | | | call 0x4A62DE → 0x4A6650 / 0x4A6D10 (code 0x25 = 640x480) |
