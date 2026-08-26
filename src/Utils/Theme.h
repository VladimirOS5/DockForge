#pragma once
#include <d2d1.h>

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

class ThemeManager {
public:
    static ThemeManager& Instance();
    void SetMode(ThemeMode mode);
    ThemeMode GetMode() const { return m_mode; }
    const ThemeColors& GetColors() const { return m_colors; }
    void Refresh();
    bool IsSystemDarkMode() const;
private:
    ThemeManager();
    void BuildColors();
    ThemeMode m_mode = ThemeMode::Auto;
    ThemeColors m_colors;
};