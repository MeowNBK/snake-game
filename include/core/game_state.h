#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <vector>
#include <deque>
#include <cstdint> // Cho uint32_t
#include "core/common.h"
#include "ai/ai_player.h"
#include "game/achievements.h"

// === Cấu trúc Config ===
struct Config {
    int fps = 30;
    double learning_rate = 0.001;
    double epsilon = 0.0;
    double min_epsilon = 0.0;
    double epsilon_decay = 0.9995;
    int epochs = 30;
    int batch_size = 32;
};

// === Khai báo 'extern' cho các biến toàn cục ===

// Trạng thái game
extern bool is_running;
extern GameMode current_mode;
extern PlayMode play_mode_cache;
extern int selected_menu_item;

// Đối tượng game
extern Config config;
extern Player player1;
extern AIPlayer ai_player;
extern Point apple;
extern AchievementManager achievement_manager;
extern PlayerStats player_stats;
extern PlayerStats ai_stats;

// Biến theo dõi trạng thái cho thành tích
extern int player_apples_this_game;
extern int ai_apples_this_game;
extern uint32_t game_start_time;
extern uint32_t mirror_match_start_time;
extern int steps_since_apple;

// Dữ liệu huấn luyện
extern std::vector<AIPlayer::Experience> expert_demonstrations;

#endif // GAME_STATE_H
