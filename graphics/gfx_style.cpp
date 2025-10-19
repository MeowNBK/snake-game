#include "gfx_style.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

#include "gfx_type.h"

namespace gfx {
namespace style {

using std::map;
using std::string;
using std::vector;

/////////////////////
// Public API
/////////////////////

bool StyleManager::load_from_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    return load_from_string(ss.str());
}

bool StyleManager::load_from_string(const std::string& css) {
    rules.clear();
    // Very small parser: find blocks "selector { ... }"
    size_t pos = 0;
    int order = 0;
    const string s = css;
    while (true) {
        // find next '{'
        size_t bpos = s.find('{', pos);
        if (bpos == string::npos) break;
        // find selector text (from pos to bpos)
        string sel = trim(s.substr(pos, bpos - pos));
        // find matching '}'
        size_t epos = s.find('}', bpos);
        if (epos == string::npos) break;
        string block = s.substr(bpos + 1, epos - bpos - 1);
        Rule r;
        r.selectors = split_selectors(sel);
        r.props = parse_props_block(block);
        r.order = order++;
        rules.emplace_back(std::move(r));
        pos = epos + 1;
    }
    return true;
}

void StyleManager::clear() { rules.clear(); }

ComputedStyle StyleManager::compute(
    const std::string& tag, const std::string& id,
    const std::vector<std::string>& classes) const {
    // For each property we keep (specificity, order) chosen
    struct Choice {
        int spec = -1;
        int order = -1;
        std::string value;
        bool set = false;
    };

    std::map<std::string, Choice> chosen;

    auto consider_prop = [&](const std::string& name, const std::string& value,
                             int spec, int order) {
        auto& c = chosen[name];
        if (!c.set || spec > c.spec || (spec == c.spec && order > c.order)) {
            c.set = true;
            c.spec = spec;
            c.order = order;
            c.value = value;
        }
    };

    for (const auto& r : rules) {
        for (const auto& sel : r.selectors) {
            if (!selector_matches(sel, tag, id, classes)) continue;
            int spec = selector_specificity(sel);
            for (const auto& p : r.props) {
                consider_prop(p.first, p.second, spec, r.order);
            }
        }
    }

    ComputedStyle out;
    // map chosen props -> typed fields
    for (const auto& kv : chosen) {
        if (!kv.second.set) continue;
        const string& k = kv.first;
        const string& v = strip_quotes(kv.second.value);
        if (k == "color")
            out.color = parse_color(v);
        else if (k == "background-color" || k == "bg")
            out.background_color = parse_color(v);
        else if (k == "font")
            out.font = v;
        else if (k == "font-size") {
            if (auto iv = parse_int_px(v)) out.font_size = *iv;
        } else if (k == "width") {
            if (auto iv = parse_int_px(v)) out.width = *iv;
        } else if (k == "height") {
            if (auto iv = parse_int_px(v)) out.height = *iv;
        } else if (k == "padding") {
            if (auto iv = parse_int_px(v)) out.padding = *iv;
        } else if (k == "margin") {
            if (auto iv = parse_int_px(v)) out.margin = *iv;
        } else if (k == "border-width") {
            if (auto iv = parse_int_px(v)) out.border_width = *iv;
        } else if (k == "border-color") {
            out.border_color = parse_color(v);
        } else if (k == "opacity") {
            if (auto fv = parse_float(v)) out.opacity = *fv;
        } else if (k == "centered") {
            string vv = v;
            std::transform(vv.begin(), vv.end(), vv.begin(), ::tolower);
            out.centered = (vv == "true" || vv == "1" || vv == "yes");
        } else if (k == "visible") {
            string vv = v;
            std::transform(vv.begin(), vv.end(), vv.begin(), ::tolower);
            out.visible = (vv != "false" && vv != "0" && vv != "no");
        } else if (k == "blended") {
            string vv = v;
            std::transform(vv.begin(), vv.end(), vv.begin(), ::tolower);
            out.blended = (vv == "true" || vv == "1" || vv == "yes");
        }
        // else ignore unknown props (easy to extend)
    }

    return out;
}

void StyleManager::draw_text_styled(const std::string& text,
                                    const std::string& tag,
                                    const std::string& id,
                                    const std::vector<std::string>& classes,
                                    int x, int y,
                                    bool centered) const noexcept {
    try {
        ComputedStyle cs = compute(tag, id, classes);
        // early exit if visible == false
        if (cs.visible.has_value() && cs.visible.value() == false) return;

        // blended mode
        if (cs.blended.has_value()) gfx::set_blended_mode(cs.blended.value());

        // background
        int tw = 0, th = 0;
        std::string font_to_use = cs.font.value_or(std::string());
        int font_size = cs.font_size.value_or(0);
        if (!font_to_use.empty()) {
            // attempt to measure using font if exists and size (get_text_size
            // uses registered fonts)
            gfx::get_text_size(text, font_to_use, tw, th);
        } else {
            // if no font specified, try default measurement with "default"
            // (best-effort)
            gfx::get_text_size(text, font_to_use, tw, th);
        }

        int pad = cs.padding.value_or(0);
        int bx = x - pad;
        int by = y - pad;
        int bw = tw + pad * 2;
        int bh = th + pad * 2;

        if (cs.background_color.has_value()) {
            gfx::draw_rect(bx, by, bw, bh, cs.background_color.value(), true);
        }
        if (cs.border_width.has_value() && cs.border_width.value() > 0 &&
            cs.border_color.has_value()) {
            int bwid = cs.border_width.value();
            // draw outer border rect (simple)
            gfx::draw_rect(bx - bwid, by - bwid, bw + bwid * 2, bh + bwid * 2,
                           cs.border_color.value(), false);
        }

        // text color
        gfx::color text_color = cs.color.value_or(gfx::colors::white);
        bool center_flag = centered || cs.centered.value_or(false);
        // if opacity set, modulate alpha
        if (cs.opacity.has_value()) {
            int a = static_cast<int>(
                255.0f * std::clamp(cs.opacity.value(), 0.0f, 1.0f));
            text_color.a = static_cast<uint8_t>(a);
        }
        gfx::draw_text(text, cs.font.value_or(std::string()), x, y, text_color,
                       center_flag);
        // restore blended mode? (not necessary here)
    } catch (...) {
        // don't throw from noexcept wrapper
    }
}

void StyleManager::draw_rect_styled(const std::string& tag,
                                    const std::string& id,
                                    const std::vector<std::string>& classes,
                                    int x, int y, int w, int h) const noexcept {
    try {
        ComputedStyle cs = compute(tag, id, classes);
        if (cs.visible.has_value() && cs.visible.value() == false) return;
        if (cs.blended.has_value()) gfx::set_blended_mode(cs.blended.value());
        if (cs.background_color.has_value())
            gfx::draw_rect(x, y, w, h, cs.background_color.value(), true);
        if (cs.border_width.has_value() && cs.border_color.has_value()) {
            gfx::draw_rect(x, y, w, h, cs.border_color.value(),
                           false);  // simple single-line border
        }
    } catch (...) {
    }
}

/////////////////////
// Internal helpers
/////////////////////

string StyleManager::trim(const string& s) {
    size_t a = 0;
    size_t b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

vector<string> StyleManager::split_selectors(const string& s) {
    vector<string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == ',') {
            string t = trim(cur);
            if (!t.empty()) out.push_back(t);
            cur.clear();
        } else
            cur.push_back(c);
    }
    string t = trim(cur);
    if (!t.empty()) out.push_back(t);
    return out;
}

