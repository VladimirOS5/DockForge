#pragma once
#include <windows.h>

class TaskbarHider {
public:
    TaskbarHider();
    ~TaskbarHider();
    bool Hide();
    bool Restore();
    bool Show(); // Alias for Restore() for compatibility
    bool IsHidden() const { return m_hidden; }
private:
    bool GetTaskbarRect(RECT& rect);
    HWND m_hTaskbar = nullptr;
    HWND m_hStart = nullptr;
    bool m_hidden = false;
    RECT m_originalRect = {};
    UINT m_originalState = 0;
};
