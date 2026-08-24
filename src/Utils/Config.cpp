#include "Config.h"

Config& Config::Instance() {
    static Config instance;
    return instance;
}

void Config::LoadDefaults() {
    m_config = DockConfig{};
}
