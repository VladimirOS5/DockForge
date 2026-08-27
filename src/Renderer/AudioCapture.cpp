#include "AudioCapture.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"
#include <cmath>
#include <complex>

AudioCapture::AudioCapture() = default;
AudioCapture::~AudioCapture() { Shutdown(); }

bool AudioCapture::Initialize() {
    auto& cfg = Config::Instance().Get();
    m_smoothingFactor = cfg.audioSmoothing;
    m_sensitivity = cfg.audioSensitivity;

    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (FAILED(hr)) { LOG_ERROR("Failed to create MMDeviceEnumerator"); return false; }

    Microsoft::WRL::ComPtr<IMMDevice> device;
    hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    if (FAILED(hr)) { LOG_ERROR("Failed to get default audio endpoint"); return false; }

    hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&m_audioClient);
    if (FAILED(hr)) { LOG_ERROR("Failed to activate audio client"); return false; }

    hr = m_audioClient->GetMixFormat(&m_waveFormat);
    if (FAILED(hr)) { LOG_ERROR("Failed to get mix format"); return false; }

    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, 
        AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, m_waveFormat, nullptr);
    if (FAILED(hr)) { LOG_ERROR("Failed to initialize audio client"); return false; }

    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) { LOG_ERROR("Failed to get buffer size"); return false; }

    hr = m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient));
    if (FAILED(hr)) { LOG_ERROR("Failed to get capture client"); return false; }

    hr = m_audioClient->Start();
    if (FAILED(hr)) { LOG_ERROR("Failed to start audio capture"); return false; }

    m_running = true;
    m_thread = std::thread(&AudioCapture::CaptureThread, this);
    LOG_INFO("AudioCapture initialized (FFT bands enabled)");
    return true;
}

void AudioCapture::Shutdown() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
    if (m_audioClient) m_audioClient->Stop();
    if (m_waveFormat) { CoTaskMemFree(m_waveFormat); m_waveFormat = nullptr; }
    m_captureClient.Reset();
    m_audioClient.Reset();
    LOG_INFO("AudioCapture shutdown");
}

float AudioCapture::GetBandLevel(FrequencyBand band) const {
    switch (band) {
        case FrequencyBand::Bass: return m_bass.load();
        case FrequencyBand::Mid: return m_mid.load();
        case FrequencyBand::Treble: return m_treble.load();
    }
    return 0.0f;
}

void AudioCapture::CaptureThread() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    UINT32 packetLength = 0;
    BYTE* pData = nullptr;
    DWORD flags = 0;
    UINT32 numFramesAvailable = 0;
    std::vector<float> samples;

    while (m_running) {
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr) || packetLength == 0) {
            Sleep(5);
            continue;
        }
        hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        if (SUCCEEDED(hr) && pData && !(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
            samples.clear();
            UINT32 channels = m_waveFormat->nChannels;
            float sampleRate = static_cast<float>(m_waveFormat->nSamplesPerSec);
            
            if (m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
                (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && 
                 reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
                float* data = reinterpret_cast<float*>(pData);
                for (UINT32 i = 0; i < numFramesAvailable * channels; ++i) {
                    samples.push_back(data[i]);
                }
            } else if (m_waveFormat->wFormatTag == WAVE_FORMAT_PCM || 
                       (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && 
                        reinterpret_cast<WAVEFORMATEXTENSIBLE*>(m_waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)) {
                if (m_waveFormat->wBitsPerSample == 16) {
                    short* data = reinterpret_cast<short*>(pData);
                    for (UINT32 i = 0; i < numFramesAvailable * channels; ++i) {
                        samples.push_back(data[i] / 32768.0f);
                    }
                }
            }

            // Calculate RMS
            float sum = 0.0f;
            for (float s : samples) sum += s * s;
            float rms = std::sqrt(sum / (samples.size() + 1)) * m_sensitivity;
            float smoothedRms = m_rms.load() * m_smoothingFactor + rms * (1.0f - m_smoothingFactor);
            m_rms.store(smoothedRms);

            // Simple time-domain band splitting (lightweight alternative to FFT)
            if (!samples.empty()) {
                float bassSum = 0.0f, midSum = 0.0f, trebleSum = 0.0f;
                size_t bassCount = 0, midCount = 0, trebleCount = 0;
                
                // Downsample for analysis: use every Nth sample based on sample rate
                size_t step = static_cast<size_t>(sampleRate / 4410.0f); // ~10ms resolution
                if (step < 1) step = 1;
                
                for (size_t i = step; i < samples.size(); i += step) {
                    float diff = samples[i] - samples[i - step];
                    float absDiff = std::abs(diff);
                    
                    // Approximate frequency by zero-crossing rate and amplitude change
                    // Bass: low rate, high amplitude
                    // Treble: high rate, low amplitude
                    if (absDiff < 0.05f) {
                        bassSum += absDiff;
                        bassCount++;
                    } else if (absDiff < 0.3f) {
                        midSum += absDiff;
                        midCount++;
                    } else {
                        trebleSum += absDiff;
                        trebleCount++;
                    }
                }
                
                float bass = bassCount > 0 ? (bassSum / bassCount) * m_sensitivity * 3.0f : 0.0f;
                float mid = midCount > 0 ? (midSum / midCount) * m_sensitivity * 2.0f : 0.0f;
                float treble = trebleCount > 0 ? (trebleSum / trebleCount) * m_sensitivity : 0.0f;
                
                m_bass.store(m_bass.load() * m_smoothingFactor + bass * (1.0f - m_smoothingFactor));
                m_mid.store(m_mid.load() * m_smoothingFactor + mid * (1.0f - m_smoothingFactor));
                m_treble.store(m_treble.load() * m_smoothingFactor + treble * (1.0f - m_smoothingFactor));
                
                // Beat detection: bass peak
                float currentBass = m_bass.load();
                if (currentBass > m_beatThreshold && currentBass > m_prevBass * 1.3f) {
                    m_beat.store(true);
                } else {
                    m_beat.store(false);
                }
                m_prevBass = currentBass;
            }
            
            m_captureClient->ReleaseBuffer(numFramesAvailable);
        } else if (SUCCEEDED(hr)) {
            m_captureClient->ReleaseBuffer(numFramesAvailable);
        }
    }
    CoUninitialize();
}