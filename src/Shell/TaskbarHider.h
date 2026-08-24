#pragma once
#include <windows.h>

class TaskbarHider {
public:
    TaskbarHider();
    ~TaskbarHider();
    bool Hide();
    bool Restore();
    bool IsHidden() const { return m_hidden; }
private:
    bool m_hidden = false;
    HWND m_hTaskbar = nullptr;
    HWND m_hStart = nullptr;
    UINT m_originalState = ABS_ALWAYSONTOP;
    RECT m_originalRect = {};
    bool GetTaskbarRect(RECT& rect);
};
