#ifndef CONFIG_H
#define CONFIG_H

#include <string>

void load_config(const std::string& filename);
void parse_args(int argc, char* argv[]);

#endif // CONFIG_H
