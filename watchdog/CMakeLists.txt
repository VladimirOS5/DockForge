add_executable(DockForge.Watchdog WIN32
    Main.cpp
    WatchdogProcess.cpp
    WatchdogProcess.h
)

target_compile_definitions(DockForge.Watchdog PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(DockForge.Watchdog PRIVATE user32 shell32)
