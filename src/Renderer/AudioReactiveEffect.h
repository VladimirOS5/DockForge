#pragma once
#include "AudioCapture.h"
#include "ParticleSystem.h"
#include "GradientFlow.h"
#include <memory>

class AudioReactiveEffect {
public:
    bool Initialize();
    void Shutdown();
    void Update(float deltaTime);
    void Render(ID2D1RenderTarget* rt, float x, float y, float w, float h);
    float GetCurrentAudioLevel() const;
private:
    std::unique_ptr<AudioCapture> m_audioCapture;
    ParticleSystem m_particles;
    GradientFlow m_gradient;
    bool m_initialized = false;
};