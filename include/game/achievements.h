#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "core/common.h"  // Sử dụng các hằng số và cấu trúc chung
#include "misc/colors.h"

// void render_text(const std::string& text, int x, int y, struct SDL_Color
// color, struct _TTF_Font* font, bool is_centered);

// Cấu trúc để lưu thông tin của một thành tích
struct Achievement {
    std::string id;           // ID duy nhất để lưu/tải
    std::string name;         // Tên hiển thị
    std::string description;  // Mô tả
    bool unlocked = false;
};

// Cấu trúc để theo dõi các chỉ số thống kê
struct PlayerStats {
    int games_played = 0;
    int total_apples_eaten = 0;
    size_t highest_score = 3;
    int scroll_offset = 0;
};

class AchievementManager {
   public:
    int scroll_offset = 0;
    // Khởi tạo danh sách tất cả các thành tích trong game
    void initialize() {
        // --- Thành tích cho Người chơi ---
        achievements["PLAYER_FIRST_APPLE"] = {"PLAYER_FIRST_APPLE",
                                              "Bữa Ăn Đầu Tiên",
                                              "Lần đầu tiên ăn một quả táo."};
        achievements["PLAYER_SCORE_10"] = {"PLAYER_SCORE_10", "Đang Lớn",
                                           "Đạt độ dài 10."};
        achievements["PLAYER_SCORE_20"] = {"PLAYER_SCORE_20", "Rắn Siêu To",
                                           "Đạt độ dài 20."};
        achievements["PLAYER_EAT_50"] = {"PLAYER_EAT_50", "Kẻ Hủy Diệt Táo",
                                         "Ăn tổng cộng 50 quả táo."};
        achievements["PLAYER_VETERAN"] = {"PLAYER_VETERAN", "Lão Làng",
                                          "Chơi tổng cộng 200 ván."};

        // --- Thành tích cho AI ---
        achievements["AI_FIRST_APPLE"] = {"AI_FIRST_APPLE", "Tự Kiếm Ăn",
                                          "AI lần đầu tiên tự ăn được táo."};
        achievements["AI_SCORE_10"] = {"AI_SCORE_10", "Tốt Nghiệp Mẫu Giáo",
                                       "AI đạt độ dài 10."};
        achievements["AI_SCORE_20"] = {"AI_SCORE_20", "Skynet Trỗi Dậy",
                                       "AI đạt độ dài 20."};
        achievements["AI_SCORE_30"] = {"AI_SCORE_30", "Siêu Máy Tính",
                                       "AI đạt được độ dài 30."};
        achievements["AI_GAMES_1000"] = {"AI_GAMES_1000", "Bộ Não Ngàn Ván",
                                         "AI đã chơi và học hỏi 1000 ván."};
        achievements["AI_BEAT_PLAYER"] = {"AI_BEAT_PLAYER", "Vượt Mặt Tạo Hóa",
                                          "Kỷ lục của AI vượt qua người chơi."};
        achievements["AI_EPSILON_LOW"] = {
            "AI_EPSILON_LOW", "Lý Trí Lên Ngôi",
            "Epsilon của AI giảm xuống dưới 0.1."};

        // --- Thành tích Kỹ năng & Tình huống ---
        achievements["SURVIVALIST"] = {"SURVIVALIST", "Bậc Thầy Sinh Tồn",
                                       "Thắng một ván VS mà không ăn táo nào."};
        achievements["FLAWLESS_VICTORY"] = {
            "FLAWLESS_VICTORY", "Chiến Thắng Tuyệt Đối",
            "Thắng một ván VS với độ dài hơn đối thủ ít nhất 10."};
        achievements["PLAYER_SURVIVE_1_MIN"] = {
            "PLAYER_SURVIVE_1_MIN", "Nghệ Sĩ Né Tránh",
            "Người chơi sống sót được 1 phút trong một ván."};
        achievements["MIRROR_MATCH"] = {
            "MIRROR_MATCH", "Gương Thần",
            "Player và AI có cùng độ dài trong 15 giây."};
        achievements["AI_WIN_VS"] = {
            "AI_WIN_VS", "Kẻ Thống Trị",
            "AI chiến thắng người chơi trong chế độ Player vs AI."};
    }

    // Kiểm tra các điều kiện để mở khóa thành tích
    // Hàm này cần được gọi mỗi frame trong hàm update() của main.cpp
    // TRONG FILE achievements.h
    // HÃY THAY THẾ HÀM check_achievements() CŨ BẰNG HÀM NÀY

