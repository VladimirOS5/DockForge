#include "AudioReactiveEffect.h"
#include "../Utils/Logger.h"

bool AudioReactiveEffect::Initialize() {
    m_audioCapture = std::make_unique<AudioCapture>();
    if (!m_audioCapture->Initialize()) {
        LOG_WARN("AudioCapture failed to initialize, continuing without audio-reactive effects");
        m_audioCapture.reset();
    }
    m_particles.Initialize(300);
    m_initialized = true;
    LOG_INFO("AudioReactiveEffect initialized");
    return true;
}

void AudioReactiveEffect::Shutdown() {
    if (m_audioCapture) m_audioCapture->Shutdown();
    m_initialized = false;
}

void AudioReactiveEffect::Update(float deltaTime) {
    m_gradient.Update(deltaTime);
    float level = GetCurrentAudioLevel();
    m_particles.SetBounds(800, 64); // will be overridden in render
    m_particles.Update(deltaTime, level);
}

void AudioReactiveEffect::Render(ID2D1RenderTarget* rt, float x, float y, float w, float h) {
    if (!m_initialized) return;
    m_particles.SetBounds(w, h);
    m_gradient.Render(rt, x, y, w, h);
    m_particles.Render(rt);
}

float AudioReactiveEffect::GetCurrentAudioLevel() const {
    if (!m_audioCapture) return 0.0f;
    return m_audioCapture->GetRMSLevel();
}