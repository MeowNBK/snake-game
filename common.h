#pragma once
#include <deque> // <--- ĐÃ THÊM THƯ VIỆN CÒN THIẾU

// === Cấu hình Grid & Window ===
const int CELL_SIZE = 20;
const int GRID_W = 30;
const int GRID_H = 24;
const int WINDOW_W = CELL_SIZE * GRID_W;
const int WINDOW_H = CELL_SIZE * GRID_H;

// === Các enum dùng chung ===
enum class Dir { UP, DOWN, LEFT, RIGHT };
enum class GameMode { MENU, PLAYING, GAME_OVER, ACHIEVEMENT_LIST, HEADLESS_TRAINING, TRAINING_SESSION };
enum class PlayMode { PLAYER_ONLY, AI_ONLY, PLAYER_VS_AI };

struct Point {
    int x, y;
    bool operator==(const Point& o) const {
        return x == o.x && y == o.y;
    }
};

// === Lớp Player cho người chơi ===
class Player {
public: // <--- THÊM public: ĐỂ CÁC FILE KHÁC CÓ THỂ TRUY CẬP
    std::deque<Point> body;
    Dir direction;
    bool is_alive;
    
    void reset(Point start_pos, Dir start_dir) {
        std::cout << "Player 1 body @ " << static_cast<const void*>(&body) << '\n';
        body.clear();
        body.push_front(start_pos);
        Point second_segment = start_pos;
        switch (start_dir) {
            case Dir::UP:    second_segment.y++; break;
            case Dir::DOWN:  second_segment.y--; break;
            case Dir::LEFT:  second_segment.x++; break;
            case Dir::RIGHT: second_segment.x--; break;
        }
        body.push_back(second_segment);
        direction = start_dir;
        is_alive = true;
    }
};