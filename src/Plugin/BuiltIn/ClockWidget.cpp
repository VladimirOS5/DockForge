#include "ClockWidget.h"
#include <wrl/client.h>
#include <chrono>
#include <iomanip>
#include <sstream>

ClockWidget::ClockWidget() {}

void ClockWidget::Update(float deltaTime) {
    m_updateTimer += deltaTime;
    if (m_updateTimer > 1.0f) {
        m_updateTimer = 0.0f;
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);
        std::wstringstream ss;
        ss << std::put_time(&tm, L"%H:%M");
        m_timeText = ss.str();
    }
}

void ClockWidget::Render(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float w, float h) {
    if (!wf || m_timeText.empty()) return;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
    wf->CreateTextFormat(L"Segoe UI", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL, 16.0f, L"", &format);
    if (!format) return;
    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.9f), &brush);
    rt->DrawTextW(m_timeText.c_str(), static_cast<UINT32>(m_timeText.length()),
        format.Get(), D2D1::RectF(x, y, x + w, y + h), brush.Get());
}