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

    // Chat 09
    std::string multiMonitorMode = "primary";
    std::string performanceProfile = "balanced";

    // Chat 10
    bool audioReactiveBackground = false;
    std::string audioReactiveMode = "both";
    int particleCount = 300;
    bool wallpaperEngineIntegration = true;
    float audioSmoothing = 0.85f;
    float audioSensitivity = 1.5f;
    bool particleGlow = true;
    bool particleTrails = false;
    std::string gradientDirection = "horizontal";

    // Chat 11 — OTA & Fallback
    bool autoCheckUpdates = true;
    int updateCheckInterval = 60;
    std::string updateChannel = "stable";
    bool autoDownloadUpdates = true;
    bool autoInstallUpdates = false;
    std::string updateServerUrl = "https://api.dockforge.app/v1/releases";
    bool safeMode = false;
    bool runSelfTestsOnStart = true;

    // Chat 12 — Polish & Testing
    bool enableMemoryTracking = true;
    bool runStabilityTestsOnStart = false;
    bool logDPIInfo = true;
    bool logHDRInfo = true;
    bool detectUWPApps = true;
    bool handleDPIScale = true;
    bool useHDRAwareColors = false;
    int stabilityTestDurationHours = 72;
    float stabilityTimeScale = 60.0f;

    // Theme mode (for Theme compatibility)
    std::string theme = "auto"; // "light", "dark", "auto"
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
