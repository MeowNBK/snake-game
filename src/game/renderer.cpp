#include "game/renderer.h"
#include "core/game_state.h" // Truy cập biến toàn cục
#include "graphics/gfx.h"    // Dùng các hàm gfx::draw_*
#include <string>
#include <vector>

void render_menu() {
    const std::vector<std::string> menu_items = {
        "Chơi một mình",
        "Xem AI chơi",
        "Solo với AI",
        "Training Trong Mơ (Siêu Tốc)",
        "Dạy AI Chơi (Snake Trainer)", 
        "Xem Thành Tích"
    };
    const int menu_item_count = menu_items.size();

    gfx::draw_text("Rắn Săn Mồi AI", "large", 0, 80, gfx::colors::neon_cyan, true);

    for (int i = 0; i < menu_item_count; ++i) {
        bool is_selected = (i == selected_menu_item);
        gfx::color color = is_selected ? gfx::colors::neon_yellow : gfx::colors::white;

        if (is_selected) {
            int tw = 0, th = 0;
            gfx::get_text_size(menu_items[i], "medium", tw, th);
            int bg_x = (WINDOW_W - tw - 20) / 2;
            int bg_y = 180 + i * 50 - 5;
            int bg_w = tw + 20;
            int bg_h = th + 10;
            gfx::draw_rect(bg_x, bg_y, bg_w, bg_h, gfx::colors::twilight_purple, true);
        }
        gfx::draw_text(menu_items[i], "medium", 0, 180 + i * 50, color, true);
    }
    
    gfx::draw_text("L/S: Tải/Lưu Não | T: Train từ Dữ liệu | R: Chơi Lại | ESC: Menu", "small", 0, WINDOW_H - 40, gfx::colors::gray80, true);
}

void render_game() {
    if (play_mode_cache != PlayMode::AI_ONLY) {
        for(const auto& p : player1.body) {
            gfx::draw_rect(p.x*CELL_SIZE, p.y*CELL_SIZE, CELL_SIZE, CELL_SIZE, 
                           player1.is_alive ? gfx::colors::vapor_neon_green : gfx::colors::gray50, true);
        }
    }
    if (play_mode_cache != PlayMode::PLAYER_ONLY) {
        for(const auto& p : ai_player.body) {
            gfx::draw_rect(p.x*CELL_SIZE, p.y*CELL_SIZE, CELL_SIZE, CELL_SIZE, 
                           ai_player.is_alive ? gfx::colors::vapor_purple : gfx::colors::gray50, true);
        }
    }
    
    gfx::draw_rect(apple.x*CELL_SIZE, apple.y*CELL_SIZE, CELL_SIZE, CELL_SIZE, gfx::colors::neon_orange, true);
    
    std::string score_text;
    if (play_mode_cache == PlayMode::PLAYER_VS_AI) score_text = "Player: " + std::to_string(player1.body.size()) + " | AI: " + std::to_string(ai_player.body.size());
    else if (play_mode_cache == PlayMode::PLAYER_ONLY) score_text = "Điểm: " + std::to_string(player1.body.size());
    else score_text = "Điểm AI: " + std::to_string(ai_player.body.size());
    
    gfx::draw_text(score_text, "medium", 10, 10, gfx::colors::white);
}

void render_game_over() {
    gfx::set_blended_mode(true);
    gfx::draw_rect(0, 0, WINDOW_W, WINDOW_H, {20, 20, 20, 210}, true);
    gfx::set_blended_mode(false);

    std::string winner_text = "";
    if (play_mode_cache == PlayMode::PLAYER_VS_AI) {
        if (!player1.is_alive && !ai_player.is_alive) winner_text = "Hòa!";
        else if (!player1.is_alive) winner_text = "AI Thắng!";
        else winner_text = "Bạn Thắng!";
    } else {
        winner_text = "GAME OVER";
    }
    
    gfx::draw_text(winner_text, "large", 0, WINDOW_H / 2 - 100, gfx::colors::neon_orange, true);
    
    std::string score_info;
    if (play_mode_cache == PlayMode::PLAYER_ONLY) score_info = "Điểm cuối: " + std::to_string(player1.body.size());
    else if (play_mode_cache == PlayMode::AI_ONLY) score_info = "Điểm cuối: " + std::to_string(ai_player.body.size());
    else score_info = "Tỉ số: Player " + std::to_string(player1.body.size()) + " - " + std::to_string(ai_player.body.size()) + " AI";
    
    gfx::draw_text(score_info, "medium", 0, WINDOW_H / 2, gfx::colors::white, true);
    gfx::draw_text("Nhấn 'R' để chơi lại | 'ESC' để về Menu", "medium", 0, WINDOW_H / 2 + 50, gfx::colors::cyan, true);
}

// Hàm render chính, điều phối các hàm con
void render() {
    switch (current_mode) {
        case GameMode::MENU: render_menu(); break;
        case GameMode::PLAYING: render_game(); break;
        case GameMode::GAME_OVER: {
            render_game();
            render_game_over();
            break;
        }
        case GameMode::ACHIEVEMENT_LIST:{
            achievement_manager.render_list_screen("large", "medium", "small");
            break;
        }
        case GameMode::HEADLESS_TRAINING: {             
            gfx::draw_text("TRAINING TRONG MƠ...", "large", 0, WINDOW_H / 2 - 80, gfx::colors::white, true);
            std::string progress = "Ván chơi: " + std::to_string(ai_stats.games_played) + " | Điểm cao nhất: " + std::to_string(ai_stats.highest_score);
            gfx::draw_text(progress, "medium", 0, WINDOW_H / 2, gfx::colors::cyan, true);
            std::string epsilon_text = "Epsilon: " + std::to_string(ai_player.epsilon);
            gfx::draw_text(epsilon_text, "medium", 0, WINDOW_H / 2 + 40, gfx::colors::cyan, true);
            gfx::draw_text("Nhấn ESC để quay lại Menu", "small", 0, WINDOW_H - 50, gfx::colors::gray80, true);
            break;
        }
        case GameMode::TRAINING_SESSION: {
            render_game();
            gfx::draw_text("Training Session: Điểu khiển để tạo data cho AI", "small", 0, WINDOW_H - 40, gfx::colors::cyan, true);
            break;
        }
    }

    achievement_manager.render_notifications("medium");
}
