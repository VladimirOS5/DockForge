#include "SystemMonitorWidget.h"
#include <wrl/client.h>
#include <algorithm>

SystemMonitorWidget::SystemMonitorWidget() {
    FILETIME idle, kernel, user;
    GetSystemTimes(&idle, &kernel, &user);
    m_lastIdle.LowPart = idle.dwLowDateTime; m_lastIdle.HighPart = idle.dwHighDateTime;
    m_lastKernel.LowPart = kernel.dwLowDateTime; m_lastKernel.HighPart = kernel.dwHighDateTime;
    m_lastUser.LowPart = user.dwLowDateTime; m_lastUser.HighPart = user.dwHighDateTime;
}

void SystemMonitorWidget::Update(float deltaTime) {
    m_updateTimer += deltaTime;
    if (m_updateTimer > 1.0f) {
        m_updateTimer = 0.0f;
        FILETIME idle, kernel, user;
        if (GetSystemTimes(&idle, &kernel, &user)) {
            ULARGE_INTEGER currIdle, currKernel, currUser;
            currIdle.LowPart = idle.dwLowDateTime; currIdle.HighPart = idle.dwHighDateTime;
            currKernel.LowPart = kernel.dwLowDateTime; currKernel.HighPart = kernel.dwHighDateTime;
            currUser.LowPart = user.dwLowDateTime; currUser.HighPart = user.dwHighDateTime;
            ULONGLONG diffIdle = currIdle.QuadPart - m_lastIdle.QuadPart;
            ULONGLONG diffKernel = currKernel.QuadPart - m_lastKernel.QuadPart;
            ULONGLONG diffUser = currUser.QuadPart - m_lastUser.QuadPart;
            ULONGLONG diffTotal = diffKernel + diffUser;
            if (diffTotal > 0) {
                m_cpuUsage = static_cast<float>(diffTotal - diffIdle) / static_cast<float>(diffTotal);
                m_cpuUsage = std::max(0.0f, std::min(1.0f, m_cpuUsage));
            }
            m_lastIdle = currIdle; m_lastKernel = currKernel; m_lastUser = currUser;
        }
    }
}

void SystemMonitorWidget::Render(ID2D1RenderTarget* rt, IDWriteFactory*, float x, float y, float w, float h) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> bgBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.15f, 0.15f, 0.17f, 0.8f), &bgBrush);
    rt->FillRectangle(D2D1::RectF(x, y, x + w, y + h), bgBrush.Get());

    float barH = h * 0.6f;
    float barY = y + (h - barH) / 2.0f;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> barBrush;
    float r = m_cpuUsage > 0.8f ? 1.0f : (m_cpuUsage > 0.5f ? 1.0f : 0.0f);
    float g = m_cpuUsage > 0.8f ? 0.2f : (m_cpuUsage > 0.5f ? 0.8f : 0.8f);
    rt->CreateSolidColorBrush(D2D1::ColorF(r, g, 0.3f, 1.0f), &barBrush);
    rt->FillRectangle(D2D1::RectF(x + 4, barY + barH * (1.0f - m_cpuUsage), x + w - 4, barY + barH), barBrush.Get());

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
    rt->CreateSolidColorBrush(D2D1::ColorF(0.4f, 0.4f, 0.45f, 0.5f), &borderBrush);
    rt->DrawRectangle(D2D1::RectF(x, y, x + w, y + h), borderBrush.Get(), 1.0f);
}