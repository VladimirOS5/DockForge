#pragma once
#include "AudioCapture.h"
#include "ParticleSystem.h"
#include "GradientFlow.h"
#include <memory>
#include <string>

enum class AudioReactiveMode { Off, Particles, Gradient, Both, Wallpaper };

class AudioReactiveEffect {
public:
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render(ID2D1RenderTarget* rt, float x, float y, float w, float h);
    float GetCurrentAudioLevel() const;
    float GetBassLevel() const;
    float GetMidLevel() const;
    float GetTrebleLevel() const;
    bool IsBeat() const;
    void SetMode(AudioReactiveMode mode) { m_mode = mode; }
    AudioReactiveMode GetMode() const { return m_mode; }
    bool IsInitialized() const { return m_initialized; }
private:
    std::unique_ptr<AudioCapture> m_audioCapture;
    ParticleSystem m_particles;
    GradientFlow m_gradient;
    AudioReactiveMode m_mode = AudioReactiveMode::Both;
    bool m_initialized = false;
    float m_width = 100, m_height = 100;
};