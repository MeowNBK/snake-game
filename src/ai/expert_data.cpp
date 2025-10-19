#include "ai/expert_data.h"
#include "core/game_state.h" // Để truy cập 'expert_demonstrations'
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <vector>

void load_pretrained_brain(AIPlayer& ai) {
    std::cout << "Tai bo nao huan luyen san cua Meo than thien...\n";
    const int num_weights = (16*24) + (24*24) + (24*4);
    const int num_biases = 24 + 24 + 4;
    
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-0.5, 0.5);

    std::vector<double> pretrained_weights;
    for(int i = 0; i < num_weights; ++i) pretrained_weights.push_back(dist(gen));
    
    std::vector<double> pretrained_biases;
    for(int i = 0; i < num_biases; ++i) pretrained_biases.push_back(dist(gen));

    ai.brain.load_from_vectors(pretrained_weights, pretrained_biases);
    ai.target_brain.copy_from(ai.brain);
    std::cout << "Da nap xong bo nao 'Vip Pro'!\n";
}

void save_expert_demonstrations(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Lỗi: Không thể mở file để lưu dữ liệu chuyên gia: " << filename << std::endl;
        return;
    }

    for (const auto& exp : expert_demonstrations) {
        file << exp.action_idx << " " << exp.reward << " " << exp.done << " ";
        for (const auto& val : exp.state) {
            file << val << " ";
        }
        file << "\n";
    }
    file.close();
    std::cout << "Đã lưu " << expert_demonstrations.size() << " kinh nghiệm chuyên gia vào " << filename << std::endl;
}

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
