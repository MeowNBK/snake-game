#pragma once
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

class NeuralNetwork {
   public:
    enum class Activation { RELU, SIGMOID, LINEAR };

    NeuralNetwork(const std::vector<int> &topology);

    std::vector<double> forward_pass(const std::vector<double> &input);
    void backward_pass(const std::vector<double> &target,
                       bool accumulate_gradients = false);
    void update_parameters(double learning_rate, int batch_size);
    void clear_gradients();

    // Nâng cấp: Lưu/Tải file nhị phân (nhanh hơn rất nhiều)
    void save_weights(const std::string &filename);
    void load_weights(const std::string &filename);

    void load_from_vectors(const std::vector<double> &flat_weights,
                           const std::vector<double> &flat_biases);

    void copy_from(const NeuralNetwork &other);
    double get_a_weight_for_debug() const;

    // THÊM 2 HÀM NÀY VÀO PHẦN public CỦA LỚP NeuralNetwork

    // Hàm Lai ghép: Lấy gen từ "other" và trộn vào "this"
    void crossover_with(const NeuralNetwork &other);

    // Hàm Đột biến: Thay đổi ngẫu nhiên một vài gen
    void mutate(double mutation_rate, double mutation_strength);

   private:
    // --- Khai báo trước các hàm nội bộ ---
    double apply_activation(double x, Activation type);
    double apply_activation_derivative(double z, Activation type);

    // --- Cấu trúc mạng (Topology) ---
    std::vector<int> topology;

    // --- Các tham số có thể học được ---
    std::vector<std::vector<std::vector<double>>> weights;
    std::vector<std::vector<double>> biases;

    // --- Các biến cho Adam Optimizer ---
    std::vector<std::vector<std::vector<double>>> m_dw, v_dw;
    std::vector<std::vector<double>> m_db, v_db;
    int t = 0;
    double beta1 = 0.9, beta2 = 0.999, epsilon_adam = 1e-8;

    // --- Cache cho quá trình tính toán ---
    std::vector<std::vector<double>> activations;
    std::vector<std::vector<double>> z_values;
    std::vector<std::vector<double>> deltas;
    std::vector<std::vector<std::vector<double>>> gradients_w;
    std::vector<std::vector<double>> gradients_b;
};

// --- Triển khai các hàm ---

inline NeuralNetwork::NeuralNetwork(const std::vector<int> &topology)
    : topology(topology) {
    std::mt19937 gen(std::random_device{}());
    for (size_t i = 0; i < topology.size() - 1; ++i) {
        int fan_in = topology[i];
        int fan_out = topology[i + 1];
        std::uniform_real_distribution<double> dist_xavier(
            -sqrt(6.0 / (fan_in + fan_out)), sqrt(6.0 / (fan_in + fan_out)));
        weights.push_back(std::vector<std::vector<double>>(
            fan_out, std::vector<double>(fan_in)));
        for (auto &row : weights.back())
            for (auto &val : row) val = dist_xavier(gen);
        biases.push_back(std::vector<double>(fan_out, 0.1));
        m_dw.push_back(std::vector<std::vector<double>>(
            fan_out, std::vector<double>(fan_in, 0.0)));
        v_dw.push_back(std::vector<std::vector<double>>(
            fan_out, std::vector<double>(fan_in, 0.0)));
        m_db.push_back(std::vector<double>(fan_out, 0.0));
        v_db.push_back(std::vector<double>(fan_out, 0.0));
        gradients_w.push_back(std::vector<std::vector<double>>(
            fan_out, std::vector<double>(fan_in, 0.0)));
        gradients_b.push_back(std::vector<double>(fan_out, 0.0));
    }
}

inline std::vector<double> NeuralNetwork::forward_pass(
    const std::vector<double> &input) {
    if (input.size() != (size_t)topology[0])
        throw std::runtime_error("Lỗi: Kích thước input không khớp!");
    activations.assign(topology.size(), std::vector<double>());
    z_values.assign(topology.size(), std::vector<double>());
    activations[0] = input;
    for (size_t i = 0; i < weights.size(); ++i) {
        size_t n_neurons = topology[i + 1];
        z_values[i + 1].resize(n_neurons);
        activations[i + 1].resize(n_neurons);
        for (size_t j = 0; j < n_neurons; ++j) {
            double z = biases[i][j];
            for (size_t k = 0; k < topology[i]; ++k)
                z += weights[i][j][k] * activations[i][k];
            z_values[i + 1][j] = z;
            Activation act_type = (i == weights.size() - 1) ? Activation::LINEAR
                                                            : Activation::RELU;
            activations[i + 1][j] = apply_activation(z, act_type);
        }
    }
    return activations.back();
}

