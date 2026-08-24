#pragma once
#include <string>

struct DockConfig {
    int targetFPS = 60;
    bool vsync = true;
    bool adaptiveFPS = true;
    int idleFPS = 10;
    int fullscreenFPS = 1;
    std::string backgroundEffect = "acrylic"; // "solid", "acrylic", "liquidglass"
    float blurRadius = 16.0f;
    float tintOpacity = 0.6f;
    float tintR = 0.12f, tintG = 0.12f, tintB = 0.14f;
    float refractionStrength = 3.0f;
    float glowIntensity = 0.35f;
    float liquidGlassSaturation = 1.2f;
    bool showFPS = false;
};

class Config {
public:
    static Config& Instance();
    void LoadDefaults();
    const DockConfig& Get() const { return m_config; }
    DockConfig& GetMutable() { return m_config; }
private:
    DockConfig m_config;
    Config() = default;
};
