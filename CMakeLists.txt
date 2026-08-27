cmake_minimum_required(VERSION 3.20)
project(DockForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable memory tracking in Debug builds
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DOCKFORGE_MEMORY_TRACKING=1)
endif()

include_directories(${CMAKE_SOURCE_DIR}/third_party)

add_executable(DockForge WIN32
    src/Core/Main.cpp
    src/Core/Application.cpp
    src/Core/Application.h
    src/Core/DockWindow.cpp
    src/Core/DockWindow.h
    src/Core/MonitorManager.cpp
    src/Core/MonitorManager.h
    src/Shell/TaskbarHider.cpp
    src/Shell/TaskbarHider.h
    src/Shell/ShellHookManager.cpp
    src/Shell/ShellHookManager.h
    src/Shell/WindowManager.cpp
    src/Shell/WindowManager.h
    src/Shell/ThumbnailPreview.cpp
    src/Shell/ThumbnailPreview.h
    src/Shell/JumpListManager.cpp
    src/Shell/JumpListManager.h
    src/Shell/TrayIconManager.cpp
    src/Shell/TrayIconManager.h
    src/Shell/StartButton.cpp
    src/Shell/StartButton.h
    src/Shell/DockContextMenu.cpp
    src/Shell/DockContextMenu.h
    src/Shell/WallpaperEngineIntegration.cpp
    src/Shell/WallpaperEngineIntegration.h
    src/Shell/UWPHelper.cpp
    src/Shell/UWPHelper.h
    src/Settings/SettingsWindow.cpp
    src/Settings/SettingsWindow.h
    src/Plugin/PluginManager.cpp
    src/Plugin/PluginManager.h
    src/Plugin/BuiltIn/ClockWidget.cpp
    src/Plugin/BuiltIn/ClockWidget.h
    src/Plugin/BuiltIn/SystemMonitorWidget.cpp
    src/Plugin/BuiltIn/SystemMonitorWidget.h
    src/Widget/DesktopWidgetWindow.cpp
    src/Widget/DesktopWidgetWindow.h
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
    src/Renderer/TweenEngine.cpp
    src/Renderer/TweenEngine.h
    src/Renderer/AudioCapture.cpp
    src/Renderer/AudioCapture.h
    src/Renderer/ParticleSystem.cpp
    src/Renderer/ParticleSystem.h
    src/Renderer/GradientFlow.cpp
    src/Renderer/GradientFlow.h
    src/Renderer/AudioReactiveEffect.cpp
    src/Renderer/AudioReactiveEffect.h
    src/Updater/OTAUpdater.cpp
    src/Updater/OTAUpdater.h
    src/Updater/VersionInfo.cpp
    src/Updater/VersionInfo.h
    src/Testing/MemoryTracker.cpp
    src/Testing/MemoryTracker.h
    src/Testing/StabilityTest.cpp
    src/Testing/StabilityTest.h
    src/Utils/Logger.cpp
    src/Utils/Logger.h
    src/Utils/Config.cpp
    src/Utils/Config.h
    src/Utils/Theme.cpp
    src/Utils/Theme.h
    src/Utils/PerformanceProfile.cpp
    src/Utils/PerformanceProfile.h
    src/Utils/ScreenCapture.cpp
    src/Utils/ScreenCapture.h
    src/Utils/FallbackManager.cpp
    src/Utils/FallbackManager.h
    src/Utils/DPIHelper.cpp
    src/Utils/DPIHelper.h
    src/Utils/HDREnums.h
    src/Utils/HDRHelper.cpp
)

target_compile_definitions(DockForge PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)

target_link_libraries(DockForge PRIVATE
    d2d1 dwrite windowscodecs shlwapi user32 shell32 gdi32 ole32 oleaut32 d3d11 dxgi comctl32 dwmapi psapi mmdevapi wininet crypt32 propsys
)

# Version info resource
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(VERSION_BUILD 0)

configure_file(
    ${CMAKE_SOURCE_DIR}/res/version.rc.in
    ${CMAKE_BINARY_DIR}/version.rc
    @ONLY
)

target_sources(DockForge PRIVATE ${CMAKE_BINARY_DIR}/version.rc)

add_subdirectory(watchdog)