    void check_achievements(const PlayerStats &player_stats,
                            const Player &player, int player_apples_this_game,
                            const PlayerStats &ai_stats, const AIPlayer &ai,
                            int ai_apples_this_game, PlayMode play_mode,
                            bool game_is_over, Uint32 game_time_ms,
                            Uint32 &mirror_match_start_time) {
        // --- Thành tích cho Người chơi ---
        if (player.is_alive) {
            if (player_stats.total_apples_eaten > 0)
                unlock("PLAYER_FIRST_APPLE");
            if (player.body.size() >= 10) unlock("PLAYER_SCORE_10");
            if (player.body.size() >= 20) unlock("PLAYER_SCORE_20");
            if (player_stats.total_apples_eaten >= 50) unlock("PLAYER_EAT_50");
            if (player_stats.games_played >= 200) unlock("PLAYER_VETERAN");
            if (game_time_ms / 1000 >= 60) unlock("PLAYER_SURVIVE_1_MIN");
        }

        // --- Thành tích cho AI ---
        if (ai.is_alive) {
            if (ai_stats.total_apples_eaten > 0) unlock("AI_FIRST_APPLE");
            if (ai.body.size() >= 10) unlock("AI_SCORE_10");
            if (ai.body.size() >= 20) unlock("AI_SCORE_20");
            if (ai.body.size() >= 30) unlock("AI_SCORE_30");
            if (ai_stats.games_played >= 1000) unlock("AI_GAMES_1000");
            if (ai.epsilon <= 0.1) unlock("AI_EPSILON_LOW");
            if (play_mode == PlayMode::AI_ONLY && (game_time_ms / 1000 >= 180))
                unlock("AI_SURVIVE_3_MIN");
        }

        if (ai_stats.highest_score > player_stats.highest_score &&
            player_stats.highest_score > 3) {
            unlock("AI_BEAT_PLAYER");
        }

        // --- Thành tích Tình huống cho chế độ VS ---
        if (play_mode == PlayMode::PLAYER_VS_AI) {
            if (player.is_alive && ai.is_alive &&
                player.body.size() == ai.body.size() &&
                player.body.size() > 3) {
                if (mirror_match_start_time == 0) {
                    mirror_match_start_time = SDL_GetTicks();
                } else if (SDL_GetTicks() - mirror_match_start_time >= 15000) {
                    unlock("MIRROR_MATCH");
                }
            } else {
                mirror_match_start_time = 0;
            }

            if (game_is_over) {
                if (!ai.is_alive && player.is_alive) {
                    if (player_apples_this_game == 0) unlock("SURVIVALIST");
                    if (player.body.size() >= ai.body.size() + 10)
                        unlock("FLAWLESS_VICTORY");
                } else if (!player.is_alive && ai.is_alive) {
                    unlock("AI_WIN_VS");
                }
            }
        }
    }

    // Mở khóa một thành tích và đẩy vào hàng chờ thông báo
    void unlock(const std::string &id) {
        if (achievements.count(id) && !achievements[id].unlocked) {
            achievements[id].unlocked = true;
            notification_queue.push_back(achievements[id]);
            std::cout << "Thành tích đã mở khóa: " << achievements[id].name
                      << std::endl;
        }
    }

    // // Hiển thị thông báo thành tích trên màn hình
    // void render_notifications(SDL_Renderer* renderer, TTF_Font* font) {
    //     if (notification_queue.empty()) return;

    //     Uint32 current_time = SDL_GetTicks();
    //     if (notification_start_time == 0) {
    //         notification_start_time = current_time;
    //     }

    //     // Hiển thị thông báo trong 3 giây
    //     if (current_time - notification_start_time < 3000) {
    //         const Achievement& ach = notification_queue.front();
    //         std::string text = "Thanh tich moi: " + ach.name;

    //         SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(),
    //         Colors::Yellow); SDL_Texture* texture =
    //         SDL_CreateTextureFromSurface(renderer, surface); int text_w =
    //         surface->w; int text_h = surface->h;

    //         // Vẽ một cái nền mờ phía sau
    //         SDL_Rect bg_rect = {(WINDOW_W - text_w - 20) / 2, WINDOW_H - 60,
    //         text_w + 20, text_h + 20}; SDL_SetRenderDrawBlendMode(renderer,
    //         SDL_BLENDMODE_BLEND); // Bật chế độ trong suốt
    //         SDL_SetRenderDrawColor(renderer, 20, 20, 20, 200); // Màu đen mờ
    //         SDL_RenderFillRect(renderer, &bg_rect);