map<string, string> StyleManager::parse_props_block(const string& block) {
    map<string, string> out;
    // naive: split by ';' and parse key:value
    size_t pos = 0;
    while (pos < block.size()) {
        size_t semi = block.find(';', pos);
        string stmt;
        if (semi == string::npos) {
            stmt = block.substr(pos);
            pos = block.size();
        } else {
            stmt = block.substr(pos, semi - pos);
            pos = semi + 1;
        }
        auto colon = stmt.find(':');
        if (colon == string::npos) continue;
        string key = trim(stmt.substr(0, colon));
        string val = trim(stmt.substr(colon + 1));
        if (!key.empty()) out[key] = val;
    }
    return out;
}

gfx::color StyleManager::parse_color(const std::string& token) {
    std::string s = trim(token);
    if (s.empty()) return gfx::color(0, 0, 0, 255);

    // hex: #RRGGBB or #RRGGBBAA or #RGB
    if (s[0] == '#') {
        string h = s.substr(1);
        if (h.size() == 3) {
            int r = std::stoi(h.substr(0, 1), nullptr, 16);
            int g = std::stoi(h.substr(1, 1), nullptr, 16);
            int b = std::stoi(h.substr(2, 1), nullptr, 16);
            r = (r << 4) | r;
            g = (g << 4) | g;
            b = (b << 4) | b;
            return gfx::color(r, g, b, 255);
        } else if (h.size() == 6 || h.size() == 8) {
            unsigned long val = std::stoul(h, nullptr, 16);
            if (h.size() == 6) {
                int r = (val >> 16) & 0xFF;
                int g = (val >> 8) & 0xFF;
                int b = val & 0xFF;
                return gfx::color(r, g, b, 255);
            } else {
                int r = (val >> 24) & 0xFF;
                int g = (val >> 16) & 0xFF;
                int b = (val >> 8) & 0xFF;
                int a = val & 0xFF;
                return gfx::color(r, g, b, a);
            }
        }
    }

    // rgba(r,g,b,a) or rgb(...)
    {
        std::regex re_rgba(
            R"(\s*rgba?\s*\(\s*([0-9]+)\s*,\s*([0-9]+)\s*,\s*([0-9]+)\s*(?:,\s*([0-9]*\.?[0-9]+)\s*)?\)\s*)",
            std::regex::icase);
        std::smatch m;
        if (std::regex_match(s, m, re_rgba)) {
            int r = std::stoi(m[1].str());
            int g = std::stoi(m[2].str());
            int b = std::stoi(m[3].str());
            int a = 255;
            if (m.size() >= 5 && m[4].matched) {
                float fa = std::stof(m[4].str());
                if (fa <= 1.0f)
                    a = static_cast<int>(fa * 255.0f);
                else
                    a = static_cast<int>(std::clamp(fa, 0.0f, 255.0f));
            }
            return gfx::color(r, g, b, a);
        }
    }

    // named colors (map to gfx::colors)
    static const std::map<std::string, gfx::color> NAMED = {
        {"night_sky", gfx::colors::night_sky},
        {"twilight_purple", gfx::colors::twilight_purple},
        {"white", gfx::colors::white},
        {"gray80", gfx::colors::gray80},
        {"gray50", gfx::colors::gray50},
        {"dark_gray", gfx::colors::dark_gray},
        {"neon_cyan", gfx::colors::neon_cyan},
        {"cyan", gfx::colors::cyan},
        {"vapor_neon_green", gfx::colors::vapor_neon_green},
        {"neon_yellow", gfx::colors::neon_yellow},
        {"yellow", gfx::colors::yellow},
        {"neon_orange", gfx::colors::neon_orange},
        {"vapor_purple", gfx::colors::vapor_purple},
        {"pastel_lilac", gfx::colors::pastel_lilac}};
    string low = s;
    std::transform(low.begin(), low.end(), low.begin(), ::tolower);
    auto it = NAMED.find(low);
    if (it != NAMED.end()) return it->second;

    // fallback try parse int (e.g. "0xAARRGGBB" or decimal)
    try {
        long long v = std::stoll(s, nullptr, 0);
        // interpret as 0xAARRGGBB if large; if <= 0xFFFFFF assume RRGGBB
        if (v <= 0xFFFFFF && v >= 0) {
            int r = (v >> 16) & 0xFF;
            int g = (v >> 8) & 0xFF;
            int b = v & 0xFF;
            return gfx::color(r, g, b, 255);
        } else {
            // take low 32bits as AARRGGBB
            uint32_t uv = static_cast<uint32_t>(v & 0xFFFFFFFF);
            int r = (uv >> 24) & 0xFF;
            int g = (uv >> 16) & 0xFF;
            int b = (uv >> 8) & 0xFF;
            int a = uv & 0xFF;
            return gfx::color(r, g, b, a);
        }
    } catch (...) {
    }

    // default
    return gfx::color(0, 0, 0, 255);
}

