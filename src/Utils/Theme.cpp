#include "Theme.h"
#include <windows.h>

Theme& Theme::Instance() {
    static Theme instance;
    return instance;
}

Theme::Theme() { BuildColors(); }

bool Theme::IsSystemDarkMode() const {
    HKEY hKey;
    DWORD value = 1;
    DWORD size = sizeof(DWORD);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, nullptr, (LPBYTE)&value, &size);
        RegCloseKey(hKey);
    }
    return value == 0;
}

void Theme::SetMode(ThemeMode mode) {
    m_mode = mode;
    BuildColors();
}

void Theme::Refresh() {
    if (m_mode == ThemeMode::Auto) BuildColors();
}

void Theme::BuildColors() {
    bool dark = (m_mode == ThemeMode::Dark) || (m_mode == ThemeMode::Auto && IsSystemDarkMode());
    if (dark) {
        m_colors.background = { 0.08f, 0.08f, 0.09f, 1.0f };
        m_colors.backgroundSecondary = { 0.12f, 0.12f, 0.14f, 1.0f };
        m_colors.accent = { 0.0f, 0.48f, 0.96f, 1.0f };
        m_colors.accentHover = { 0.2f, 0.6f, 1.0f, 1.0f };
        m_colors.border = { 0.2f, 0.2f, 0.22f, 1.0f };
        m_colors.text = { 1.0f, 1.0f, 1.0f, 1.0f };
        m_colors.textSecondary = { 0.6f, 0.6f, 0.65f, 1.0f };
        m_colors.controlBg = { 0.18f, 0.18f, 0.2f, 1.0f };
        m_colors.controlBgHover = { 0.25f, 0.25f, 0.28f, 1.0f };
        m_colors.shadow = { 0.0f, 0.0f, 0.0f, 0.5f };
    } else {
        m_colors.background = { 0.98f, 0.98f, 0.98f, 1.0f };
        m_colors.backgroundSecondary = { 0.95f, 0.95f, 0.96f, 1.0f };
        m_colors.accent = { 0.0f, 0.4f, 0.9f, 1.0f };
        m_colors.accentHover = { 0.1f, 0.5f, 1.0f, 1.0f };
        m_colors.border = { 0.8f, 0.8f, 0.82f, 1.0f };
        m_colors.text = { 0.1f, 0.1f, 0.1f, 1.0f };
        m_colors.textSecondary = { 0.4f, 0.4f, 0.45f, 1.0f };
        m_colors.controlBg = { 0.9f, 0.9f, 0.92f, 1.0f };
        m_colors.controlBgHover = { 0.85f, 0.85f, 0.88f, 1.0f };
        m_colors.shadow = { 0.0f, 0.0f, 0.0f, 0.15f };
    }
}

void Theme::Apply(const std::string& themeStr) {
    if (themeStr == "dark") SetMode(ThemeMode::Dark);
    else if (themeStr == "light") SetMode(ThemeMode::Light);
    else SetMode(ThemeMode::Auto);
}