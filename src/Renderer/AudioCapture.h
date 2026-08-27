#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <atomic>
#include <thread>
#include <vector>
#include <wrl/client.h>

enum class FrequencyBand { Bass, Mid, Treble };

class AudioCapture {
public:
    AudioCapture();
    ~AudioCapture();
    bool Initialize();
    void Shutdown();
    
    // Overall RMS level (0.0 - 1.0, smoothed)
    float GetRMSLevel() const { return m_rms.load(); }
    
    // Frequency band levels (0.0 - 1.0, smoothed)
    float GetBandLevel(FrequencyBand band) const;
    
    // Peak detection
    bool IsBeat() const { return m_beat.load(); }
    
    bool IsRunning() const { return m_running; }
    
private:
    void CaptureThread();
    void ProcessFFT(const std::vector<float>& samples);
    void UpdateBands();
    
    std::atomic<float> m_rms{0.0f};
    std::atomic<float> m_bass{0.0f};
    std::atomic<float> m_mid{0.0f};
    std::atomic<float> m_treble{0.0f};
    std::atomic<bool> m_beat{false};
    std::atomic<bool> m_running{false};
    
    std::thread m_thread;
    Microsoft::WRL::ComPtr<IAudioClient> m_audioClient;
    Microsoft::WRL::ComPtr<IAudioCaptureClient> m_captureClient;
    WAVEFORMATEX* m_waveFormat = nullptr;
    UINT32 m_bufferFrameCount = 0;
    
    // FFT / smoothing state
    std::vector<float> m_fftBuffer;
    std::vector<float> m_window;
    float m_smoothingFactor = 0.85f;
    float m_sensitivity = 1.5f;
    float m_bassAccumulator = 0.0f;
    float m_beatThreshold = 0.15f;
    float m_prevBass = 0.0f;
};