std::optional<int> StyleManager::parse_int_px(const std::string& token) {
    std::string s = trim(token);
    if (s.empty()) return {};
    // allow "12px" or "12"
    if (s.size() >= 2 && s.substr(s.size() - 2) == "px")
        s = s.substr(0, s.size() - 2);
    try {
        int v = std::stoi(s);
        return v;
    } catch (...) {
        return {};
    }
}

std::optional<float> StyleManager::parse_float(const std::string& token) {
    std::string s = trim(token);
    if (s.empty()) return {};
    try {
        float v = std::stof(s);
        return v;
    } catch (...) {
        return {};
    }
}

bool StyleManager::selector_matches(const std::string& selector,
                                    const std::string& tag,
                                    const std::string& id,
                                    const std::vector<std::string>& classes) {
    // simple: selector is either "#id", ".class", or "tag"
    if (selector.empty()) return false;
    if (selector[0] == '#') {
        return selector.substr(1) == id;
    } else if (selector[0] == '.') {
        string cls = selector.substr(1);
        for (const auto& c : classes)
            if (c == cls) return true;
        return false;
    } else {
        return selector == tag;
    }
}

int StyleManager::selector_specificity(const std::string& selector) {
    if (selector.empty()) return 0;
    if (selector[0] == '#') return 100;
    if (selector[0] == '.') return 10;
    return 1;
}

std::string StyleManager::strip_quotes(const std::string& s) {
    if (s.size() >= 2) {
        if ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\'')) {
            return s.substr(1, s.size() - 2);
        }
    }
    return s;
}

}  // namespace style
}  // namespace gfx