inline void NeuralNetwork::clear_gradients() {
    for (auto &layer : gradients_w)
        for (auto &neuron : layer) std::fill(neuron.begin(), neuron.end(), 0.0);
    for (auto &layer : gradients_b) std::fill(layer.begin(), layer.end(), 0.0);
}

inline void NeuralNetwork::backward_pass(const std::vector<double> &target,
                                         bool accumulate_gradients) {
    if (!accumulate_gradients) clear_gradients();

    size_t output_layer_idx = topology.size() - 1;
    if (target.size() != topology[output_layer_idx])
        throw std::runtime_error("Lỗi: Kích thước target không khớp!");

    deltas.assign(topology.size(), std::vector<double>());
    deltas[output_layer_idx].resize(topology[output_layer_idx]);

    // Giai đoạn 1: Tính Delta cho lớp Output
    for (size_t i = 0; i < topology[output_layer_idx]; ++i) {
        double error = activations[output_layer_idx][i] - target[i];
        deltas[output_layer_idx][i] =
            error * apply_activation_derivative(z_values[output_layer_idx][i],
                                                Activation::LINEAR);
    }

    // Giai đoạn 2: Lan truyền ngược Delta cho các lớp ẩn (từ L-1 về lớp 1)
    for (size_t i = output_layer_idx - 1; i > 0; --i) {
        deltas[i].resize(topology[i]);
        for (size_t j = 0; j < topology[i]; ++j) {
            double error_propagated = 0.0;
            for (size_t k = 0; k < topology[i + 1]; ++k) {
                error_propagated += weights[i][k][j] * deltas[i + 1][k];
            }
            deltas[i][j] =
                error_propagated *
                apply_activation_derivative(z_values[i][j], Activation::RELU);
        }
    }

    // Giai đoạn 3: Dùng các Delta đã tính để tính Gradient cho tất cả các
    // weight và bias
    for (size_t i = 0; i < weights.size(); ++i) {
        for (size_t j = 0; j < topology[i + 1]; ++j) {
            for (size_t k = 0; k < topology[i]; ++k) {
                gradients_w[i][j][k] += deltas[i + 1][j] * activations[i][k];
            }
            gradients_b[i][j] += deltas[i + 1][j];
        }
    }
}

inline void NeuralNetwork::update_parameters(double learning_rate,
                                             int batch_size) {
    if (batch_size == 0) return;
    t++;
    for (size_t i = 0; i < weights.size(); ++i) {
        for (size_t j = 0; j < weights[i].size(); ++j) {
            double grad_b = gradients_b[i][j] / batch_size;
            m_db[i][j] = beta1 * m_db[i][j] + (1 - beta1) * grad_b;
            v_db[i][j] = beta2 * v_db[i][j] + (1 - beta2) * pow(grad_b, 2);
            double m_hat_b = m_db[i][j] / (1 - pow(beta1, t));
            double v_hat_b = v_db[i][j] / (1 - pow(beta2, t));
            biases[i][j] -=
                learning_rate * m_hat_b / (sqrt(v_hat_b) + epsilon_adam);
            for (size_t k = 0; k < weights[i][j].size(); ++k) {
                double grad_w = gradients_w[i][j][k] / batch_size;
                m_dw[i][j][k] = beta1 * m_dw[i][j][k] + (1 - beta1) * grad_w;
                v_dw[i][j][k] =
                    beta2 * v_dw[i][j][k] + (1 - beta2) * pow(grad_w, 2);
                double m_hat_w = m_dw[i][j][k] / (1 - pow(beta1, t));
                double v_hat_w = v_dw[i][j][k] / (1 - pow(beta2, t));
                weights[i][j][k] -=
                    learning_rate * m_hat_w / (sqrt(v_hat_w) + epsilon_adam);
            }
        }
    }
}

inline void NeuralNetwork::save_weights(const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) return;
    for (const auto &layer : weights)
        for (const auto &neuron : layer)
            file.write(reinterpret_cast<const char *>(neuron.data()),
                       neuron.size() * sizeof(double));
    for (const auto &layer : biases)
        file.write(reinterpret_cast<const char *>(layer.data()),
                   layer.size() * sizeof(double));
}

