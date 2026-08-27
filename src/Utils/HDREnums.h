#pragma once
#include <windows.h>

// HDR display detection and color space helpers
enum class DisplayColorSpace {
    SDR,           // sRGB
    HDR10,         // ST.2084 (PQ)
    HDR_HLG,       // Hybrid Log-Gamma
    HDR_SCRGB,     // scRGB (linear, float)
    Unknown
};

class HDRHelper {
public:
    static bool IsHDRSupported();
    static bool IsHDRActive();
    static DisplayColorSpace GetCurrentColorSpace();
    static float GetSDRWhiteLevel(); // nits
    static void LogDisplayInfo();
private:
    static bool QueryDisplayAdvancedColorInfo();
};
