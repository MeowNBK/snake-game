#pragma once
#include "common.h"
#include "neural_network.h"
#include <deque>
#include <vector>
#include <algorithm> // for std::max_element

// Lớp này quản lý một con rắn AI
class AIPlayer {
public:
    std::deque<Point> body;
    Dir direction;
    bool is_alive;
    
    // Mỗi AI sẽ có một bộ não riêng
    NeuralNetwork brain;
    NeuralNetwork target_brain;
    double epsilon = 0.9;

    // Cấu trúc để ghi nhớ kinh nghiệm
    struct Experience {
        std::vector<double> state;
        int action_idx; // 0:UP, 1:DOWN, 2:LEFT, 3:RIGHT
        double reward;
        std::vector<double> next_state;
        bool done;
    };
    std::deque<Experience> memory; // Bộ nhớ kinh nghiệm
    size_t memory_size = 10000;
    int train_step_counter = 0;

    AIPlayer() : brain({16, 24, 24, 4}), target_brain({16, 24, 24, 4}) { // Sửa thành 16 input
        reset({10, 10}, Dir::LEFT);
        target_brain.copy_from(brain);
    }
    
    void reset(Point start_pos, Dir start_dir) {
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

    // "Mắt" của AI: Chuyển trạng thái game thành vector số
    std::vector<double> get_state_vector(Point apple, const std::deque<Point>& opponent_body) {
        Point head = body.front();

        // Các hướng cần kiểm tra: N, NE, E, SE, S, SW, W, NW
        const std::vector<Point> directions = {
            {0, -1}, {1, -1}, {1, 0}, {1, 1}, 
            {0, 1}, {-1, 1}, {-1, 0}, {-1, -1}
        };
        
        // Vector state mới (8 khoảng cách nguy hiểm + 4 hướng táo + 4 hướng đi = 16 inputs)
        std::vector<double> state(16, 0.0);

        // Inputs 0-7: "Tầm nhìn" 8 hướng về khoảng cách tới nguy hiểm
        for (size_t i = 0; i < directions.size(); ++i) {
            Point p = head;
            double distance = 0.0;
            while (true) {
                p.x += directions[i].x;
                p.y += directions[i].y;
                distance += 1.0;

                // Kiểm tra va chạm tường, thân mình, hoặc đối thủ
                bool danger_found = (
                    p.x < 0 || p.x >= GRID_W || p.y < 0 || p.y >= GRID_H ||
                    std::any_of(body.begin(), body.end(), [&](Point seg){ return p == seg; }) ||
                    std::any_of(opponent_body.begin(), opponent_body.end(), [&](Point seg){ return p == seg; })
                );

                if (danger_found) {
                    state[i] = 1.0 / distance; // Dùng nghịch đảo khoảng cách
                    break;
                }
                
                // Thêm một giới hạn để việc "nhìn" không bị vô tận
                if(distance > (GRID_W + GRID_H)) break;
            }
        }
        
        // Inputs 8-11: Hướng của quả táo (dạng boolean)
        state[8] = (apple.y < head.y); // Táo ở trên
        state[9] = (apple.y > head.y); // Táo ở dưới
        state[10] = (apple.x < head.x); // Táo ở bên trái
        state[11] = (apple.x > head.x); // Táo ở bên phải

        // Inputs 12-15: Hướng di chuyển hiện tại
        state[12] = (direction == Dir::UP);
        state[13] = (direction == Dir::DOWN);
        state[14] = (direction == Dir::LEFT);
        state[15] = (direction == Dir::RIGHT);

        return state;
    }

    // Ra quyết định dựa trên state
    // Dir choose_move(const std::vector<double>& state) {
    //     std::vector<double> output = brain.forward_pass(state);
    //     // Tìm neuron có giá trị output cao nhất
    //     int action_idx = std::distance(output.begin(), std::max_element(output.begin(), output.end()));
    //     return static_cast<Dir>(action_idx);
    // }

    // THAY THẾ TOÀN BỘ HÀM choose_move CŨ BẰNG HÀM "CHỐNG TỰ SÁT" NÀY
    // THAY THẾ TOÀN BỘ HÀM choose_move CŨ BẰNG HÀM NÀY
    Dir choose_move(const std::vector<double>& state_vec) {
        static std::mt19937 gen(std::random_device{}());
        std::uniform_real_distribution<> dis(0.0, 1.0);

        // Xác định trước hướng đi ngược lại không hợp lệ
        Dir opposite_dir = Dir::UP; // Giá trị mặc định
        if (direction == Dir::UP) opposite_dir = Dir::DOWN;
        if (direction == Dir::DOWN) opposite_dir = Dir::UP;
        if (direction == Dir::LEFT) opposite_dir = Dir::RIGHT;
        if (direction == Dir::RIGHT) opposite_dir = Dir::LEFT;
        int opposite_dir_idx = static_cast<int>(opposite_dir);

        if (dis(gen) < epsilon) {
            // --- THĂM DÒ AN TOÀN ---
            std::vector<Dir> possible_moves;
            for (int i = 0; i < 4; ++i) {
                if (i == opposite_dir_idx) continue; // Bỏ qua hướng đi ngược lại
                possible_moves.push_back(static_cast<Dir>(i));
            }
            // Chọn ngẫu nhiên trong số các hướng hợp lệ
            return possible_moves[std::uniform_int_distribution<int>(0, possible_moves.size() - 1)(gen)];
        } else {
            // --- KHAI THÁC THÔNG MINH ---
            std::vector<double> output = brain.forward_pass(state_vec);

            // "Cấm" bộ não chọn hướng đi ngược lại bằng cách gán cho nó một giá trị cực thấp
            output[opposite_dir_idx] = -1.0e9; // Một số âm rất lớn

            // Bây giờ, tìm hướng tốt nhất trong số các lựa chọn đã được "lọc"
            int action_idx = std::distance(output.begin(), std::max_element(output.begin(), output.end()));
            return static_cast<Dir>(action_idx);
        }
    }

    
    // Ghi nhớ kinh nghiệm
    void remember(const Experience& exp) {
        if (memory.size() > memory_size) {
            memory.pop_front();
        }
        memory.push_back(exp);
    }

    // "Rút kinh nghiệm" từ những gì đã nhớ
    void train_from_memory(size_t batch_size, double learning_rate) {
        if (memory.size() < (size_t)batch_size) return;
        std::vector<Experience> batch;
        std::sample(memory.begin(), memory.end(), std::back_inserter(batch), batch_size, std::mt19937{std::random_device{}()});
        
        brain.clear_gradients(); // Xóa gradient cũ trước khi bắt đầu batch mới
        for (const auto& exp : batch) {
            std::vector<double> current_q_values = brain.forward_pass(exp.state);
            std::vector<double> target_q_values = current_q_values;
            double new_q;
            if (exp.done) {
                new_q = exp.reward;
            } else {
                std::vector<double> next_q_values = target_brain.forward_pass(exp.next_state);
                double max_next_q = *std::max_element(next_q_values.begin(), next_q_values.end());
                new_q = exp.reward + 0.9 * max_next_q; // gamma = 0.9
            }
            target_q_values[exp.action_idx] = new_q;
            // Chỉ tích lũy gradient, không update ngay
            brain.backward_pass(target_q_values, true); // true = accumulate
        }
        // Cập nhật weight một lần cho cả batch
        brain.update_parameters(learning_rate, batch_size);
        train_step_counter += batch_size;
        if (train_step_counter > 1000) {
            target_brain.copy_from(brain);
            train_step_counter = 0;
        }
    }


    // THÊM HÀM MỚI NÀY VÀO LỚP AIPlayer TRONG ai_player.h
    void train_from_demonstrations(const std::vector<Experience>& demonstrations, int epochs, double learning_rate, int batch_size) {
        if (demonstrations.empty()) return;
        std::cout << "--- BẮT ĐẦU HUẤN LUYỆN THEO CHUYÊN GIA ---\n";
        std::vector<Experience> data = demonstrations;
        std::mt19937 rng(std::random_device{}());

        for (int epoch = 0; epoch < epochs; ++epoch) {
            std::shuffle(data.begin(), data.end(), rng);
            double total_loss_epoch = 0.0;
            for (size_t start = 0; start < data.size(); start += batch_size) {
                size_t end = std::min(start + batch_size, data.size());
                brain.clear_gradients();
                for (size_t i = start; i < end; ++i) {
                    const auto& exp = data[i];
                    std::vector<double> out = brain.forward_pass(exp.state);
                    std::vector<double> target(4, 0.0);
                    target[exp.action_idx] = 1.0;
                    total_loss_epoch += -std::log(out[exp.action_idx] + 1e-9);
                    brain.backward_pass(target, true);
                }
                brain.update_parameters(learning_rate, end - start);
            }
            if ((epoch + 1) % 10 == 0) std::cout << "Epoch " << (epoch + 1) << "/" << epochs << ", Loss: " << total_loss_epoch / data.size() << "\n";
        }
        target_brain.copy_from(brain);
        std::cout << "--- HUẤN LUYỆN THEO CHUYÊN GIA HOÀN TẤT ---\n";
    }
};