    //         // Vẽ chữ đè lên trên
    //         SDL_Rect dst_rect = {(WINDOW_W - text_w) / 2, WINDOW_H - 50,
    //         text_w, text_h}; SDL_RenderCopy(renderer, texture, nullptr,
    //         &dst_rect);

    //         SDL_FreeSurface(surface);
    //         SDL_DestroyTexture(texture);
    //     } else {
    //         notification_queue.pop_front();
    //         notification_start_time = 0; // Reset timer cho thông báo tiếp
    //         theo
    //     }
    // }

    void render_notifications(SDL_Renderer *renderer, TTF_Font *font);

    // Hàm để lưu và tải tiến trình thành tích
    void save_progress(const PlayerStats &ps, const PlayerStats &as) {
        std::ofstream file("achievements_progress.txt");
        file << ps.games_played << " " << ps.total_apples_eaten << " "
             << ps.highest_score << "\n";
        file << as.games_played << " " << as.total_apples_eaten << " "
             << as.highest_score << "\n";
        for (const auto &pair : achievements) {
            if (pair.second.unlocked) {
                file << pair.first << "\n";
            }
        }
        file.close();
    }

    void load_progress(PlayerStats &ps, PlayerStats &as) {
        std::ifstream file("achievements_progress.txt");
        if (!file.is_open()) return;
        file >> ps.games_played >> ps.total_apples_eaten >> ps.highest_score;
        file >> as.games_played >> as.total_apples_eaten >> as.highest_score;
        std::string id;
        while (file >> id) {
            if (achievements.count(id)) {
                achievements[id].unlocked = true;
            }
        }
        file.close();
        std::cout << "Đã tải tiến trình thành tích" << std::endl;
    }

    // THÊM HÀM MỚI NÀY VÀO PHẦN public CỦA LỚP AchievementManager
    // void render_list_screen(SDL_Renderer* renderer, TTF_Font* font) {
    //     // Render tiêu đề
    //     render_text_helper(renderer, font, "PHONG TRUYEN THONG", WINDOW_W /
    //     2, 50, Colors::NeonYellow, true);

    //     int y_pos = 120;
    //     for (const auto& pair : achievements) {
    //         const Achievement& ach = pair.second;
    //         if (ach.unlocked) {
    //             // Hiển thị đầy đủ nếu đã mở khóa
    //             render_text_helper(renderer, font, ach.name, 50, y_pos,
    //             Colors::VaporNeonGreen); render_text_helper(renderer, font,
    //             ach.description, 50, y_pos + 25, Colors::White);
    //         } else {
    //             // Hiển thị mờ và ẩn mô tả nếu chưa mở khóa
    //             render_text_helper(renderer, font, ach.name, 50, y_pos,
    //             Colors::Gray50); render_text_helper(renderer, font,
    //             "????????????????", 50, y_pos + 25, Colors::DarkGray);
    //         }
    //         y_pos += 60;
    //     }
    //     render_text_helper(renderer, font, "Nhan ESC de quay lai Menu",
    //     WINDOW_W / 2, WINDOW_H - 40, Colors::Cyan, true);
    // }

    void render_list_screen(SDL_Renderer *renderer, TTF_Font *font_title,
                            TTF_Font *font_text);

    // Thêm một hàm helper để render text cho gọn (có thể đặt ở file riêng hoặc
    // trong game.cpp) void render_text_helper(SDL_Renderer* r, TTF_Font* f,
    // const std::string& text, int x, int y, SDL_Color c, bool centered =
    // false) {
    //     SDL_Surface* surface = TTF_RenderUTF8_Blended(f, text.c_str(), c);
    //     if (!surface) return;
    //     SDL_Texture* texture = SDL_CreateTextureFromSurface(r, surface);
    //     SDL_Rect dst = {x, y, surface->w, surface->h};
    //     if (centered) dst.x = (WINDOW_W - dst.w) / 2;
    //     SDL_RenderCopy(r, texture, nullptr, &dst);
    //     SDL_DestroyTexture(texture);
    //     SDL_FreeSurface(surface);
    // }

   private:
    std::map<std::string, Achievement> achievements;
    std::deque<Achievement> notification_queue;
    Uint32 notification_start_time = 0;
};