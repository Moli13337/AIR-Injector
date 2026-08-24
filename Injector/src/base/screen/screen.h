#ifndef SCREEN_H
#define SCREEN_H

#include <imgui.h>
#include <d3d9.h>
#include <windows.h>

class Screen {
public:
    void SetupStyle();
    bool Render();
    
    // Set the window handle for proper closing
    void SetWindowHandle(HWND handle) { m_hwnd = handle; }

private:
    HWND m_hwnd = nullptr;
};

#endif // SCREEN_H