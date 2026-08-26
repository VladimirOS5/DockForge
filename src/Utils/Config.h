#pragma once
#include <string>
#include <filesystem>

struct DockConfig {
    int targetFPS = 60;
    bool vsync = true;
    bool adaptiveFPS = true;
    int idleFPS = 10;
    int fullscreenFPS = 1;
    bool showFPS = false;
    std::string backgroundEffect = "acrylic";
    float blurRadius = 16.0f;
    float tintOpacity = 0.6f;
    float tintR = 0.12f, tintG = 0.12f, tintB = 0.14f;
    float refractionStrength = 3.0f;
    float glowIntensity = 0.35f;
    float liquidGlassSaturation = 1.2f;
    float animationSpeed = 1.0f;
    float magnificationScale = 1.3f;
    float magnificationRange = 2.0f;
    std::string magnificationEasing = "easeOutCirc";
    bool jumpAnimation = true;
    float jumpAnimationSpeed = 400.0f;
    bool genieEffect = true;
    float genieAnimationSpeed = 500.0f;
    bool badgePulse = true;
    float badgePulseSpeed = 300.0f;
    float slideAnimationSpeed = 600.0f;
    std::string defaultEasing = "easeOutBack";
    bool runningIndicatorPulse = true;
    bool thumbnailPreviews = true;
    bool jumpLists = true;
    bool windowGrouping = true;
    bool showTrayIcons = true;
    bool showStartButton = true;
    std::string startButtonSkin = "default";

    // Chat 09 additions
    std::string multiMonitorMode = "primary";      // "primary", "all"
    std::string performanceProfile = "balanced";   // "eco", "balanced", "performance", "custom"
};

class Config {
public:
    static Config& Instance();
    void LoadDefaults();
    bool LoadFromFile();
    bool SaveToFile();
    bool ExportTheme(const std::string& path);
    bool ImportTheme(const std::string& path);
    const DockConfig& Get() const { return m_config; }
    DockConfig& GetMutable() { return m_config; }
private:
    Config();
    void EnsureConfigDir();
    DockConfig m_config;
    std::filesystem::path m_configPath;
};