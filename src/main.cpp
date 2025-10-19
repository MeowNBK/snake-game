// === ĐÃ LOẠI BỎ SDL VÀ TTF, THAY BẰNG GFX ===
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <sstream>
#include <deque>
#include <algorithm>
#include <random>
#include <cmath> // Cần cho std::hypot

// Thư viện GFX mới
#include "graphics/gfx.h" 

// Các file header tự tạo
#include "core/common.h"
#include "ai/ai_player.h"
#include "game/achievements.h"
// #include "libs/colors.h" // KHÔNG DÙNG NỮA, gfx::colors đã thay thế

// === Cấu trúc Config & Các biến toàn cục ===

struct Config {
    int fps = 30; // GFX không dùng, nhưng logic game có thể dùng
    double learning_rate = 0.001;
    double epsilon = 0.0;
    double min_epsilon = 0.0;
    double epsilon_decay = 0.9995;
    int epochs = 30;
    int batch_size = 32;
};

// === LOẠI BỎ CÁC BIẾN TOÀN CỤC SDL ===
// SDL_Window* window = nullptr;
// SDL_Renderer* renderer = nullptr;
// TTF_Font* font_medium = nullptr;
// TTF_Font* font_large = nullptr;
// TTF_Font* font_small = nullptr;

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

// === Khai báo các hàm (Function Prototypes) ===
void load_config(const std::string& filename);
void parse_args(int argc, char* argv[]);
void init();
void cleanup();
void handle_input();
void reset_game();
void update();
void render();
// void render_text(...); // KHÔNG CẦN NỮA, gfx::draw_text đã thay thế
void render_menu();
void render_game();
void render_game_over();
void spawn_apple();

// --- Triển khai các hàm của AchievementManager cần GFX ---
// === ĐÃ REFACTOR SANG GFX ===
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
        // Dùng màu từ gfx_type.h hoặc tạo màu tại chỗ
        gfx::draw_rect(bg_x, bg_y, bg_w, bg_h, {20, 20, 20, 220}, true);
        
        int text_x = (WINDOW_W - text_w) / 2;
        int text_y = WINDOW_H - 55;
        gfx::draw_text(text, font_id, text_x, text_y, gfx::colors::yellow);
        gfx::set_blended_mode(false); // Trả lại
    } else {
        notification_queue.pop_front();
        notification_start_time = 0;
    }
}

// === ĐÃ REFACTOR SANG GFX ===
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


void load_pretrained_brain(AIPlayer& ai) {
    std::cout << "Tai bo nao huan luyen san cua Meo than thien...\n";
    // Đây là các weight và bias của một bộ não đã được huấn luyện tốt
    // (Lưu ý: số lượng phải khớp chính xác với kiến trúc mạng của bạn!)
    // Ví dụ cho mạng {16, 24, 24, 4}
    const int num_weights = (16*24) + (24*24) + (24*4);
    const int num_biases = 24 + 24 + 4;
    
    // Tạo dữ liệu ngẫu nhiên giả lập cho ví dụ
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-0.5, 0.5);

    std::vector<double> pretrained_weights;
    for(int i = 0; i < num_weights; ++i) pretrained_weights.push_back(dist(gen));
    
    std::vector<double> pretrained_biases;
    for(int i = 0; i < num_biases; ++i) pretrained_biases.push_back(dist(gen));

    // Gọi hàm để nạp các giá trị này vào mạng neural của AI
    ai.brain.load_from_vectors(pretrained_weights, pretrained_biases);
    ai.target_brain.copy_from(ai.brain);
    std::cout << "Da nap xong bo nao 'Vip Pro'!\n";
}

// Hàm lưu các kinh nghiệm của "chuyên gia" (bạn) ra file
void save_expert_demonstrations(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file để lưu dữ liệu chuyên gia: " << filename << std::endl;
        return;
    }

    for (const auto& exp : expert_demonstrations) {
        // Lưu action, reward, done
        file << exp.action_idx << " " << exp.reward << " " << exp.done << " ";
        // Lưu state vector
        for (const auto& val : exp.state) {
            file << val << " ";
        }
        file << "\n";
    }
    file.close();
    std::cout << "Đã lưu " << expert_demonstrations.size() << " kinh nghiệm chuyên gia vào " << filename << std::endl;
}

// Hàm tải các kinh nghiệm chuyên gia từ file
void load_expert_demonstrations(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Không tìm thấy file dữ liệu chuyên gia. Bỏ qua." << std::endl;
        return;
    }

    expert_demonstrations.clear();
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        AIPlayer::Experience exp;
        ss >> exp.action_idx >> exp.reward >> exp.done;
        double val;
        while (ss >> val) {
            exp.state.push_back(val);
        }
        expert_demonstrations.push_back(exp);
    }
    file.close();
    std::cout << "Đã tải " << expert_demonstrations.size() << " kinh nghiệm chuyên gia từ " << filename << std::endl;
}

