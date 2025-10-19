#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
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
// #include <windows.h>

// Các file header tự tạo
#include "common.h"
#include "ai_player.h"
#include "achievements.h"
#include "libs/colors.h"

// === Cấu trúc Config & Các biến toàn cục ===

struct Config {
    int fps = 30;
    double learning_rate = 0.001;
    // double epsilon = 0.9;
    double epsilon = 0.0;
    // double min_epsilon = 0.05;
    double min_epsilon = 0.0;
    double epsilon_decay = 0.9995;
    int epochs = 30;
    int batch_size = 32;
};

// Lớp Player cho người chơi
// class Player {
// public:
//     std::deque<Point> body;
//     Dir direction;
//     bool is_alive;
    
//     void reset(Point start_pos, Dir start_dir) {
//         body.clear();
//         body.push_front(start_pos);
//         Point second_segment = start_pos;
//         switch (start_dir) {
//             case Dir::UP:    second_segment.y++; break;
//             case Dir::DOWN:  second_segment.y--; break;
//             case Dir::LEFT:  second_segment.x++; break;
//             case Dir::RIGHT: second_segment.x--; break;
//         }
//         body.push_back(second_segment);
//         direction = start_dir;
//         is_alive = true;
//     }
// };

// SDL
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font_medium = nullptr;
TTF_Font* font_large = nullptr;
TTF_Font* font_small = nullptr;

// Trạng thái game
bool is_running = true;
// enum class GameMode { MENU, PLAYING, GAME_OVER, ACHIEVEMENT_LIST, HEADLESS_TRAINING };
GameMode current_mode = GameMode::MENU;

// Chế độ chơi cụ thể, dùng để xác định logic
// enum class PlayMode { PLAYER_ONLY, AI_ONLY, PLAYER_VS_AI };
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
Uint32 game_start_time = 0;
Uint32 mirror_match_start_time = 0;
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
void render_text(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font, bool is_centered = false);
void render_menu();
void render_game();
void render_game_over();
void spawn_apple();

// --- Triển khai các hàm của AchievementManager cần SDL ---
// Đặt ở đây vì chúng cần hàm render_text và các biến toàn cục
void AchievementManager::render_notifications(SDL_Renderer* renderer, TTF_Font* font) {
    if (notification_queue.empty()) return;
    Uint32 current_time = SDL_GetTicks();
    if (notification_start_time == 0) {
        notification_start_time = current_time;
    }
    if (current_time - notification_start_time < 3000) {
        const Achievement& ach = notification_queue.front();
        std::string text = "Thành Tích Mới: " + ach.name;
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), Colors::Yellow);
        if(!surface) return;
        int text_w = surface->w; int text_h = surface->h;
        SDL_Rect bg_rect = {(WINDOW_W - text_w - 20) / 2, WINDOW_H - 60, text_w + 20, text_h + 10};
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 20, 20, 20, 220);
        SDL_RenderFillRect(renderer, &bg_rect);
        SDL_Rect dst_rect = {(WINDOW_W - text_w) / 2, WINDOW_H - 55, text_w, text_h};
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_RenderCopy(renderer, texture, nullptr, &dst_rect);
        SDL_FreeSurface(surface); SDL_DestroyTexture(texture);
    } else {
        notification_queue.pop_front();
        notification_start_time = 0;
    }
}