inline void NeuralNetwork::load_weights(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Không tìm thấy file brain, dùng weights ngẫu nhiên.\n";
        return;
    }
    for (auto &layer : weights)
        for (auto &neuron : layer)
            file.read(reinterpret_cast<char *>(neuron.data()),
                      neuron.size() * sizeof(double));
    for (auto &layer : biases)
        file.read(reinterpret_cast<char *>(layer.data()),
                  layer.size() * sizeof(double));
    std::cout << "Đã tải weights từ file: " << filename << std::endl;
}

inline void NeuralNetwork::copy_from(const NeuralNetwork &other) {
    this->weights = other.weights;
    this->biases = other.biases;
}
inline double NeuralNetwork::get_a_weight_for_debug() const {
    if (!weights.empty() && !weights[0].empty() && !weights[0][0].empty()) {
        return weights[0][0][0];
    }
    return 0.0;
}
inline double NeuralNetwork::apply_activation(double x, Activation type) {
    switch (type) {
        case Activation::RELU:
            return std::max(0.0, x);
        case Activation::SIGMOID:
            return 1.0 / (1.0 + exp(-x));
        case Activation::LINEAR:
        default:
            return x;
    }
}
inline double NeuralNetwork::apply_activation_derivative(double z,
                                                         Activation type) {
    switch (type) {
        case Activation::RELU:
            return (z > 0) ? 1.0 : 0.0;
        case Activation::SIGMOID: {
            double a = apply_activation(z, Activation::SIGMOID);
            return a * (1.0 - a);
        }
        case Activation::LINEAR:
        default:
            return 1.0;
    }
}

inline void NeuralNetwork::load_from_vectors(
    const std::vector<double> &flat_weights,
    const std::vector<double> &flat_biases) {
    size_t weight_idx = 0;
    for (auto &layer : weights) {
        for (auto &neuron : layer) {
            for (auto &w : neuron) {
                if (weight_idx < flat_weights.size()) {
                    w = flat_weights[weight_idx++];
                }
            }
        }
    }
    size_t bias_idx = 0;
    for (auto &layer : biases) {
        for (auto &b : layer) {
            if (bias_idx < flat_biases.size()) {
                b = flat_biases[bias_idx++];
            }
        }
    }
}

// THÊM 2 HÀM NÀY VÀO PHẦN public CỦA LỚP NeuralNetwork

// Hàm Lai ghép: Lấy gen từ "other" và trộn vào "this"
inline void NeuralNetwork::crossover_with(const NeuralNetwork &other) {
    std::mt19937 gen(std::random_device{}());
    // Lai ghép weights
    for (size_t i = 0; i < weights.size(); ++i) {
        for (size_t j = 0; j < weights[i].size(); ++j) {
            for (size_t k = 0; k < weights[i][j].size(); ++k) {
                // Với mỗi gen, có 50% cơ hội nhận từ "other"
                if (std::uniform_real_distribution<>(0.0, 1.0)(gen) < 0.5) {
                    this->weights[i][j][k] = other.weights[i][j][k];
                }
            }
        }
    }
    // Lai ghép biases
    for (size_t i = 0; i < biases.size(); ++i) {
        for (size_t j = 0; j < biases[i].size(); ++j) {
            if (std::uniform_real_distribution<>(0.0, 1.0)(gen) < 0.5) {
                this->biases[i][j] = other.biases[i][j];
            }
        }
    }
}

// Hàm Đột biến: Thay đổi ngẫu nhiên một vài gen
inline void NeuralNetwork::mutate(double mutation_rate,
                                  double mutation_strength) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> dist(-mutation_strength,
                                                mutation_strength);

    for (auto &layer : weights) {
        for (auto &neuron : layer) {
            for (auto &w : neuron) {
                // Với mỗi weight, có một xác suất nhỏ là nó sẽ bị đột biến
                if (std::uniform_real_distribution<>(0.0, 1.0)(gen) <
                    mutation_rate) {
                    w += dist(gen);
                }
            }
        }
    }
    // Tương tự cho biases
    for (auto &layer : biases) {
        for (auto &b : layer) {
            if (std::uniform_real_distribution<>(0.0, 1.0)(gen) <
                mutation_rate) {
                b += dist(gen);
            }
        }
    }
}