#pragma once
#include <d2d1.h>
#include <string>

enum class ThemeMode { Light, Dark, Auto };

struct ThemeColors {
    D2D1_COLOR_F background;
    D2D1_COLOR_F backgroundSecondary;
    D2D1_COLOR_F accent;
    D2D1_COLOR_F accentHover;
    D2D1_COLOR_F border;
    D2D1_COLOR_F text;
    D2D1_COLOR_F textSecondary;
    D2D1_COLOR_F controlBg;
    D2D1_COLOR_F controlBgHover;
    D2D1_COLOR_F shadow;
};

class Theme {
public:
    static Theme& Instance();
    void SetMode(ThemeMode mode);
    ThemeMode GetMode() const { return m_mode; }
    const ThemeColors& GetColors() const { return m_colors; }
    void Refresh();
    bool IsSystemDarkMode() const;
    void Apply(const std::string& themeStr);
private:
    Theme();
    void BuildColors();
    ThemeMode m_mode = ThemeMode::Auto;
    ThemeColors m_colors;
};

// Alias for compatibility
using ThemeManager = Theme;