void AchievementManager::render_list_screen(SDL_Renderer* renderer, TTF_Font* font_title, TTF_Font* font_text) {
    render_text("Phòng Truyền Thống", 0, 50, Colors::NeonCyan, font_title, true);
    int y_pos = 120;
    int unlocked_count = 0;
    for (const auto& pair : achievements) {
        const Achievement& ach = pair.second;
        if (ach.unlocked) {
            render_text(ach.name, 50, y_pos, Colors::VaporNeonGreen, font_text, false);
            render_text(ach.description, 70, y_pos + 25, Colors::White, font_small);
            unlocked_count++;
        } else {
            render_text("Thành Tích Bí Ẩn", 50, y_pos, Colors::Gray50, font_text, false);
            render_text("????????????????", 70, y_pos + 25, Colors::DarkGray, font_small);
        }
        y_pos += 60;
    }
    std::string progress_text = "Tiến độ: " + std::to_string(unlocked_count) + "/" + std::to_string(achievements.size());
    render_text(progress_text, 0, 20, Colors::PastelLilac, font_medium, true);
    render_text("Nhấn ESC để quay lại Menu", 0, WINDOW_H - 40, Colors::Cyan, font_small, true);
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

// Thêm 2 hàm này vào main.cpp

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
    init();

    // SetConsoleOutputCP(65001);
    // SetConsoleCP(65001);

    achievement_manager.initialize();
    achievement_manager.load_progress(player_stats, ai_stats);
    ai_player.epsilon = config.epsilon;

    Uint32 frame_start;
    int frame_time;

    while (is_running) {
        if (current_mode == GameMode::HEADLESS_TRAINING) {
            handle_input(); // Vẫn xử lý để có thể nhấn ESC thoát
            update();
            if (!ai_player.is_alive) {
                reset_game();
            }
        } else {
            frame_start = SDL_GetTicks();
            handle_input();
            if (current_mode == GameMode::PLAYING || current_mode == GameMode::TRAINING_SESSION) {
                update();
            }
            render();
            int current_frame_delay = (config.fps > 0) ? (1000 / config.fps) : 0;
            frame_time = SDL_GetTicks() - frame_start;
            if (current_frame_delay > frame_time) {
                SDL_Delay(current_frame_delay - frame_time);
            }
        }
    }

    achievement_manager.save_progress(player_stats, ai_stats);
    cleanup();
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


void init() {
    SDL_Init(SDL_INIT_VIDEO);
    TTF_Init();
    window = SDL_CreateWindow("Snake AI", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_W, WINDOW_H, SDL_WINDOW_SHOWN);
    
    // 3. THÊM 2 DÒNG NÀY NGAY BÊN DƯỚI:
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_SetWindowTitle(window, "Rắn Săn Mồi AI - Phiên Bản Hoàn Chỉnh"); // <-- Thêm dòng này
    font_small = TTF_OpenFont("CascadiaCode.ttf", 18);
    font_medium = TTF_OpenFont("CascadiaCode.ttf", 24);
    font_large = TTF_OpenFont("CascadiaCode.ttf", 48);
    if (!font_small || !font_medium || !font_large) {
        std::cerr << "Lỗi: Không tìm thấy font CascadiaCode.ttf hoặc không thể mở." << std::endl;
        exit(1);
    }
}

void cleanup() {
    TTF_CloseFont(font_small);
    TTF_CloseFont(font_medium);
    TTF_CloseFont(font_large);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// THAY THẾ HÀM reset_game CŨ
void reset_game() {
    // Bước 1: Cập nhật điểm cao nhất từ ván vừa kết thúc.
    // Chỉ cập nhật khi không phải đang ở menu (tránh reset lúc mới vào game)
    if (current_mode != GameMode::MENU) {
        if (player_stats.highest_score < player1.body.size()) {
            player_stats.highest_score = player1.body.size();
        }
        if (ai_stats.highest_score < ai_player.body.size()) {
            ai_stats.highest_score = ai_player.body.size();
        }
    }
    
    // Bước 2: Tăng số ván chơi một cách an toàn và chính xác.
    // Xác định chế độ chơi nào vừa kết thúc để tăng bộ đếm cho đúng.
    bool ai_played_last_game = (play_mode_cache == PlayMode::AI_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI || current_mode == GameMode::HEADLESS_TRAINING);
    bool player_played_last_game = (play_mode_cache == PlayMode::PLAYER_ONLY || play_mode_cache == PlayMode::PLAYER_VS_AI || current_mode == GameMode::TRAINING_SESSION);

    if (current_mode != GameMode::MENU) { // Không tính lần reset đầu tiên khi vào game
        if (player_played_last_game) {
            player_stats.games_played++;
        }
        if (ai_played_last_game) {
            ai_stats.games_played++;
        }
    }

    // Bước 3: Reset các đối tượng game về trạng thái ban đầu
    player1.reset({5, 5}, Dir::RIGHT);
    ai_player.reset({GRID_W - 5, 5}, Dir::LEFT);
    spawn_apple();

    // Bước 4: Reset các biến đếm của ván mới
    game_start_time = SDL_GetTicks();
    player_apples_this_game = 0;
    ai_apples_this_game = 0;
    mirror_match_start_time = 0;
    steps_since_apple = 0;
    
    // Bước 5: Cập nhật Epsilon cho AI (chỉ khi AI có chơi)
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
        // Dùng khoảng cách Euclidean (chính xác hơn)
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
        // for (const auto& segment : snake.body) {
        //     if (new_head == segment) {
        //         snake.is_alive = false;
        //         return -10.0; // Phạt nặng vì tự cắn
        //     }
        // }
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
                steps_since_apple = 0; // Reset đồng hồ
            } else {
                player_apples_this_game++;
                player_stats.total_apples_eaten++;
            }
        } else {
            snake.body.pop_back();
            // Hệ thống thưởng phạt mới: chỉ có thưởng/phạt khi chết và ăn táo
            // Loại bỏ các hình phạt nhỏ để AI tự do khám phá hơn
            // reward = 0; 
                double new_dist = std::hypot(new_head.x - apple.x, new_head.y - apple.y);
                reward = (new_dist < old_dist) ? 0.1 : -0.2; // Thưởng khi gần hơn, phạt khi xa hơn

        }

        // Trừng phạt vì đi lòng vòng không hiệu quả
        if (is_ai && steps_since_apple > GRID_W * GRID_H * 1.5) {
            snake.is_alive = false;
            reward = -10.0; // Hình phạt tương đương đâm tường
        }
        return reward;
    };
    // Chế độ ghi lại kinh nghiệm của chuyên gia (người chơi)
    if (current_mode == GameMode::TRAINING_SESSION) {
        if (!player1.is_alive) { // Nếu chết thì quay về menu
            current_mode = GameMode::MENU;
            return;
        }
        std::deque<Point> empty_body;
        std::vector<double> state_before_move = ai_player.get_state_vector(apple, empty_body); // Dùng AI để lấy state cho player
        int action_taken = static_cast<int>(player1.direction);
        move_snake(player1, false, empty_body); // Di chuyển player
        expert_demonstrations.push_back({state_before_move, action_taken, 1.0, {}, false});
        return; // Kết thúc update cho frame này
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
    Uint32 game_time = SDL_GetTicks() - game_start_time;
    achievement_manager.check_achievements(player_stats, player1, player_apples_this_game, ai_stats, ai_player, ai_apples_this_game, play_mode_cache, game_is_over, game_time, mirror_match_start_time);
    
    if (game_is_over && current_mode != GameMode::HEADLESS_TRAINING) {
        current_mode = GameMode::GAME_OVER;
    }
}


