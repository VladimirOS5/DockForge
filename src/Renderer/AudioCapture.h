#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <atomic>
#include <thread>
#include <vector>

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();
    bool Initialize();
    void Shutdown();
    float GetRMSLevel() const { return m_rms.load(); }
    bool IsRunning() const { return m_running; }
private:
    void CaptureThread();
    std::atomic<float> m_rms{0.0f};
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_captureClient;
    WAVEFORMATEX* m_waveFormat = nullptr;
    UINT32 m_bufferFrameCount = 0;
};