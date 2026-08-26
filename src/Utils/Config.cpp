#include "Config.h"
#include "Theme.h"
#include "../Utils/Logger.h"
#include <fstream>
#include <filesystem>
#include <shlobj.h>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

Config::Config() { EnsureConfigDir(); }

void Config::EnsureConfigDir() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path))) {
        m_configPath = std::filesystem::path(path) / L"DockForge" / L"config.json";
    } else {
        m_configPath = std::filesystem::path("config.json");
    }
    std::filesystem::create_directories(m_configPath.parent_path());
}

void Config::LoadDefaults() {
    m_config = DockConfig{};
}

bool Config::LoadFromFile() {
    if (!std::filesystem::exists(m_configPath)) {
        LoadDefaults();
        SaveToFile();
        return true;
    }
    try {
        std::ifstream f(m_configPath);
        json j; f >> j;
        auto& c = m_config;
        c.targetFPS = j.value("targetFPS", 60);
        c.vsync = j.value("vsync", true);
        c.adaptiveFPS = j.value("adaptiveFPS", true);
        c.showFPS = j.value("showFPS", false);
        c.backgroundEffect = j.value("backgroundEffect", std::string("acrylic"));
        c.blurRadius = j.value("blurRadius", 16.0f);
        c.tintOpacity = j.value("tintOpacity", 0.6f);
        c.tintR = j.value("tintR", 0.12f); c.tintG = j.value("tintG", 0.12f); c.tintB = j.value("tintB", 0.14f);
        c.refractionStrength = j.value("refractionStrength", 3.0f);
        c.glowIntensity = j.value("glowIntensity", 0.35f);
        c.liquidGlassSaturation = j.value("liquidGlassSaturation", 1.2f);
        c.animationSpeed = j.value("animationSpeed", 1.0f);
        c.magnificationScale = j.value("magnificationScale", 1.3f);
        c.magnificationRange = j.value("magnificationRange", 2.0f);
        c.magnificationEasing = j.value("magnificationEasing", std::string("easeOutCirc"));
        c.jumpAnimation = j.value("jumpAnimation", true);
        c.jumpAnimationSpeed = j.value("jumpAnimationSpeed", 400.0f);
        c.genieEffect = j.value("genieEffect", true);
        c.genieAnimationSpeed = j.value("genieAnimationSpeed", 500.0f);
        c.badgePulse = j.value("badgePulse", true);
        c.badgePulseSpeed = j.value("badgePulseSpeed", 300.0f);
        c.slideAnimationSpeed = j.value("slideAnimationSpeed", 600.0f);
        c.defaultEasing = j.value("defaultEasing", std::string("easeOutBack"));
        c.runningIndicatorPulse = j.value("runningIndicatorPulse", true);
        c.thumbnailPreviews = j.value("thumbnailPreviews", true);
        c.jumpLists = j.value("jumpLists", true);
        c.windowGrouping = j.value("windowGrouping", true);
        c.showTrayIcons = j.value("showTrayIcons", true);
        c.showStartButton = j.value("showStartButton", true);
        c.startButtonSkin = j.value("startButtonSkin", std::string("default"));

        // Chat 09 additions
        c.multiMonitorMode = j.value("multiMonitorMode", std::string("primary"));
        c.performanceProfile = j.value("performanceProfile", std::string("balanced"));

        std::string theme = j.value("theme", std::string("auto"));
        if (theme == "light") ThemeManager::Instance().SetMode(ThemeMode::Light);
        else if (theme == "dark") ThemeManager::Instance().SetMode(ThemeMode::Dark);
        else ThemeManager::Instance().SetMode(ThemeMode::Auto);
        return true;
    } catch (...) {
        LoadDefaults();
        return false;
    }
}

