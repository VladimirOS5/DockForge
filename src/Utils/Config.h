#pragma once
#include <string>

struct DockConfig {
    // Performance
    int targetFPS = 60;
    bool vsync = true;
    bool adaptiveFPS = true;
    int idleFPS = 10;
    int fullscreenFPS = 1;
    bool showFPS = false;

    // Effects
    std::string backgroundEffect = "acrylic"; // "solid", "acrylic", "liquidglass"
    float blurRadius = 16.0f;
    float tintOpacity = 0.6f;
    float tintR = 0.12f, tintG = 0.12f, tintB = 0.14f;
    float refractionStrength = 3.0f;
    float glowIntensity = 0.35f;
    float liquidGlassSaturation = 1.2f;

    // Animations
    float animationSpeed = 1.0f; // global multiplier
    float magnificationScale = 1.3f;
    float magnificationRange = 2.0f; // how many neighbor icons also scale
    std::string magnificationEasing = "easeOutCirc";
    bool jumpAnimation = true;
    float jumpAnimationSpeed = 400.0f; // ms
    bool genieEffect = true;
    float genieAnimationSpeed = 500.0f; // ms
    bool badgePulse = true;
    float badgePulseSpeed = 300.0f; // ms
    float slideAnimationSpeed = 600.0f; // ms
    std::string defaultEasing = "easeOutBack";
    bool runningIndicatorPulse = true;
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
