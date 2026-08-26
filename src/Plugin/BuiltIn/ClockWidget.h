#pragma once
#include "../PluginManager.h"
#include <string>

class ClockWidget : public WidgetBase {
public:
    ClockWidget();
    void Update(float deltaTime) override;
    void Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float w, float h) override;
    float GetWidth() const override { return 70.0f; }
    const char* GetName() const override { return "Clock"; }
private:
    std::wstring m_timeText;
    float m_updateTimer = 0.0f;
};