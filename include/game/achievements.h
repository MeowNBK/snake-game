#pragma once
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <fstream>
#include <iostream>
#include <cstdint> // Cho uint32_t
#include "core/common.h" // Sử dụng các hằng số và cấu trúc chung
// #include "libs/colors.h" // KHÔNG CẦN NỮA, SẼ DÙNG GFX::COLORS

// Cấu trúc để lưu thông tin của một thành tích
struct Achievement {
    std::string id;          // ID duy nhất để lưu/tải
    std::string name;        // Tên hiển thị
    std::string description; // Mô tả
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
        achievements["PLAYER_FIRST_APPLE"] = {"PLAYER_FIRST_APPLE", "Bữa Ăn Đầu Tiên", "Lần đầu tiên ăn một quả táo."};
        achievements["PLAYER_SCORE_10"] = {"PLAYER_SCORE_10", "Đang Lớn", "Đạt độ dài 10."};
        achievements["PLAYER_SCORE_20"] = {"PLAYER_SCORE_20", "Rắn Siêu To", "Đạt độ dài 20."};
        achievements["PLAYER_EAT_50"] = {"PLAYER_EAT_50", "Kẻ Hủy Diệt Táo", "Ăn tổng cộng 50 quả táo."};
        achievements["PLAYER_VETERAN"] = {"PLAYER_VETERAN", "Lão Làng", "Chơi tổng cộng 200 ván."};

        // --- Thành tích cho AI ---
        achievements["AI_FIRST_APPLE"] = {"AI_FIRST_APPLE", "Tự Kiếm Ăn", "AI lần đầu tiên tự ăn được táo."};
        achievements["AI_SCORE_10"] = {"AI_SCORE_10", "Tốt Nghiệp Mẫu Giáo", "AI đạt độ dài 10."};
        achievements["AI_SCORE_20"] = {"AI_SCORE_20", "Skynet Trỗi Dậy", "AI đạt độ dài 20."};
        achievements["AI_SCORE_30"] = {"AI_SCORE_30", "Siêu Máy Tính", "AI đạt được độ dài 30."};
        achievements["AI_GAMES_1000"] = {"AI_GAMES_1000", "Bộ Não Ngàn Ván", "AI đã chơi và học hỏi 1000 ván."};
        achievements["AI_BEAT_PLAYER"] = {"AI_BEAT_PLAYER", "Vượt Mặt Tạo Hóa", "Kỷ lục của AI vượt qua người chơi."};
        achievements["AI_EPSILON_LOW"] = {"AI_EPSILON_LOW", "Lý Trí Lên Ngôi", "Epsilon của AI giảm xuống dưới 0.1."};
        
        // --- Thành tích Kỹ năng & Tình huống ---
        achievements["SURVIVALIST"] = {"SURVIVALIST", "Bậc Thầy Sinh Tồn", "Thắng một ván VS mà không ăn táo nào."};
        achievements["FLAWLESS_VICTORY"] = {"FLAWLESS_VICTORY", "Chiến Thắng Tuyệt Đối", "Thắng một ván VS với độ dài hơn đối thủ ít nhất 10."};
        achievements["PLAYER_SURVIVE_1_MIN"] = {"PLAYER_SURVIVE_1_MIN", "Nghệ Sĩ Né Tránh", "Người chơi sống sót được 1 phút trong một ván."};
        achievements["MIRROR_MATCH"] = {"MIRROR_MATCH", "Gương Thần", "Player và AI có cùng độ dài trong 15 giây."};
        achievements["AI_WIN_VS"] = {"AI_WIN_VS", "Kẻ Thống Trị", "AI chiến thắng người chơi trong chế độ Player vs AI."};
    }


void check_achievements(
    const PlayerStats& player_stats, const Player& player, int player_apples_this_game,
    const PlayerStats& ai_stats, const AIPlayer& ai, int ai_apples_this_game,
    PlayMode play_mode, bool game_is_over,
    uint32_t game_time_ms, uint32_t& mirror_match_start_time
) {
    // --- Thành tích cho Người chơi ---
    if (player.is_alive) {
        if (player_stats.total_apples_eaten > 0) unlock("PLAYER_FIRST_APPLE");
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
        // if (play_mode == PlayMode::AI_ONLY && (game_time_ms / 1000 >= 180)) unlock("AI_SURVIVE_3_MIN"); // (Thành tích này không có trong init)
    }
    
    if (ai_stats.highest_score > player_stats.highest_score && player_stats.highest_score > 3) {
        unlock("AI_BEAT_PLAYER");
    }

    // --- Thành tích Tình huống cho chế độ VS ---
    if (play_mode == PlayMode::PLAYER_VS_AI) {
        if (player.is_alive && ai.is_alive && player.body.size() == ai.body.size() && player.body.size() > 3) {
            if (mirror_match_start_time == 0) {
                mirror_match_start_time = game_time_ms + 1; // +1 để tránh 0
            } else if (game_time_ms - mirror_match_start_time >= 15000) {
                unlock("MIRROR_MATCH");
            }
        } else {
            mirror_match_start_time = 0;
        }

        if (game_is_over) {
            if (!ai.is_alive && player.is_alive) {
                if (player_apples_this_game == 0) unlock("SURVIVALIST");
                if (player.body.size() >= ai.body.size() + 10) unlock("FLAWLESS_VICTORY");
            } else if (!player.is_alive && ai.is_alive) {
                unlock("AI_WIN_VS");
            }
        }
    }
}


    // Mở khóa một thành tích và đẩy vào hàng chờ thông báo
    void unlock(const std::string& id) {
        if (achievements.count(id) && !achievements[id].unlocked) {
            achievements[id].unlocked = true;
            notification_queue.push_back(achievements[id]);
            std::cout << "Thành tích đã mở khóa: " << achievements[id].name << std::endl;
        }
    }

    // === ĐÃ THAY ĐỔI CHỮ KÝ HÀM ===
    void render_notifications(const std::string& font_id);
    
    // Hàm để lưu và tải tiến trình thành tích
    void save_progress(const PlayerStats& ps, const PlayerStats& as) {
        std::ofstream file("achievements_progress.txt");
        file << ps.games_played << " " << ps.total_apples_eaten << " " << ps.highest_score << "\n";
        file << as.games_played << " " << as.total_apples_eaten << " " << as.highest_score << "\n";
        for (const auto& pair : achievements) {
            if (pair.second.unlocked) {
                file << pair.first << "\n";
            }
        }
        file.close();
    }
    
    void load_progress(PlayerStats& ps, PlayerStats& as) {
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

    // === ĐÃ THAY ĐỔI CHỮ KÝ HÀM ===
    void render_list_screen(const std::string& font_title_id, 
                              const std::string& font_text_id, 
                              const std::string& font_small_id);

private:
    std::map<std::string, Achievement> achievements;
    std::deque<Achievement> notification_queue;
    uint32_t notification_start_time = 0;
};