#include "AudioCapture.h"
#include "../Utils/Logger.h"
#include <cmath>

AudioCapture::AudioCapture() = default;
AudioCapture::~AudioCapture() { Shutdown(); }

bool AudioCapture::Initialize() {
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

    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, m_waveFormat, nullptr);
    if (FAILED(hr)) { LOG_ERROR("Failed to initialize audio client"); return false; }

    hr = m_audioClient->GetBufferSize(&m_bufferFrameCount);
    if (FAILED(hr)) { LOG_ERROR("Failed to get buffer size"); return false; }

    hr = m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient));
    if (FAILED(hr)) { LOG_ERROR("Failed to get capture client"); return false; }

    hr = m_audioClient->Start();
    if (FAILED(hr)) { LOG_ERROR("Failed to start audio capture"); return false; }

    m_running = true;
    m_thread = std::thread(&AudioCapture::CaptureThread, this);
    LOG_INFO("AudioCapture initialized");
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

void AudioCapture::CaptureThread() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    UINT32 packetLength = 0;
    BYTE* pData = nullptr;
    DWORD flags = 0;
    UINT32 numFramesAvailable = 0;

    while (m_running) {
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr) || packetLength == 0) {
            Sleep(5);
            continue;
        }
        hr = m_captureClient->GetBuffer(&pData, &numFramesAvailable, &flags, nullptr, nullptr);
        if (SUCCEEDED(hr) && pData) {
            float sum = 0.0f;
            UINT32 channels = m_waveFormat->nChannels;
            if (m_waveFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
                (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)m_waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
                float* samples = (float*)pData;
                for (UINT32 i = 0; i < numFramesAvailable * channels; ++i) {
                    sum += samples[i] * samples[i];
                }
            } else if (m_waveFormat->wFormatTag == WAVE_FORMAT_PCM || 
                       (m_waveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)m_waveFormat)->SubFormat == KSDATAFORMAT_SUBTYPE_PCM)) {
                if (m_waveFormat->wBitsPerSample == 16) {
                    short* samples = (short*)pData;
                    for (UINT32 i = 0; i < numFramesAvailable * channels; ++i) {
                        float s = samples[i] / 32768.0f;
                        sum += s * s;
                    }
                }
            }
            float rms = sqrtf(sum / (numFramesAvailable * channels + 1));
            m_rms.store(rms);
            m_captureClient->ReleaseBuffer(numFramesAvailable);
        }
    }
    CoUninitialize();
}