#include "core/game_state.h"

// === Định nghĩa các biến toàn cục ===

// Trạng thái game
bool is_running = true;
GameMode current_mode = GameMode::MENU;
PlayMode play_mode_cache = PlayMode::PLAYER_ONLY;
int selected_menu_item = 0;

// Đối tượng game
Config config;
Player player1;
AIPlayer ai_player;
Point apple;
AchievementManager achievement_manager;
PlayerStats player_stats;
PlayerStats ai_stats;

// Biến theo dõi trạng thái cho thành tích
int player_apples_this_game = 0;
int ai_apples_this_game = 0;
uint32_t game_start_time = 0;
uint32_t mirror_match_start_time = 0;
int steps_since_apple = 0;

std::vector<AIPlayer::Experience> expert_demonstrations;
