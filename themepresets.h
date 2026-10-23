#pragma once

struct ThemePreset {
    const char *name;
    const char *accent;
    const char *background; // empty = system palette
    const char *text;       // empty = auto from background
};

static const ThemePreset THEME_PRESETS[] = {
    {"System Default (XETAL Green)", "#27ae60", "",        ""},
    {"XETAL Dark",                   "#27ae60", "#1d1f24", ""},
    {"XETAL Red",                    "#c0392b", "#201a1a", ""},
    {"Midnight Blue",                "#3498db", "#0f1626", ""},
    {"Matrix",                       "#00cc44", "#0a0f0a", "#9dffb0"},
    {"Solar Light",                  "#e67e22", "#f5f1e8", ""},
    {"Purple Haze",                  "#9b59b6", "#191221", ""},
    {"Custom",                       "",        "",        ""},
};
static const int THEME_PRESET_COUNT = sizeof(THEME_PRESETS) / sizeof(THEME_PRESETS[0]);
