#ifndef COLORS_H
#define COLORS_H

#include <SDL.h>

#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace Colors {

// --- Màu “Kinh điển” (Basic) ---
constexpr SDL_Color Red = {255, 0, 0, 255};
constexpr SDL_Color Green = {0, 255, 0, 255};
constexpr SDL_Color Blue = {0, 0, 255, 255};
constexpr SDL_Color Black = {0, 0, 0, 255};
constexpr SDL_Color White = {255, 255, 255, 255};
constexpr SDL_Color Yellow = {255, 255, 0, 255};
constexpr SDL_Color Cyan = {0, 255, 255, 255};
constexpr SDL_Color Magenta = {255, 0, 255, 255};
constexpr SDL_Color Orange = {255, 165, 0, 255};
constexpr SDL_Color Purple = {128, 0, 128, 255};
constexpr SDL_Color Pink = {255, 192, 203, 255};
constexpr SDL_Color Brown = {165, 42, 42, 255};
constexpr SDL_Color Gray = {128, 128, 128, 255};

// --- Bảng Utility (Đen trắng xám mở rộng) ---
constexpr SDL_Color Gray20 = {51, 51, 51, 255};
constexpr SDL_Color Gray50 = {128, 128, 128, 255};
constexpr SDL_Color Gray80 = {204, 204, 204, 255};
constexpr SDL_Color DarkGray = {64, 64, 64, 255};
constexpr SDL_Color LightGray = {192, 192, 192, 255};

// --- Bảng Chill / Lofi (êm dịu, ấm áp) ---
constexpr SDL_Color LofiBeige = {245, 241, 230, 255};
constexpr SDL_Color LofiBrown = {175, 144, 92, 255};
constexpr SDL_Color LofiMint = {200, 228, 187, 255};
constexpr SDL_Color LofiDustyPink = {231, 201, 216, 255};

// --- Bảng Pastel (mơ màng nhẹ nhàng) ---
constexpr SDL_Color PastelPeach = {255, 218, 185, 255};
constexpr SDL_Color PastelLilac = {200, 162, 200, 255};
constexpr SDL_Color PastelAqua = {177, 238, 238, 255};
constexpr SDL_Color PastelLemon = {253, 253, 150, 255};
constexpr SDL_Color PastelGreen = {152, 251, 152, 255};
constexpr SDL_Color PastelSky = {135, 206, 250, 255};

// --- Bảng Đêm (tĩnh lặng huyền bí) ---
constexpr SDL_Color NightSky = {15, 15, 40, 255};
constexpr SDL_Color TwilightPurple = {72, 61, 139, 255};
constexpr SDL_Color MoonGray = {200, 200, 210, 180};
constexpr SDL_Color StarYellow = {255, 255, 180, 200};
constexpr SDL_Color DeepSeaBlue = {0, 48, 73, 255};

// --- Bảng Vaporwave / Retro (hoài cổ neon dịu) ---
constexpr SDL_Color VaporPink = {255, 105, 180, 255};
constexpr SDL_Color VaporTeal = {54, 117, 136, 255};
constexpr SDL_Color VaporPurple = {147, 112, 219, 255};
constexpr SDL_Color VaporNeonGreen = {57, 255, 20, 255};
constexpr SDL_Color VaporOrange = {255, 179, 0, 255};

// --- Bảng Cyberpunk / Neon (tương phản cao) ---
constexpr SDL_Color NeonMagenta = {255, 20, 147, 255};
constexpr SDL_Color NeonCyan = {0, 255, 255, 255};
constexpr SDL_Color NeonYellow = {255, 255, 0, 255};
constexpr SDL_Color NeonOrange = {255, 69, 0, 255};
constexpr SDL_Color NeonGreen = {57, 255, 20, 255};

// --- Bảng Earth Tones (đất đai thiên nhiên) ---
constexpr SDL_Color EarthSand = {210, 180, 140, 255};
constexpr SDL_Color EarthOlive = {128, 128, 0, 255};
constexpr SDL_Color EarthTerracotta = {204, 102, 51, 255};
constexpr SDL_Color EarthForest = {34, 85, 34, 255};
constexpr SDL_Color EarthClay = {193, 154, 107, 255};