// THAY THẾ HÀM handle_input CŨ
// TRONG FILE main.cpp
// HÃY THAY THẾ HÀM handle_input CŨ BẰNG PHIÊN BẢN NÀY

void handle_input() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            is_running = false;
        }
        if (e.type == SDL_KEYDOWN) {
            // Các phím tắt toàn cục hoạt động ở mọi nơi
            if (e.key.keysym.sym == SDLK_s) { ai_player.brain.save_weights("snake_brain.bin"); std::cout << "Đã lưu bộ não AI!\n"; }
            if (e.key.keysym.sym == SDLK_l) { ai_player.brain.load_weights("snake_brain.bin"); ai_player.target_brain.copy_from(ai_player.brain); std::cout << "Đã tải bộ não AI!\n"; }
            if (e.key.keysym.sym == SDLK_ESCAPE) { current_mode = GameMode::MENU; }

            // Xử lý input theo từng màn hình (GameMode)
            switch (current_mode) {
                case GameMode::MENU:
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:   selected_menu_item = (selected_menu_item - 1 + 6) % 6; break; // Có 6 mục
                        case SDLK_DOWN: selected_menu_item = (selected_menu_item + 1) % 6; break;
                        case SDLK_RETURN: case SDLK_KP_ENTER:
                            if      (selected_menu_item == 0) { play_mode_cache = PlayMode::PLAYER_ONLY; current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 1) { play_mode_cache = PlayMode::AI_ONLY;     current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 2) { play_mode_cache = PlayMode::PLAYER_VS_AI;  current_mode = GameMode::PLAYING; reset_game(); }
                            else if (selected_menu_item == 3) { play_mode_cache = PlayMode::AI_ONLY;     current_mode = GameMode::HEADLESS_TRAINING; reset_game(); }
                            else if (selected_menu_item == 4) { play_mode_cache = PlayMode::PLAYER_ONLY; current_mode = GameMode::TRAINING_SESSION; reset_game(); } // Chế độ Dạy AI
                            else if (selected_menu_item == 5) { current_mode = GameMode::ACHIEVEMENT_LIST; }
                            break;
                            case SDLK_t: // Phím tắt để huấn luyện từ dữ liệu chuyên gia
                                if (!expert_demonstrations.empty()) {
                                    // Trộn dữ liệu một lần trước khi train
                                    std::shuffle(
                                        expert_demonstrations.begin(),
                                        expert_demonstrations.end(),
                                        std::mt19937{std::random_device{}()}
                                    );

                                    // Gọi hàm train mới (đã hỗ trợ batch_size)
                                    ai_player.train_from_demonstrations(
                                        expert_demonstrations,
                                        config.epochs,           // số epoch
                                        config.learning_rate,    // learning rate
                                        config.batch_size        // batch size
                                    );

                                    expert_demonstrations.clear();
                                    std::cout << "Đã học xong dữ liệu từ chuyên gia. Nhấn S để lưu lại bộ não mới.\n";
                                } else {
                                    std::cout << "Chưa có dữ liệu từ chuyên gia để huấn luyện! Hãy vào chế độ 'AI Training'.\n";
                                }
                                break;
                            case SDLK_p: // 'P' for Pre-trained
                                load_pretrained_brain(ai_player);
                                break;
                            case SDLK_i:
                                load_expert_demonstrations("expert_data.bin");
                                break;
                            case SDLK_e:
                                save_expert_demonstrations("expert_data.bin");
                                break;

                    }
                    break;

                case GameMode::PLAYING:
                case GameMode::TRAINING_SESSION:
                    if (e.key.keysym.sym == SDLK_r) { reset_game(); }
                    switch (e.key.keysym.sym) {
                        case SDLK_UP:    if (player1.direction != Dir::DOWN) player1.direction = Dir::UP; break;
                        case SDLK_DOWN:  if (player1.direction != Dir::UP) player1.direction = Dir::DOWN; break;
                        case SDLK_LEFT:  if (player1.direction != Dir::RIGHT) player1.direction = Dir::LEFT; break;
                        case SDLK_RIGHT: if (player1.direction != Dir::LEFT) player1.direction = Dir::RIGHT; break;
                    }
                    break;

                case GameMode::GAME_OVER:
                    if (e.key.keysym.sym == SDLK_r) { current_mode = GameMode::PLAYING; reset_game(); }
                    break;
                
                default: break; // Các mode khác chỉ cần ESC (đã xử lý ở trên)
            }
        }
    }
}


