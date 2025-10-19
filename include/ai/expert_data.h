#ifndef EXPERT_DATA_H
#define EXPERT_DATA_H

#include "ai/ai_player.h"
#include <string>

void load_pretrained_brain(AIPlayer& ai);
void save_expert_demonstrations(const std::string& filename);
void load_expert_demonstrations(const std::string& filename);

#endif // EXPERT_DATA_H