bool Config::SaveToFile() {
    try {
        json j;
        auto& c = m_config;
        j["targetFPS"] = c.targetFPS; j["vsync"] = c.vsync; j["adaptiveFPS"] = c.adaptiveFPS; j["showFPS"] = c.showFPS;
        j["backgroundEffect"] = c.backgroundEffect; j["blurRadius"] = c.blurRadius; j["tintOpacity"] = c.tintOpacity;
        j["tintR"] = c.tintR; j["tintG"] = c.tintG; j["tintB"] = c.tintB;
        j["refractionStrength"] = c.refractionStrength; j["glowIntensity"] = c.glowIntensity; j["liquidGlassSaturation"] = c.liquidGlassSaturation;
        j["animationSpeed"] = c.animationSpeed; j["magnificationScale"] = c.magnificationScale; j["magnificationRange"] = c.magnificationRange;
        j["magnificationEasing"] = c.magnificationEasing; j["jumpAnimation"] = c.jumpAnimation; j["jumpAnimationSpeed"] = c.jumpAnimationSpeed;
        j["genieEffect"] = c.genieEffect; j["genieAnimationSpeed"] = c.genieAnimationSpeed; j["badgePulse"] = c.badgePulse;
        j["badgePulseSpeed"] = c.badgePulseSpeed; j["slideAnimationSpeed"] = c.slideAnimationSpeed; j["defaultEasing"] = c.defaultEasing;
        j["runningIndicatorPulse"] = c.runningIndicatorPulse; j["thumbnailPreviews"] = c.thumbnailPreviews; j["jumpLists"] = c.jumpLists;
        j["windowGrouping"] = c.windowGrouping; j["showTrayIcons"] = c.showTrayIcons; j["showStartButton"] = c.showStartButton;
        j["startButtonSkin"] = c.startButtonSkin;

        // Chat 09 additions
        j["multiMonitorMode"] = c.multiMonitorMode;
        j["performanceProfile"] = c.performanceProfile;

        auto mode = ThemeManager::Instance().GetMode();
        j["theme"] = (mode == ThemeMode::Light) ? "light" : (mode == ThemeMode::Dark) ? "dark" : "auto";
        std::ofstream f(m_configPath); f << j.dump(4);
        return true;
    } catch (...) { return false; }
}

bool Config::ExportTheme(const std::string& path) {
    try {
        json j; j["type"] = "DockForgeTheme"; j["version"] = "1.0";
        auto& c = m_config;
        j["backgroundEffect"] = c.backgroundEffect; j["blurRadius"] = c.blurRadius; j["tintOpacity"] = c.tintOpacity;
        j["tintR"] = c.tintR; j["tintG"] = c.tintG; j["tintB"] = c.tintB;
        j["refractionStrength"] = c.refractionStrength; j["glowIntensity"] = c.glowIntensity; j["liquidGlassSaturation"] = c.liquidGlassSaturation;
        j["animationSpeed"] = c.animationSpeed; j["magnificationScale"] = c.magnificationScale; j["magnificationRange"] = c.magnificationRange;
        j["magnificationEasing"] = c.magnificationEasing; j["jumpAnimationSpeed"] = c.jumpAnimationSpeed;
        j["genieAnimationSpeed"] = c.genieAnimationSpeed; j["badgePulseSpeed"] = c.badgePulseSpeed;
        j["slideAnimationSpeed"] = c.slideAnimationSpeed; j["defaultEasing"] = c.defaultEasing;
        auto mode = ThemeManager::Instance().GetMode();
        j["theme"] = (mode == ThemeMode::Light) ? "light" : (mode == ThemeMode::Dark) ? "dark" : "auto";
        std::ofstream f(path); f << j.dump(4); return true;
    } catch (...) { return false; }
}

bool Config::ImportTheme(const std::string& path) {
    try {
        std::ifstream f(path); json j; f >> j;
        if (j.value("type", std::string()) != "DockForgeTheme") return false;
        auto& c = m_config;
        c.backgroundEffect = j.value("backgroundEffect", c.backgroundEffect);
        c.blurRadius = j.value("blurRadius", c.blurRadius); c.tintOpacity = j.value("tintOpacity", c.tintOpacity);
        c.tintR = j.value("tintR", c.tintR); c.tintG = j.value("tintG", c.tintG); c.tintB = j.value("tintB", c.tintB);
        c.refractionStrength = j.value("refractionStrength", c.refractionStrength);
        c.glowIntensity = j.value("glowIntensity", c.glowIntensity); c.liquidGlassSaturation = j.value("liquidGlassSaturation", c.liquidGlassSaturation);
        c.animationSpeed = j.value("animationSpeed", c.animationSpeed); c.magnificationScale = j.value("magnificationScale", c.magnificationScale);
        c.magnificationRange = j.value("magnificationRange", c.magnificationRange); c.magnificationEasing = j.value("magnificationEasing", c.magnificationEasing);
        c.jumpAnimationSpeed = j.value("jumpAnimationSpeed", c.jumpAnimationSpeed); c.genieAnimationSpeed = j.value("genieAnimationSpeed", c.genieAnimationSpeed);
        c.badgePulseSpeed = j.value("badgePulseSpeed", c.badgePulseSpeed); c.slideAnimationSpeed = j.value("slideAnimationSpeed", c.slideAnimationSpeed);
        c.defaultEasing = j.value("defaultEasing", c.defaultEasing);
        std::string theme = j.value("theme", std::string("auto"));
        if (theme == "light") ThemeManager::Instance().SetMode(ThemeMode::Light);
        else if (theme == "dark") ThemeManager::Instance().SetMode(ThemeMode::Dark);
        else ThemeManager::Instance().SetMode(ThemeMode::Auto);
        SaveToFile(); return true;
    } catch (...) { return false; }
}