// === HÀM MAIN - Entry Point ===
int main(int argc, char* argv[]) {
    load_config("config.ini");
    parse_args(argc, argv);
    init(); // Đã refactor sang GFX

    achievement_manager.initialize();
    achievement_manager.load_progress(player_stats, ai_stats);
    ai_player.epsilon = config.epsilon;

    // === LOGIC VÒNG LẶP ĐÃ CẬP NHẬT CHO GFX ===
    // (Loại bỏ logic frame delay thủ công)
    while (is_running) {
        if (current_mode == GameMode::HEADLESS_TRAINING) {
            handle_input(); // Vẫn xử lý để có thể nhấn ESC thoát
            update();
            if (!ai_player.is_alive) {
                reset_game();
            }
        } else {
            // GFX xử lý clear và present
            gfx::begin_draw(gfx::colors::night_sky);

            handle_input();
            if (current_mode == GameMode::PLAYING || current_mode == GameMode::TRAINING_SESSION) {
                update();
            }
            render(); // Chỉ gọi các hàm gfx::draw_*

            gfx::end_draw();
        }
    }

    achievement_manager.save_progress(player_stats, ai_stats);
    cleanup(); // Đã refactor sang GFX
    return 0;
}

// === IMPLEMENTATION CÁC HÀM ===

void load_config(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string key, value;

        if (std::getline(ss, key, '=') && std::getline(ss, value)) {
            key.erase(0, key.find_first_not_of(" \t\n\r"));
            key.erase(key.find_last_not_of(" \t\n\r") + 1);
            value.erase(0, value.find_first_not_of(" \t\n\r"));
            value.erase(value.find_last_not_of(" \t\n\r") + 1);
            if (key.empty() || key[0] == '#') continue;

            try {
                if (key == "FPS") config.fps = std::stoi(value);
                else if (key == "LEARNING_RATE") config.learning_rate = std::stod(value);
                else if (key == "START_EPSILON") config.epsilon = std::stod(value);
                else if (key == "MIN_EPSILON") config.min_epsilon = std::stod(value);
                else if (key == "EPSILON_DECAY") config.epsilon_decay = std::stod(value);
                else if (key == "EPOCHS") config.epochs = std::stoi(value);
                else if (key == "BATCH_SIZE") config.batch_size = std::stoi(value);
            } catch (...) {
                std::cerr << "[WARN] Không thể parse: " << key << " = " << value << '\n';
            }
        }
    }
}


void parse_args(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--fps" && i + 1 < argc) config.fps = std::stoi(argv[++i]);
        else if (arg == "--lr" && i + 1 < argc) config.learning_rate = std::stod(argv[++i]);
        else if (arg == "--epsilon" && i + 1 < argc) config.epsilon = std::stod(argv[++i]);
        else if (arg == "--min-epsilon" && i + 1 < argc) config.min_epsilon = std::stod(argv[++i]);
        else if (arg == "--decay" && i + 1 < argc) config.epsilon_decay = std::stod(argv[++i]);
        else if (arg == "--epochs" && i + 1 < argc) config.epochs = std::stoi(argv[++i]);
        else if (arg == "--batch" && i + 1 < argc) config.batch_size = std::stoi(argv[++i]);
    }
}

// === ĐÃ REFACTOR SANG GFX ===
void init() {
    // GFX lo việc init SDL, TTF, Window, Renderer
    if (!gfx::init("Rắn Săn Mồi AI - Phiên Bản Hoàn Chỉnh", WINDOW_W, WINDOW_H)) {
        std::cerr << "Lỗi: Không thể khởi tạo GFX.\n";
        exit(1);
    }
    
    // GFX lo việc tải fonts
    if (!gfx::add_font("small", "CascadiaCode.ttf", 18) ||
        !gfx::add_font("medium", "CascadiaCode.ttf", 24) ||
        !gfx::add_font("large", "CascadiaCode.ttf", 48)) {
        std::cerr << "Lỗi: Không tìm thấy font CascadiaCode.ttf hoặc không thể mở." << std::endl;
        gfx::shutdown();
        exit(1);
    }
}

// === ĐÃ REFACTOR SANG GFX ===
void cleanup() {
    gfx::shutdown(); // GFX lo việc dọn dẹp
}