// --- Bảng Accent (highlight, shadow) ---
constexpr SDL_Color AccentHotPink = {255, 105, 180, 200};
constexpr SDL_Color AccentIceBlue = {173, 216, 230, 200};
constexpr SDL_Color AccentMint = {152, 255, 152, 200};
constexpr SDL_Color ShadowDark = {0, 0, 0, 150};

// --- Màu trong suốt / bán trong suốt ---
constexpr SDL_Color Transparent = {0, 0, 0, 0};
constexpr SDL_Color TranspWhite = {255, 255, 255, 128};
constexpr SDL_Color TranspRed = {255, 0, 0, 128};
constexpr SDL_Color TranspGreen = {0, 255, 0, 128};
constexpr SDL_Color TranspBlue = {0, 0, 255, 128};

// === Bản đồ tên → màu (không phân biệt hoa thường) ===
inline const std::unordered_map<std::string, SDL_Color> NameMap = {
    // Basic
    {"red", Red},
    {"green", Green},
    {"blue", Blue},
    {"black", Black},
    {"white", White},
    {"yellow", Yellow},
    {"cyan", Cyan},
    {"magenta", Magenta},
    {"orange", Orange},
    {"purple", Purple},
    {"pink", Pink},
    {"brown", Brown},
    {"gray", Gray},
    {"gray20", Gray20},
    {"gray50", Gray50},
    {"gray80", Gray80},
    {"darkgray", DarkGray},
    {"lightgray", LightGray},
    // Chill / Lofi
    {"lofi_beige", LofiBeige},
    {"lofi_brown", LofiBrown},
    {"lofi_mint", LofiMint},
    {"lofi_dustypink", LofiDustyPink},
    // Pastel
    {"pastel_peach", PastelPeach},
    {"pastel_lilac", PastelLilac},
    {"pastel_aqua", PastelAqua},
    {"pastel_lemon", PastelLemon},
    {"pastel_green", PastelGreen},
    {"pastel_sky", PastelSky},
    // Đêm
    {"night_sky", NightSky},
    {"twilight_purple", TwilightPurple},
    {"moon_gray", MoonGray},
    {"star_yellow", StarYellow},
    {"deepsea_blue", DeepSeaBlue},
    // Vaporwave / Retro
    {"vapor_pink", VaporPink},
    {"vapor_teal", VaporTeal},
    {"vapor_purple", VaporPurple},
    {"vapor_neongreen", VaporNeonGreen},
    {"vapor_orange", VaporOrange},
    // Cyberpunk / Neon
    {"neon_magenta", NeonMagenta},
    {"neon_cyan", NeonCyan},
    {"neon_yellow", NeonYellow},
    {"neon_orange", NeonOrange},
    {"neon_green", NeonGreen},
    // Earth Tones
    {"earth_sand", EarthSand},
    {"earth_olive", EarthOlive},
    {"earth_terracotta", EarthTerracotta},
    {"earth_forest", EarthForest},
    {"earth_clay", EarthClay},
    // Accent
    {"accent_hotpink", AccentHotPink},
    {"accent_iceblue", AccentIceBlue},
    {"accent_mint", AccentMint},
    {"shadow_dark", ShadowDark},
    // Transparent
    {"transparent", Transparent},
    {"transp_white", TranspWhite},
    {"transp_red", TranspRed},
    {"transp_green", TranspGreen},
    {"transp_blue", TranspBlue}};

// Lấy màu theo tên (không phân biệt hoa thường); ném exception nếu không tìm
// thấy.
inline SDL_Color GetByName(const std::string &name) {
    std::string key;
    key.reserve(name.size());
    for (char c : name) key += std::tolower(c);
    auto it = NameMap.find(key);
    if (it != NameMap.end()) return it->second;
    throw std::runtime_error("Color not found: " + name);
}

// Helper set màu cho renderer
inline void SetDrawColor(SDL_Renderer *r, const SDL_Color &c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
}

}  // namespace Colors

#endif  // COLORS_H
