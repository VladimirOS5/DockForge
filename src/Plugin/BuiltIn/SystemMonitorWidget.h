#pragma once
#include "../PluginManager.h"

class SystemMonitorWidget : public WidgetBase {
public:
    SystemMonitorWidget();
    void Update(float deltaTime) override;
    void Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float w, float h) override;
    float GetWidth() const override { return 60.0f; }
    const char* GetName() const override { return "SystemMonitor"; }
private:
    float m_cpuUsage = 0.0f;
    ULARGE_INTEGER m_lastIdle = {}, m_lastKernel = {}, m_lastUser = {};
    float m_updateTimer = 0.0f;
};