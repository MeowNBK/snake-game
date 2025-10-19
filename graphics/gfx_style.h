#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "gfx.h"
#include "gfx_type.h"

namespace gfx {
namespace style {

// A computed style (result of cascade). Fields are optional — if not set,
// caller falls back.
struct ComputedStyle {
    std::optional<gfx::color> color;
    std::optional<gfx::color> background_color;
    std::optional<std::string> font;  // font id (as used in gfx::add_font)
    std::optional<int> font_size;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<int> padding;
    std::optional<int> margin;
    std::optional<int> border_width;
    std::optional<gfx::color> border_color;
    std::optional<float> opacity;  // 0..1
    std::optional<bool> centered;  // text centered horizontally
    std::optional<bool> visible;
    std::optional<bool> blended;
};

// Main StyleManager
class StyleManager {
   public:
    StyleManager() = default;

    // load CSS-like from file or string
    bool load_from_file(const std::string& path);
    bool load_from_string(const std::string& css);

    // clear all rules
    void clear();

    // Compute final style for an element described by:
    // tag (e.g. "text", "rect", any logical tag), id (without leading '#'),
    // classes (no leading '.')
    ComputedStyle compute(const std::string& tag, const std::string& id = "",
                          const std::vector<std::string>& classes = {}) const;

    // convenience draw helpers (use computed style: background -> border ->
    // content)
    void draw_text_styled(const std::string& text, const std::string& tag,
                          const std::string& id,
                          const std::vector<std::string>& classes, int x, int y,
                          bool centered = false) const noexcept;

    void draw_rect_styled(const std::string& tag, const std::string& id,
                          const std::vector<std::string>& classes, int x, int y,
                          int w, int h) const noexcept;

   private:
    struct Rule {
        std::vector<std::string> selectors;  // e.g. "text", ".menu", "#main"
        std::map<std::string, std::string> props;  // raw properties
        int order = 0;  // rule order (for tie-breaking)
    };

    std::vector<Rule> rules;

    // parsing helpers (internal)
    static std::string trim(const std::string& s);
    static std::vector<std::string> split_selectors(const std::string& s);
    static std::map<std::string, std::string> parse_props_block(
        const std::string& block);
    static gfx::color parse_color(const std::string& token);
    static std::optional<int> parse_int_px(const std::string& token);
    static std::optional<float> parse_float(const std::string& token);
    static bool selector_matches(const std::string& selector,
                                 const std::string& tag, const std::string& id,
                                 const std::vector<std::string>& classes);
    static int selector_specificity(const std::string& selector);
    static std::string strip_quotes(const std::string& s);
};

}  // namespace style
}  // namespace gfx
