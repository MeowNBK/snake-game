#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace gfx {

// Color (RGBA 0..255)
struct color {
    uint8_t r, g, b, a;

    color() noexcept = default;
    constexpr color(int red, int green, int blue, int alpha)
        : r(red), g(green), b(blue), a(alpha) {}
    color(int32_t i) noexcept { from_int(i); }
    color(const std::string& s) { from_string(s); }
    color(const color& other) noexcept { from_int(other.as_int()); }
    color(color&& other) noexcept {
        from_int(other.as_int());
        other.from_int(0);
    }
    inline color& operator=(const color& other) noexcept {
        if (this == &other) return *this;
        from_int(other.as_int());
        return *this;
    }
    inline color& operator=(color&& other) noexcept {
        if (this == &other) return *this;
        from_int(other.as_int());
        other.from_int(0);
        return *this;
    }
    inline color& operator=(const std::string& s) noexcept { 
        from_string(s);
        return *this;
    }
    inline color& operator=(int32_t i) noexcept { 
        from_int(i);
        return *this;
    }
    inline color& operator=(std::initializer_list<int> list) noexcept {
        auto it = list.begin();
        if (list.size() >= 4) {
            r = *it++;
            g = *it++;
            b = *it++;
            a = *it++;
        }
        return *this;
    }
    ~color() noexcept = default;

    [[nodiscard]] inline int32_t as_int() const noexcept {
        int32_t res = 0;
        res |= (r << 24);
        res |= (g << 16);
        res |= (b << 8);
        res |= a;
        return res;
    }

    inline void from_int(int32_t i) noexcept {
        r = (i >> 24) & 0xFF;
        g = (i >> 16) & 0xFF;
        b = (i >> 8) & 0xFF;
        a = i & 0xFF;
    }

    inline void from_string(const std::string& s) noexcept {
        using i32 = int32_t;
        using u32 = uint32_t;
        using i32_limits = std::numeric_limits<i32>;

        std::string_view str = s;

        size_t left = 0;
        while (left < str.size() &&
               std::isspace(static_cast<unsigned char>(str[left])))
            ++left;
        size_t right = str.size();
        while (right > left &&
               std::isspace(static_cast<unsigned char>(str[right - 1])))
            --right;
        if (left >= right) return from_int(0);

        str = str.substr(left, right - left);

        bool negative = false;
        size_t pos = 0;
        if (str[0] == '-') {
            negative = true;
            str.remove_prefix(1);
        } else if (str[0] == '+') {
            str.remove_prefix(1);
        }

        int base = 10;

        if (str.size() >= 2) {
            std::string_view prefix = str.substr(0, 2);

            // We can handle binary right there
            if (prefix == "0b" || prefix == "0B") {
                u32 accumulator = 0;
                const u32 limit = static_cast<u32>(
                    i32_limits::max());  // The maximum value Integer can reach

                str.remove_prefix(2);
                for (char c : str) {
                    if (c == '0' || c == '1') {
                        int d = c - '0';
                        if (accumulator > (limit - d) / 2) {
                            return negative ? from_int(i32_limits::min())
                                            : from_int(i32_limits::max());
                        }
                        accumulator = (accumulator << 1) | static_cast<u32>(d);
                    } else
                        break;
                }

                i32 result = static_cast<i32>(accumulator);
                return negative ? from_int(-result) : from_int(result);
            } else if (prefix == "0x" || prefix == "0X") {
                base = 16;
                str.remove_prefix(2);
            } else if (prefix == "0o" || prefix == "0O") {
                base = 8;
                str.remove_prefix(2);
            } else if (str[0] == '#') {
                base = 16;
                str.remove_prefix(1);
            }
        }

        errno = 0;
        char* endptr = nullptr;
        const std::string token(str.begin(), str.end());
        i32 value = std::strtoll(token.c_str(), &endptr, base);
        if (endptr == token.c_str()) return from_int(0);
        if (errno == ERANGE) {
            return (value > 0) ? from_int(i32_limits::max())
                               : from_int(i32_limits::min());
        }
        if (value > i32_limits::max()) {
            return from_int(i32_limits::max());
        }
        if (value < i32_limits::min()) {
            return from_int(i32_limits::min());
        }

        from_int(value);
    }
};

// Standardized keys that the API exposes (SDL scancodes/keycodes are hidden)
enum class key {
    UP,
    DOWN,
    LEFT,
    RIGHT,
    RETURN,
    ESCAPE,
    S,
    L,
    R,
    T,
    P,
    I,
    E,
    UNKNOWN
};

// Simple event (only QUIT and KEYDOWN are reported)
struct event {
    enum class Type { NONE, QUIT, KEYDOWN } type = Type::NONE;
    key key = key::UNKNOWN;
};

// Opaque texture identifier. 0 == invalid.
using texture_id = uint32_t;

}  // namespace gfx