#include "ParticleSystem.h"
#include <cmath>

void ParticleSystem::Initialize(int maxParticles) {
    m_maxParticles = maxParticles;
    m_particles.reserve(maxParticles);
    m_rng.seed(static_cast<unsigned>(GetTickCount()));
}

void ParticleSystem::Update(float deltaTime, float audioLevel) {
    m_spawnTimer += deltaTime;
    float spawnRate = 0.02f / (audioLevel + 0.1f);
    while (m_spawnTimer > spawnRate) {
        m_spawnTimer -= spawnRate;
        Spawn(audioLevel);
    }

    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        it->x += it->vx * deltaTime;
        it->y += it->vy * deltaTime;
        it->life -= deltaTime / it->maxLife;
        if (it->life <= 0) it = m_particles.erase(it);
        else ++it;
    }
}

void ParticleSystem::Spawn(float audioLevel) {
    if (static_cast<int>(m_particles.size()) >= m_maxParticles) return;
    Particle p;
    std::uniform_real_distribution<float> distX(0, m_width);
    std::uniform_real_distribution<float> distV(-20, 20);
    std::uniform_real_distribution<float> distLife(2, 5);
    std::uniform_real_distribution<float> distSize(1, 4);
    p.x = distX(m_rng);
    p.y = m_height + 5;
    p.vx = distV(m_rng);
    p.vy = -30.0f - audioLevel * 100.0f;
    p.maxLife = distLife(m_rng);
    p.life = p.maxLife;
    p.size = distSize(m_rng) * (1.0f + audioLevel * 2.0f);

    float hue = fmodf(GetTickCount() / 10000.0f + audioLevel, 1.0f);
    // Simple HSV-ish to RGB
    auto hsv = [&](float h, float s, float v) -> D2D1_COLOR_F {
        int i = int(h * 6);
        float f = h * 6 - i;
        float p = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);
        switch (i % 6) {
            case 0: return {v, t, p, 0.6f};
            case 1: return {q, v, p, 0.6f};
            case 2: return {p, v, t, 0.6f};
            case 3: return {p, q, v, 0.6f};
            case 4: return {t, p, v, 0.6f};
            case 5: return {v, p, q, 0.6f};
        }
        return {v, p, q, 0.6f};
    };
    p.color = hsv(hue, 0.8f, 1.0f);
    m_particles.push_back(p);
}

void ParticleSystem::Render(ID2D1RenderTarget* rt) {
    for (const auto& p : m_particles) {
        float alpha = p.life / p.maxLife * p.color.a;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        rt->CreateSolidColorBrush({p.color.r, p.color.g, p.color.b, alpha}, &brush);
        rt->FillEllipse(D2D1::Ellipse({p.x, p.y}, p.size, p.size), brush.Get());
    }
}