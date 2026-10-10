// [Resolution] in c1m.ini (the size can also come from "-res WxH" on the command line).
#pragma once

struct Config {
    int width = 0, height = 0;  // custom size (at least 640x480, the menus' size)
};

const Config& cfg();
void load_config();