void render_text(const std::string& text, int x, int y, SDL_Color color, TTF_Font* font, bool is_centered) {
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect dst = {x, y, surface->w, surface->h};
    if (is_centered) dst.x = (WINDOW_W - dst.w) / 2;
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void render_menu() {
    // Thêm lựa chọn mới vào danh sách
    const std::vector<std::string> menu_items = {
        "Chơi một mình",
        "Xem AI chơi",
        "Solo với AI",
        "Training Trong Mơ (Siêu Tốc)",
        "Dạy AI Chơi (Snake Trainer)", // <-- LỰA CHỌN MỚI
        "Xem Thành Tích"
    };
    const int menu_item_count = menu_items.size();

    render_text("Rắn Săn Mồi AI", 0, 80, Colors::NeonCyan, font_large, true);

    // Vòng lặp vẽ các mục trong menu
    for (int i = 0; i < menu_item_count; ++i) {
        bool is_selected = (i == selected_menu_item);
        SDL_Color color = is_selected ? Colors::NeonYellow : Colors::White;

        // Vẽ nền highlight cho mục đang được chọn
        if (is_selected) {
            SDL_Surface* s = TTF_RenderUTF8_Blended(font_medium, menu_items[i].c_str(), color);
            if(s) {
                SDL_Rect bg = {(WINDOW_W - s->w - 20) / 2, 180 + i * 50 - 5, s->w + 20, s->h + 10};
                Colors::SetDrawColor(renderer, Colors::TwilightPurple);
                SDL_RenderFillRect(renderer, &bg);
                SDL_FreeSurface(s);
            }
        }
        render_text(menu_items[i], 0, 180 + i * 50, color, font_medium, true);
    }

    // Cập nhật dòng hướng dẫn ở cuối
    render_text("L/S: Tải/Lưu Não | T: Train từ Dữ liệu | R: Chơi Lại | ESC: Menu", 0, WINDOW_H - 40, Colors::Gray80, font_small, true);
}


void render_game() {
    if (play_mode_cache != PlayMode::AI_ONLY) {
        for(const auto& p : player1.body) {
            SDL_Rect rect = {p.x*CELL_SIZE, p.y*CELL_SIZE, CELL_SIZE, CELL_SIZE};
            Colors::SetDrawColor(renderer, player1.is_alive ? Colors::VaporNeonGreen : Colors::Gray50);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    if (play_mode_cache != PlayMode::PLAYER_ONLY) {
        for(const auto& p : ai_player.body) {
            SDL_Rect rect = {p.x*CELL_SIZE, p.y*CELL_SIZE, CELL_SIZE, CELL_SIZE};
            Colors::SetDrawColor(renderer, ai_player.is_alive ? Colors::VaporPurple : Colors::Gray50);
            SDL_RenderFillRect(renderer, &rect);
        }
    }
    SDL_Rect apple_rect = {apple.x*CELL_SIZE, apple.y*CELL_SIZE, CELL_SIZE, CELL_SIZE};
    Colors::SetDrawColor(renderer, Colors::NeonOrange);
    SDL_RenderFillRect(renderer, &apple_rect);
    std::string score_text;
    if (play_mode_cache == PlayMode::PLAYER_VS_AI) score_text = "Player: " + std::to_string(player1.body.size()) + " | AI: " + std::to_string(ai_player.body.size());
    else if (play_mode_cache == PlayMode::PLAYER_ONLY) score_text = "Điểm: " + std::to_string(player1.body.size());
    else score_text = "Điểm AI: " + std::to_string(ai_player.body.size());
    render_text(score_text, 10, 10, Colors::White, font_medium);
}

void render_game_over() {
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 20, 20, 20, 210);
    SDL_Rect overlay = {0, 0, WINDOW_W, WINDOW_H};
    SDL_RenderFillRect(renderer, &overlay);
    std::string winner_text = "";
    if (play_mode_cache == PlayMode::PLAYER_VS_AI) {
        if (!player1.is_alive && !ai_player.is_alive) winner_text = "Hòa!";
        else if (!player1.is_alive) winner_text = "AI Thắng!";
        else winner_text = "Bạn Thắng!";
    } else {
        winner_text = "GAME OVER";
    }
    render_text(winner_text, 0, WINDOW_H / 2 - 100, Colors::NeonOrange, font_large, true);
    std::string score_info;
    if (play_mode_cache == PlayMode::PLAYER_ONLY) score_info = "Điểm cuối: " + std::to_string(player1.body.size());
    else if (play_mode_cache == PlayMode::AI_ONLY) score_info = "Điểm cuối: " + std::to_string(ai_player.body.size());
    else score_info = "Tỉ số: Player " + std::to_string(player1.body.size()) + " - " + std::to_string(ai_player.body.size()) + " AI";
    render_text(score_info, 0, WINDOW_H / 2, Colors::White, font_medium, true);
    render_text("Nhấn 'R' để chơi lại | 'ESC' để về Menu", 0, WINDOW_H / 2 + 50, Colors::Cyan, font_medium, true);
}


void render() {
    Colors::SetDrawColor(renderer, Colors::NightSky);
    SDL_RenderClear(renderer);

    switch (current_mode) {
        case GameMode::MENU: render_menu(); break;
        case GameMode::PLAYING: render_game(); break;
        case GameMode::GAME_OVER: {
            render_game();
            render_game_over();
            break;
        }
        case GameMode::ACHIEVEMENT_LIST:{
            achievement_manager.render_list_screen(renderer, font_large, font_medium);
            break;
        }
        case GameMode::HEADLESS_TRAINING: {             
            render_text("TRAINING TRONG MƠ...", 0, WINDOW_H / 2 - 80, Colors::White, font_large, true);
                std::string progress = "Ván chơi: " + std::to_string(ai_stats.games_played) + " | Điểm cao nhất: " + std::to_string(ai_stats.highest_score);
                render_text(progress, 0, WINDOW_H / 2, Colors::Cyan, font_medium, true);
                std::string epsilon_text = "Epsilon: " + std::to_string(ai_player.epsilon);
                render_text(epsilon_text, 0, WINDOW_H / 2 + 40, Colors::Cyan, font_medium, true);
                render_text("Nhấn ESC để quay lại Menu", 0, WINDOW_H - 50, Colors::Gray80, font_small, true);
            break;
        }
        case GameMode::TRAINING_SESSION: {
            // Vẽ như game bình thường để người chơi “dạy AI”
            render_game();
            // Và có thể hiển thị thêm dòng hướng dẫn
            render_text("Training Session: Điểu khiển để tạo data cho AI", 0, WINDOW_H - 40, Colors::Cyan, font_small, true);
            break;
        }
    }

    achievement_manager.render_notifications(renderer, font_medium);
    SDL_RenderPresent(renderer);
}