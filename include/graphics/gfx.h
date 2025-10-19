#pragma once

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

#include "graphics/gfx_type.h"

namespace gfx {

/**
 * Khởi tạo hệ thống gfx (tạo cửa sổ, renderer, init TTF).
 * - Không ném ngoại lệ; trả về false nếu lỗi.
 * - Các hàm khác sẽ no-op nếu init() thất bại.
 *
 * @param title : tiêu đề cửa sổ
 * @param width, height : kích thước ban đầu (px)
 * @return true nếu khởi tạo thành công
 *
 * Ví dụ:
 *   if (!gfx::init("MyGame", 800, 600)) return -1;
 */
bool init(const std::string& title, int width, int height) noexcept;

/**
 * Thêm font (logic id -> file .ttf)
 * @param id : tên logic dùng khi gọi draw_text / get_text_size
 * @param path : đường dẫn tới file TTF
 * @param size : cỡ chữ (pt)
 * @return true nếu load thành công (hoặc font đã tồn tại)
 */
bool add_font(const std::string& id, const std::string& path,
              int size) noexcept;

/**
 * Kiểm tra font đã được add chưa
 */
bool font_exists(const std::string& id) noexcept;

/**
 * Gỡ font khỏi hệ thống.
 * - Xóa các cache text liên quan.
 * @return true nếu đã tồn tại và xóa thành công
 */
bool remove_font(const std::string& id) noexcept;

/**
 * Dọn dẹp toàn bộ tài nguyên và tắt SDL/TTF.
 * Gọi trước khi thoát chương trình.
 */
void shutdown() noexcept;

/**
 * Lấy event đơn giản (NON-BLOCKING): trả về Event::Type::NONE nếu không có
 * event.
 */
event poll_event() noexcept;

/**
 * Bắt đầu frame mới, xóa màn hình bằng clear_color.
 * Gọi trước khi vẽ primitives / textures / text.
 */
void begin_draw(color clear_color) noexcept;

/**
 * Kết thúc frame và present lên màn hình.
 */
void end_draw() noexcept;

/**
 * Bật/tắt blended drawing mode cho renderer.
 */
void set_blended_mode(bool enabled) noexcept;

/**
 * Cài tỉ lệ scale cho renderer (affects subsequent draws).
 */
void set_scale(float sx, float sy) noexcept;

/**
 * Đổi tiêu đề cửa sổ.
 */
void set_window_title(const std::string& title) noexcept;

/**
 * Lấy kích thước hiện tại của cửa sổ (px)
 */
void get_window_size(int& out_w, int& out_h) noexcept;

/**
 * Toggle fullscreen (desktop fullscreen).
 */
void toggle_fullscreen() noexcept;

/**
 * Thử bật/tắt vsync.
 * --- CHÚ Ý QUAN TRỌNG ---
 * Việc recreate renderer để bật/tắt vsync sẽ **invalidate tất cả** các
 * TextureID hiện có (SDL_Texture gắn với renderer cũ sẽ bị hủy). Hàm này sẽ hủy
 * hết texture trong hệ thống (bao gồm cache text) khi thành công.
 *
 * @return true nếu recreate renderer thành công (lúc này textures đã bị hủy),
 * false nếu lỗi.
 */
bool set_vsync(bool enable) noexcept;

// --- Drawing primitives ---
void draw_rect(int x, int y, int w, int h, color color,
               bool fill = true) noexcept;
void draw_line(int x1, int y1, int x2, int y2, color color) noexcept;
void draw_circle(int cx, int cy, int radius, color color,
                 bool fill = false) noexcept;

// --- Text ---
/**
 * Vẽ text bằng font logic id (phải add_font trước).
 * - centered = true => căn giữa theo chiều ngang cửa sổ (x,y lúc này là ignored
 * x)
 */
void draw_text(const std::string& text, const std::string& font_id, int x,
               int y, color color, bool centered = false) noexcept;

/**
 * Lấy kích thước hộp chữ (w,h) với font đã add
 */
void get_text_size(const std::string& text, const std::string& font_id,
                   int& out_w, int& out_h) noexcept;

/**
 * Xóa cache text (dọn texture cache được tạo khi render text).
 */
void clear_text_cache() noexcept;

// --- Texture API (opaque handles) ---
/**
 * Tạo texture từ BMP file. Trả về 0 nếu lỗi.
 */
texture_id create_texture_from_bmp(const std::string& path) noexcept;

/**
 * Tạo texture làm render target (texture rỗng với kích thước w,h).
 * Trả về handle (0 == lỗi).
 */
texture_id create_render_target(int w, int h) noexcept;

/**
 * Hủy một texture theo handle. Trả về true nếu đã hủy.
 */
bool destroy_texture(texture_id id) noexcept;

/**
 * Vẽ texture theo handle.
 * - Nếu w==0 hoặc h==0 sẽ dùng kích thước gốc của texture.
 * - centered=true => coi x,y là tâm texture
 */
void draw_texture(texture_id id, int x, int y, int w = 0, int h = 0,
                  double angle = 0.0, bool centered = false) noexcept;

/**
 * Như draw_texture nhưng cho phép tint màu (Color.a dùng như alpha)
 */
void draw_texture_tinted(texture_id id, int x, int y, color tint, int w = 0,
                         int h = 0, double angle = 0.0,
                         bool centered = false) noexcept;

/**
 * Đổi render target. id == 0 => default backbuffer.
 */
void set_render_target(texture_id id) noexcept;

/**
 * Lưu screenshot hiện tại vào BMP file đường dẫn path.
 */
bool take_screenshot(const std::string& path) noexcept;

/**
 * FPS xấp xỉ (cập nhật mỗi giây)
 */
float get_fps() noexcept;

uint32_t get_ticks() noexcept;
void delay(uint32_t ms) noexcept;

namespace colors {
inline constexpr color night_sky = {16, 20, 31, 255};
inline constexpr color twilight_purple = {36, 28, 51, 255};
inline constexpr color white = {255, 255, 255, 255};
inline constexpr color gray80 = {204, 204, 204, 255};
inline constexpr color gray50 = {128, 128, 128, 255};
inline constexpr color dark_gray = {64, 64, 64, 255};
inline constexpr color neon_cyan = {0, 255, 255, 255};
inline constexpr color cyan = {0, 180, 180, 255};
inline constexpr color vapor_neon_green = {57, 255, 20, 255};
inline constexpr color neon_yellow = {255, 230, 0, 255};
inline constexpr color yellow = {255, 255, 0, 255};
inline constexpr color neon_orange = {255, 107, 0, 255};
inline constexpr color vapor_purple = {156, 39, 176, 255};
inline constexpr color pastel_lilac = {200, 180, 220, 255};
}  // namespace colors

}  // namespace gfx
