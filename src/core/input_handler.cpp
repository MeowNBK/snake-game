#include "core/input_handler.h"
#include "core/game_state.h" // Truy cập biến toàn cục
#include "game/game_logic.h" // Dùng reset_game()
#include "ai/expert_data.h"  // Dùng các hàm load/save/train
#include "graphics/gfx.h"    // Dùng gfx::event
#include <iostream>
#include <algorithm>
#include <random>

void handle_input() {
    gfx::event e;
    while ((e = gfx::poll_event()).type != gfx::event::Type::NONE) {
        if (e.type == gfx::event::Type::QUIT) {
            is_running = false;
        }
        if (e.type == gfx::event::Type::KEYDOWN) {
            if (e.key == gfx::key::S) { ai_player.brain.save_weights("snake_brain.bin"); std::cout << "Đã lưu bộ não AI!\n"; }
            if (e.key == gfx::key::L) { ai_player.brain.load_weights("snake_brain.bin"); ai_player.target_brain.copy_from(ai_player.brain); std::cout << "Đã tải bộ não AI!\n"; }
            if (e.key == gfx::key::ESCAPE) { current_mode = GameMode::MENU; }

            switch (current_mode) {
                case GameMode::MENU:
                    switch (e.key) {
                        case gfx::key::UP:   selected_menu_item = (selected_menu_item - 1 + 6) % 6; break;
                        case gfx::key::DOWN: selected_menu_item = (selected_menu_item + 1) % 6; break;
                        case gfx::key::RETURN:
                            if      (selected_menu_item == 0) { play_mode_cache = PlayMode::PLAYER_ONLY; current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 1) { play_mode_cache = PlayMode::AI_ONLY;     current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 2) { play_mode_cache = PlayMode::PLAYER_VS_AI;  current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 3) { play_mode_cache = PlayMode::AI_ONLY;     current_mode = GameMode::HEADLESS_TRAINING; reset_game(); }
                            else if (selected_menu_item == 4) { play_mode_cache = PlayMode::PLAYER_ONLY; current_mode = GameMode::TRAINING_SESSION; reset_game(); } 
                            else if (selected_menu_item == 5) { current_mode = GameMode::ACHIEVEMENT_LIST; }
                            break;
                        case gfx::key::T:
                            if (!expert_demonstrations.empty()) {
                                std::shuffle(
                                    expert_demonstrations.begin(),
                                    expert_demonstrations.end(),
                                    std::mt19937{std::random_device{}()}
                                );
                                ai_player.train_from_demonstrations(
                                    expert_demonstrations,
                                    config.epochs,
                                    config.learning_rate,
                                    config.batch_size
                                );
                                expert_demonstrations.clear();
                                std::cout << "Đã học xong dữ liệu từ chuyên gia. Nhấn S để lưu lại bộ não mới.\n";
                            } else {
                                std::cout << "Chưa có dữ liệu từ chuyên gia để huấn luyện! Hãy vào chế độ 'AI Training'.\n";
                            }
                            break;
                        case gfx::key::P: load_pretrained_brain(ai_player); break;
                        case gfx::key::I: load_expert_demonstrations("expert_data.bin"); break;
                        case gfx::key::E: save_expert_demonstrations("expert_data.bin"); break;
                        default: break;
                    }
                    break;

                case GameMode::PLAYING:
                case GameMode::TRAINING_SESSION:
                    if (e.key == gfx::key::R) { reset_game(); }
                    switch (e.key) {
                        case gfx::key::UP:    if (player1.direction != Dir::DOWN) player1.direction = Dir::UP; break;
                        case gfx::key::DOWN:  if (player1.direction != Dir::UP) player1.direction = Dir::DOWN; break;
                        case gfx::key::LEFT:  if (player1.direction != Dir::RIGHT) player1.direction = Dir::LEFT; break;
                        case gfx::key::RIGHT: if (player1.direction != Dir::LEFT) player1.direction = Dir::RIGHT; break;
                        default: break;
                    }
                    break;

                case GameMode::GAME_OVER:
                    if (e.key == gfx::key::R) { current_mode = GameMode::PLAYING; reset_game(); }
                    break;
                
                default: break;
            }
        }
    }
}