// THAY THẾ HÀM reset_game CŨ
void reset_game() {
    // Bước 1: Cập nhật điểm cao nhất từ ván vừa kết thúc.
    if (current_mode != GameMode::MENU) {
        if (player_stats.highest_score < player1.body.size()) {
            player_stats.highest_score = player1.body.size();
        }
        if (ai_stats.highest_score < ai_player.body.size()) {
            ai_stats.highest_score = ai_player.body.size();
        }
    }
    
    // Bước 2: Tăng số ván chơi
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

    // Bước 3: Reset các đối tượng game
    player1.reset({5, 5}, Dir::RIGHT);
    ai_player.reset({GRID_W - 5, 5}, Dir::LEFT);
    spawn_apple();

    // Bước 4: Reset các biến đếm
    game_start_time = gfx::get_ticks(); // Dùng GFX
    player_apples_this_game = 0;
    ai_apples_this_game = 0;
    mirror_match_start_time = 0;
    steps_since_apple = 0;
    
    // Bước 5: Cập nhật Epsilon
    if (ai_played_last_game) {
        if (ai_player.epsilon > config.min_epsilon) {
            ai_player.epsilon *= config.epsilon_decay;
        }
    }
}



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

// THAY THẾ HÀM update CŨ
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

        // Logic xuyên tường
        if (new_head.x < 0) new_head.x = GRID_W - 1;
        if (new_head.x >= GRID_W) new_head.x = 0;
        if (new_head.y < 0) new_head.y = GRID_H - 1;
        if (new_head.y >= GRID_H) new_head.y = 0;

        // Kiểm tra va chạm
        for (const auto& segment : snake.body) {
            if (new_head == segment) {
                snake.is_alive = false;
                return -10.0; // Phạt nặng vì tự cắn
            }
        }
        if (play_mode_cache == PlayMode::PLAYER_VS_AI) {
            for (const auto& segment : opponent_body) {
                if (new_head == segment) {
                    snake.is_alive = false;
                    return -10.0; // Phạt nặng vì đâm đối thủ
                }
            }
        }

        snake.body.push_front(new_head);
        
        double reward = 0.0;
        if (new_head == apple) {
            reward = 10.0; // Thưởng lớn
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
    // Chế độ ghi lại kinh nghiệm của chuyên gia
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

    // Các chế độ chơi bình thường
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
    uint32_t game_time = gfx::get_ticks() - game_start_time; // Dùng GFX
    
    // Cập nhật logic check mirror_match
    if (play_mode_cache == PlayMode::PLAYER_VS_AI && 
        player1.is_alive && ai_player.is_alive && 
        player1.body.size() == ai_player.body.size() && player1.body.size() > 3) 
    {
        if (mirror_match_start_time == 0) {
            mirror_match_start_time = gfx::get_ticks();
        }
    } else {
        mirror_match_start_time = 0; // Reset
    }
    
    achievement_manager.check_achievements(player_stats, player1, player_apples_this_game, ai_stats, ai_player, ai_apples_this_game, play_mode_cache, game_is_over, game_time, mirror_match_start_time);
    
    if (game_is_over && current_mode != GameMode::HEADLESS_TRAINING) {
        current_mode = GameMode::GAME_OVER;
    }
}


// === ĐÃ REFACTOR SANG GFX ===
void handle_input() {
    gfx::event e;
    while ((e = gfx::poll_event()).type != gfx::event::Type::NONE) {
        if (e.type == gfx::event::Type::QUIT) {
            is_running = false;
        }
        if (e.type == gfx::event::Type::KEYDOWN) {
            // Các phím tắt toàn cục
            if (e.key == gfx::key::S) { ai_player.brain.save_weights("snake_brain.bin"); std::cout << "Đã lưu bộ não AI!\n"; }
            if (e.key == gfx::key::L) { ai_player.brain.load_weights("snake_brain.bin"); ai_player.target_brain.copy_from(ai_player.brain); std::cout << "Đã tải bộ não AI!\n"; }
            if (e.key == gfx::key::ESCAPE) { current_mode = GameMode::MENU; }

            // Xử lý input theo từng màn hình (GameMode)
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
                        case gfx::key::T: // Phím tắt 'T'
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
                        case gfx::key::P: // 'P' for Pre-trained
                            load_pretrained_brain(ai_player);
                            break;
                        case gfx::key::I: // 'I' for Import
                            load_expert_demonstrations("expert_data.bin");
                            break;
                        case gfx::key::E: // 'E' for Export
                            save_expert_demonstrations("expert_data.bin");
                            break;
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

// === HÀM render_text ĐÃ BỊ XÓA (gfx::draw_text thay thế) ===

// === ĐÃ REFACTOR SANG GFX ===
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

// === ĐÃ REFACTOR SANG GFX ===
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

// === ĐÃ REFACTOR SANG GFX ===
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

// === ĐÃ REFACTOR SANG GFX ===
// (Hàm này giờ không clear/present nữa)
void render() {
    // gfx::begin_draw() đã clear màn hình

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
    
    // gfx::end_draw() sẽ present
}