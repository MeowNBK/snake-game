#include "game/game_logic.h"
#include "core/game_state.h" // Truy cập các biến toàn cục
#include "graphics/gfx.h"    // Dùng gfx::get_ticks()
#include <cmath>             // Dùng std::hypot
#include <random>
#include <algorithm>
#include <iostream>          // Cần cho debug (nếu có)

void spawn_apple() {
    static std::mt19937 mt{std::random_device{}()};
    std::uniform_int_distribution<int> dx(0, GRID_W - 1), dy(0, GRID_H - 1);
    bool on_snake;
    int safety_counter = 0;
    do {
        on_snake = false;
        apple = {dx(mt), dy(mt)};
        if (play_mode_cache != PlayMode::AI_ONLY) {
            for (const auto& seg : player1.body) if (apple == seg) { on_snake = true; break; }
        }
        if (on_snake) continue;
        if (play_mode_cache != PlayMode::PLAYER_ONLY) {
            for (const auto& seg : ai_player.body) if (apple == seg) { on_snake = true; break; }
        }
        safety_counter++;
    } while (on_snake && safety_counter < 100);
}

void reset_game() {
    if (current_mode != GameMode::MENU) {
        if (player_stats.highest_score < player1.body.size()) {
            player_stats.highest_score = player1.body.size();
        }
        if (ai_stats.highest_score < ai_player.body.size()) {
            ai_stats.highest_score = ai_player.body.size();
        }
    }
    
    bool ai_played_last_game = (play_mode_cache == PlayMode::AI_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI || current_mode == GameMode::HEADLESS_TRAINING);
    bool player_played_last_game = (play_mode_cache == PlayMode::PLAYER_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI || current_mode == GameMode::TRAINING_SESSION);

    if (current_mode != GameMode::MENU) { 
        if (player_played_last_game) {
            player_stats.games_played++;
        }
        if (ai_played_last_game) {
            ai_stats.games_played++;
        }
    }

    player1.reset({5, 5}, Dir::RIGHT);
    ai_player.reset({GRID_W - 5, 5}, Dir::LEFT);
    spawn_apple();

    game_start_time = gfx::get_ticks();
    player_apples_this_game = 0;
    ai_apples_this_game = 0;
    mirror_match_start_time = 0;
    steps_since_apple = 0;
    
    if (ai_played_last_game) {
        if (ai_player.epsilon > config.min_epsilon) {
            ai_player.epsilon *= config.epsilon_decay;
        }
    }
}

void update() {
    auto move_snake = [&](auto& snake, bool is_ai, const auto& opponent_body) -> double {
        if (!snake.is_alive) return 0.0;

        Point head = snake.body.front();
        double old_dist = std::hypot(head.x - apple.x, head.y - apple.y); 
        
        Point new_head = head;
        switch (snake.direction) {
            case Dir::UP:    new_head.y--; break;
            case Dir::DOWN:  new_head.y++; break;
            case Dir::LEFT:  new_head.x--; break;
            case Dir::RIGHT: new_head.x++; break;
        }

        if (new_head.x < 0) new_head.x = GRID_W - 1;
        if (new_head.x >= GRID_W) new_head.x = 0;
        if (new_head.y < 0) new_head.y = GRID_H - 1;
        if (new_head.y >= GRID_H) new_head.y = 0;

        for (const auto& segment : snake.body) {
            if (new_head == segment) {
                snake.is_alive = false;
                return -10.0;
            }
        }
        if (play_mode_cache == PlayMode::PLAYER_VS_AI) {
            for (const auto& segment : opponent_body) {
                if (new_head == segment) {
                    snake.is_alive = false;
                    return -10.0;
                }
            }
        }

        snake.body.push_front(new_head);
        
        double reward = 0.0;
        if (new_head == apple) {
            reward = 10.0;
            spawn_apple();
            if (is_ai) {
                ai_apples_this_game++;
                ai_stats.total_apples_eaten++;
                steps_since_apple = 0;
            } else {
                player_apples_this_game++;
                player_stats.total_apples_eaten++;
            }
        } else {
            snake.body.pop_back();
            double new_dist = std::hypot(new_head.x - apple.x, new_head.y - apple.y);
            reward = (new_dist < old_dist) ? 0.1 : -0.2; 
        }

        if (is_ai && steps_since_apple > GRID_W * GRID_H * 1.5) {
            snake.is_alive = false;
            reward = -10.0; 
        }
        return reward;
    };
    
    if (current_mode == GameMode::TRAINING_SESSION) {
        if (!player1.is_alive) { 
            current_mode = GameMode::MENU;
            return;
        }
        std::deque<Point> empty_body;
        std::vector<double> state_before_move = ai_player.get_state_vector(apple, empty_body); 
        int action_taken = static_cast<int>(player1.direction);
        move_snake(player1, false, empty_body); 
        expert_demonstrations.push_back({state_before_move, action_taken, 1.0, {}, false});
        return; 
    }

    bool is_ai_turn = (play_mode_cache == PlayMode::AI_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI || current_mode == GameMode::HEADLESS_TRAINING);
    bool is_player_turn = (play_mode_cache == PlayMode::PLAYER_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI);
    
    std::vector<double> old_state; int action_idx = 0; double reward = 0.0;

    if (is_ai_turn && ai_player.is_alive) {
        steps_since_apple++;
        old_state = ai_player.get_state_vector(apple, player1.body);
        Dir chosen_dir = ai_player.choose_move(old_state);
        ai_player.direction = chosen_dir; action_idx = static_cast<int>(chosen_dir);
    }
    
    if (is_player_turn && player1.is_alive) move_snake(player1, false, ai_player.body);
    if (is_ai_turn && ai_player.is_alive) reward = move_snake(ai_player, true, player1.body);

    if (is_ai_turn) {
        std::vector<double> new_state = ai_player.get_state_vector(apple, player1.body);
        ai_player.remember({old_state, action_idx, reward, new_state, !ai_player.is_alive});
        ai_player.train_from_memory(config.batch_size, config.learning_rate);
    }
    
    bool game_is_over = (!player1.is_alive && is_player_turn) || (!ai_player.is_alive && is_ai_turn);
    uint32_t game_time = gfx::get_ticks() - game_start_time;
    
    if (play_mode_cache == PlayMode::PLAYER_VS_AI && 
        player1.is_alive && ai_player.is_alive && 
        player1.body.size() == ai_player.body.size() && player1.body.size() > 3) 
    {
        if (mirror_match_start_time == 0) {
            mirror_match_start_time = gfx::get_ticks();
        }
    } else {
        mirror_match_start_time = 0;
    }
    
    achievement_manager.check_achievements(player_stats, player1, player_apples_this_game, ai_stats, ai_player, ai_apples_this_game, play_mode_cache, game_is_over, game_time, mirror_match_start_time);
    
    if (game_is_over && current_mode != GameMode::HEADLESS_TRAINING) {
        current_mode = GameMode::GAME_OVER;
    }
}
