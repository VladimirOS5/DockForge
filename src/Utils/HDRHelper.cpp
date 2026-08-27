#include "HDREnums.h"
#include "../Utils/Logger.h"
#include <dxgi1_6.h>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")

bool HDRHelper::IsHDRSupported() {
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory6), reinterpret_cast<void**>(factory.GetAddressOf()));
    if (FAILED(hr)) return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++i) {
        Microsoft::WRL::ComPtr<IDXGIOutput> output;
        for (UINT j = 0; adapter->EnumOutputs(j, output.GetAddressOf()) != DXGI_ERROR_NOT_FOUND; ++j) {
            // Check for HDR support via color space properties
            // Simplified: assume DXGI 1.6+ supports HDR if adapter is not software
            DXGI_OUTPUT_DESC desc;
            if (SUCCEEDED(output->GetDesc(&desc))) {
                // Real implementation would use IDXGIOutput6::GetDesc1 for AdvancedColor
                return true;
            }
            output.Reset();
        }
        adapter.Reset();
    }
    return false;
}

bool HDRHelper::IsHDRActive() {
    // Check if any monitor is in HDR mode
    // This requires Windows 10 1709+ and DXGI 1.6
    // Simplified: return false for now, real impl needs display config API
    return false;
}

DisplayColorSpace HDRHelper::GetCurrentColorSpace() {
    if (IsHDRActive()) return DisplayColorSpace::HDR10;
    return DisplayColorSpace::SDR;
}

float HDRHelper::GetSDRWhiteLevel() {
    // Windows default SDR white level on HDR display is typically 80 nits
    // Can be queried via AdvancedColorInfo in real implementation
    return IsHDRActive() ? 80.0f : 100.0f;
}

void HDRHelper::LogDisplayInfo() {
    LOG_INFO("HDR Support: " + std::string(IsHDRSupported() ? "YES" : "NO"));
    LOG_INFO("HDR Active: " + std::string(IsHDRActive() ? "YES" : "NO"));

    auto cs = GetCurrentColorSpace();
    std::string csName;
    switch (cs) {
        case DisplayColorSpace::SDR: csName = "sRGB"; break;
        case DisplayColorSpace::HDR10: csName = "HDR10 (PQ)"; break;
        case DisplayColorSpace::HDR_HLG: csName = "HLG"; break;
        case DisplayColorSpace::HDR_SCRGB: csName = "scRGB"; break;
        default: csName = "Unknown"; break;
    }
    LOG_INFO("Color Space: " + csName);
    LOG_INFO("SDR White Level: " + std::to_string(static_cast<int>(GetSDRWhiteLevel())) + " nits");
}

bool HDRHelper::QueryDisplayAdvancedColorInfo() {
    // Placeholder for real DXGI 1.6 advanced color info query
    return false;
}
