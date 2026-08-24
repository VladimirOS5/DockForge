cmake_minimum_required(VERSION 3.20)
project(DockForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(DockForge WIN32
    src/Core/Main.cpp
    src/Core/Application.cpp
    src/Core/Application.h
    src/Core/DockWindow.cpp
    src/Core/DockWindow.h
    src/Shell/TaskbarHider.cpp
    src/Shell/TaskbarHider.h
    src/Utils/Logger.cpp
    src/Utils/Logger.h
    src/Utils/Config.cpp
    src/Utils/Config.h
    src/Utils/ScreenCapture.cpp
    src/Utils/ScreenCapture.h
    src/Renderer/D2DRenderer.cpp
    src/Renderer/D2DRenderer.h
    src/Renderer/EffectRenderer.cpp
    src/Renderer/EffectRenderer.h
    src/Renderer/Effects.cpp
    src/Renderer/Effects.h
    src/Renderer/FrameLimiter.cpp
    src/Renderer/FrameLimiter.h
    src/Renderer/IconLoader.cpp
    src/Renderer/IconLoader.h
    src/Renderer/TextureAtlas.cpp
    src/Renderer/TextureAtlas.h
    src/Renderer/AnimatedIcon.cpp
    src/Renderer/AnimatedIcon.h
    src/Renderer/BadgeRenderer.cpp
    src/Renderer/BadgeRenderer.h
)

target_compile_definitions(DockForge PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(DockForge PRIVATE
    d2d1 dwrite windowscodecs shlwapi user32 shell32 gdi32 ole32 oleaut32 d3d11 dxgi comctl32
)

add_subdirectory(watchdog)
