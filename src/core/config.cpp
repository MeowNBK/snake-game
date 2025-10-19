#include "core/config.h"
#include "core/game_state.h" // Để truy cập biến 'config'
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>

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
