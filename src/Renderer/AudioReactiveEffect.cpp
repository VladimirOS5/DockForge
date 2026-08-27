#include "AudioReactiveEffect.h"
#include "../Utils/Logger.h"
#include "../Utils/Config.h"

bool AudioReactiveEffect::Initialize() {
    auto& cfg = Config::Instance().Get();
    
    // Parse mode from config
    if (cfg.audioReactiveMode == "off") m_mode = AudioReactiveMode::Off;
    else if (cfg.audioReactiveMode == "particles") m_mode = AudioReactiveMode::Particles;
    else if (cfg.audioReactiveMode == "gradient") m_mode = AudioReactiveMode::Gradient;
    else if (cfg.audioReactiveMode == "wallpaper") m_mode = AudioReactiveMode::Wallpaper;
    else m_mode = AudioReactiveMode::Both;
    
    if (m_mode == AudioReactiveMode::Off) {
        m_initialized = true;
        return true;
    }
    
    m_audioCapture = std::make_unique<AudioCapture>();
    if (!m_audioCapture->Initialize()) {
        LOG_WARN("AudioCapture failed to initialize, continuing without audio-reactive effects");
        m_audioCapture.reset();
    }
    
    if (m_mode == AudioReactiveMode::Particles || m_mode == AudioReactiveMode::Both) {
        m_particles.Initialize(cfg.particleCount);
        m_particles.SetGravity(20.0f); // Particles fall down, then bounce
        m_particles.SetWind(5.0f);     // Slight drift
    }
    
    m_initialized = true;
    LOG_INFO("AudioReactiveEffect initialized (mode: " + cfg.audioReactiveMode + ")");
    return true;
}

void AudioReactiveEffect::Shutdown() {
    if (m_audioCapture) m_audioCapture->Shutdown();
    m_initialized = false;
}

void AudioReactiveEffect::Update(float deltaTime) {
    if (!m_initialized || m_mode == AudioReactiveMode::Off) return;
    
    float level = GetCurrentAudioLevel();
    float bass = GetBassLevel();
    float mid = GetMidLevel();
    float treble = GetTrebleLevel();
    
    if (m_mode == AudioReactiveMode::Gradient || m_mode == AudioReactiveMode::Both) {
        m_gradient.Update(deltaTime, level, bass, mid);
    }
    
    if (m_mode == AudioReactiveMode::Particles || m_mode == AudioReactiveMode::Both) {
        m_particles.SetBounds(m_width, m_height);
        m_particles.Update(deltaTime, level, bass, treble);
    }
}

void AudioReactiveEffect::Render(ID2D1RenderTarget* rt, float x, float y, float w, float h) {
    if (!m_initialized || m_mode == AudioReactiveMode::Off) return;
    
    m_width = w;
    m_height = h;
    
    auto& cfg = Config::Instance().Get();
    
    if (m_mode == AudioReactiveMode::Gradient || m_mode == AudioReactiveMode::Both) {
        GradientDirection dir = GradientDirection::Horizontal;
        if (cfg.gradientDirection == "vertical") dir = GradientDirection::Vertical;
        else if (cfg.gradientDirection == "radial") dir = GradientDirection::Radial;
        m_gradient.Render(rt, x, y, w, h, dir);
    }
    
    if (m_mode == AudioReactiveMode::Particles || m_mode == AudioReactiveMode::Both) {
        m_particles.SetBounds(w, h);
        m_particles.Render(rt, cfg.particleGlow, cfg.particleTrails);
    }
}

float AudioReactiveEffect::GetCurrentAudioLevel() const {
    if (!m_audioCapture) return 0.0f;
    return m_audioCapture->GetRMSLevel();
}

float AudioReactiveEffect::GetBassLevel() const {
    if (!m_audioCapture) return 0.0f;
    return m_audioCapture->GetBandLevel(FrequencyBand::Bass);
}

float AudioReactiveEffect::GetMidLevel() const {
    if (!m_audioCapture) return 0.0f;
    return m_audioCapture->GetBandLevel(FrequencyBand::Mid);
}

float AudioReactiveEffect::GetTrebleLevel() const {
    if (!m_audioCapture) return 0.0f;
    return m_audioCapture->GetBandLevel(FrequencyBand::Treble);
}

bool AudioReactiveEffect::IsBeat() const {
    if (!m_audioCapture) return false;
    return m_audioCapture->IsBeat();
}