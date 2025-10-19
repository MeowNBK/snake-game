// === BAO GỒM CÁC MODULE ===
#include "graphics/gfx.h"
#include "core/common.h"
#include "core/config.h"       // Module Config
#include "core/system.h"       // Module Init/Cleanup
#include "core/game_state.h"   // Module Trạng thái toàn cục
#include "core/input_handler.h"// Module Xử lý Input
#include "game/game_logic.h"   // Module Logic Game
#include "game/renderer.h"   // Module Vẽ
#include "ai/ai_player.h"      // (Cần cho achievement_manager)
#include "game/achievements.h" // (Cần cho achievement_manager)

// === HÀM MAIN - Entry Point ===
int main(int argc, char* argv[]) {
    // 1. Khởi tạo
    load_config("config.ini"); // Từ core/config.cpp
    parse_args(argc, argv);    // Từ core/config.cpp
    init(); // Từ core/system.cpp

    achievement_manager.initialize();
    achievement_manager.load_progress(player_stats, ai_stats);
    ai_player.epsilon = config.epsilon;

    // 2. Vòng lặp game
    while (is_running) {
        if (current_mode == GameMode::HEADLESS_TRAINING) {
            handle_input(); // Vẫn xử lý để có thể nhấn ESC thoát
            update();       // Từ game/game_logic.cpp
            if (!ai_player.is_alive) {
                reset_game(); // Từ game/game_logic.cpp
            }
        } else {
            // GFX xử lý clear và present
            gfx::begin_draw(gfx::colors::night_sky);

            handle_input(); // Từ core/input_handler.cpp
            if (current_mode == GameMode::PLAYING || current_mode == GameMode::TRAINING_SESSION) {
                update();   // Từ game/game_logic.cpp
            }
            render();       // Từ graphics/renderer.cpp

            gfx::end_draw();
        }
    }

    // 3. Dọn dẹp
    achievement_manager.save_progress(player_stats, ai_stats);
    cleanup(); // Từ core/system.cpp
    return 0;
}

// === TRIỂN KHAI CÁC HÀM CỦA AchievementManager CẦN GFX ===
// (Để tạm ở đây, hoặc tạo file src/game/achievements.cpp)

void AchievementManager::render_notifications(const std::string& font_id) {
    if (notification_queue.empty()) return;
    uint32_t current_time = gfx::get_ticks();
    if (notification_start_time == 0) {
        notification_start_time = current_time;
    }
    if (current_time - notification_start_time < 3000) {
        const Achievement& ach = notification_queue.front();
        std::string text = "Thành Tích Mới: " + ach.name;
        
        int text_w = 0, text_h = 0;
        gfx::get_text_size(text, font_id, text_w, text_h);

        int bg_x = (WINDOW_W - text_w - 20) / 2;
        int bg_y = WINDOW_H - 60;
        int bg_w = text_w + 20;
        int bg_h = text_h + 10;
        
        gfx::set_blended_mode(true);
        gfx::draw_rect(bg_x, bg_y, bg_w, bg_h, {20, 20, 20, 220}, true);
        
        int text_x = (WINDOW_W - text_w) / 2;
        int text_y = WINDOW_H - 55;
        gfx::draw_text(text, font_id, text_x, text_y, gfx::colors::yellow);
        gfx::set_blended_mode(false);
    } else {
        notification_queue.pop_front();
        notification_start_time = 0;
    }
}

void AchievementManager::render_list_screen(const std::string& font_title_id, const std::string& font_text_id, const std::string& font_small_id) {
    gfx::draw_text("Phòng Truyền Thống", font_title_id, 0, 50, gfx::colors::neon_cyan, true);
    int y_pos = 120;
    int unlocked_count = 0;
    for (const auto& pair : achievements) {
        const Achievement& ach = pair.second;
        if (ach.unlocked) {
            gfx::draw_text(ach.name, font_text_id, 50, y_pos, gfx::colors::vapor_neon_green, false);
            gfx::draw_text(ach.description, font_small_id, 70, y_pos + 25, gfx::colors::white);
            unlocked_count++;
        } else {
            gfx::draw_text("Thành Tích Bí Ẩn", font_text_id, 50, y_pos, gfx::colors::gray50, false);
            gfx::draw_text("????????????????", font_small_id, 70, y_pos + 25, gfx::colors::dark_gray);
        }
        y_pos += 60;
    }
    std::string progress_text = "Tiến độ: " + std::to_string(unlocked_count) + "/" + std::to_string(achievements.size());
    gfx::draw_text(progress_text, font_text_id, 0, 20, gfx::colors::pastel_lilac, true);
    gfx::draw_text("Nhấn ESC để quay lại Menu", font_small_id, 0, WINDOW_H - 40, gfx::colors::cyan, true);
}
