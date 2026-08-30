#include "ParticleSystem.h"
#include <cmath>

void ParticleSystem::Initialize(int maxParticles) {
    m_maxParticles = maxParticles;
    m_particles.reserve(maxParticles);
    m_rng.seed(static_cast<unsigned>(GetTickCount64()));
    m_resourcesInitialized = false;
}

void ParticleSystem::Clear() {
    m_particles.clear();
}

void ParticleSystem::Update(float deltaTime, float audioLevel, float bassLevel, float trebleLevel) {
    m_time += deltaTime;
    m_spawnTimer += deltaTime;

    // Audio-reactive spawn rate: more audio = more particles
    float spawnRate = std::max(0.005f, 0.05f / (1.0f + audioLevel * 5.0f));
    while (m_spawnTimer > spawnRate) {
        m_spawnTimer -= spawnRate;
        Spawn(audioLevel, bassLevel, trebleLevel);
    }

    for (auto it = m_particles.begin(); it != m_particles.end(); ) {
        // Store trail positions
        if (m_trailsEnabled) {
            it->trailX[2] = it->trailX[1];
            it->trailY[2] = it->trailY[1];
            it->trailX[1] = it->trailX[0];
            it->trailY[1] = it->trailY[0];
            it->trailX[0] = it->x;
            it->trailY[0] = it->y;
        }

        // Physics
        it->vy += m_gravity * deltaTime;
        it->vx += m_wind * deltaTime;
        it->x += it->vx * deltaTime;
        it->y += it->vy * deltaTime;
        it->rotation += it->rotationSpeed * deltaTime;

        // Bounce off bottom
        if (it->y > m_height + it->size) {
            it->y = m_height + it->size;
            it->vy *= -0.4f;
            it->vx *= 0.8f;
        }

        // Bounce off sides
        if (it->x < -it->size) { it->x = -it->size; it->vx *= -0.6f; }
        if (it->x > m_width + it->size) { it->x = m_width + it->size; it->vx *= -0.6f; }

        // Life decay
        it->life -= deltaTime / it->maxLife;

        // Size pulse with treble
        it->size = it->baseSize * (1.0f + trebleLevel * 0.5f * std::sin(m_time * 10.0f + it->x));

        if (it->life <= 0) it = m_particles.erase(it);
        else ++it;
    }
}

void ParticleSystem::Spawn(float audioLevel, float bassLevel, float trebleLevel) {
    if (static_cast<int>(m_particles.size()) >= m_maxParticles) return;

    Particle p;
    std::uniform_real_distribution<float> distX(0, m_width);
    std::uniform_real_distribution<float> distV(-30, 30);
    std::uniform_real_distribution<float> distLife(1.5f, 4.0f);
    std::uniform_real_distribution<float> distSize(1.5f, 5.0f);
    std::uniform_int_distribution<int> distShape(0, 2);

    p.x = distX(m_rng);
    p.y = m_height + 5; // Spawn from bottom
    p.vx = distV(m_rng) * (1.0f + bassLevel);
    p.vy = -40.0f - audioLevel * 150.0f - bassLevel * 80.0f; // Bass pushes particles higher
    p.maxLife = distLife(m_rng) * (1.0f + trebleLevel * 0.5f);
    p.life = p.maxLife;
    p.baseSize = distSize(m_rng) * (1.0f + audioLevel * 2.0f + bassLevel);
    p.size = p.baseSize;
    p.rotationSpeed = distV(m_rng) * 2.0f;
    p.shape = static_cast<ParticleShape>(distShape(m_rng));

    // Color based on audio bands
    float hue = fmodf(m_time * 0.1f + bassLevel * 0.3f + trebleLevel * 0.2f, 1.0f);
    auto hsvToRgb = [&](float h, float s, float v) -> D2D1_COLOR_F {
        int i = static_cast<int>(h * 6);
        float f = h * 6 - i;
        float p_val = v * (1 - s);
        float q = v * (1 - f * s);
        float t = v * (1 - (1 - f) * s);
        switch (i % 6) {
            case 0: return {v, t, p_val, 0.7f};
            case 1: return {q, v, p_val, 0.7f};
            case 2: return {p_val, v, t, 0.7f};
            case 3: return {p_val, q, v, 0.7f};
            case 4: return {t, p_val, v, 0.7f};
            case 5: return {v, p_val, q, 0.7f};
        }
        return {v, p_val, q, 0.7f};
    };

    // Bass = warm colors (red/orange), Treble = cool colors (blue/purple)
    float sat = 0.6f + audioLevel * 0.4f;
    float val = 0.8f + audioLevel * 0.2f;
    p.color = hsvToRgb(hue, sat, val);

    m_particles.push_back(p);
}

void ParticleSystem::Render(ID2D1RenderTarget* rt, bool glowEnabled, bool trailsEnabled) {
    if (!m_resourcesInitialized) {
        rt->CreateSolidColorBrush(D2D1::ColorF(1,1,1,1), &m_sharedBrush);
        m_resourcesInitialized = true;
    }

    // First pass: glow (if enabled)
    if (glowEnabled) {
        for (const auto& p : m_particles) {
            float alpha = (p.life / p.maxLife) * p.color.a;
            if (alpha > 0.01f) RenderGlow(rt, p, alpha);
        }
    }

    // Second pass: trails
    if (trailsEnabled) {
        for (const auto& p : m_particles) {
            float alpha = (p.life / p.maxLife) * p.color.a * 0.4f;
            if (alpha > 0.01f && m_sharedBrush) {
                for (int i = 0; i < 3; ++i) {
                    float trailAlpha = alpha * (0.3f - i * 0.1f);
                    m_sharedBrush->SetColor({p.color.r, p.color.g, p.color.b, trailAlpha});
                    float trailSize = p.size * (0.6f - i * 0.15f);
                    rt->FillEllipse(D2D1::Ellipse({p.trailX[i], p.trailY[i]}, trailSize, trailSize), m_sharedBrush.Get());
                }
            }
        }
    }

    // Third pass: core particles
    for (const auto& p : m_particles) {
        float alpha = (p.life / p.maxLife) * p.color.a;
        if (alpha <= 0.01f) continue;

        if (m_sharedBrush) {
            m_sharedBrush->SetColor({p.color.r, p.color.g, p.color.b, alpha});

            D2D1_MATRIX_3X2_F oldTransform;
            rt->GetTransform(&oldTransform);

            D2D1_MATRIX_3X2_F transform = D2D1::Matrix3x2F::Rotation(
                p.rotation * 57.2958f, {p.x, p.y});
            rt->SetTransform(transform);

            switch (p.shape) {
                case ParticleShape::Circle:
                    rt->FillEllipse(D2D1::Ellipse({p.x, p.y}, p.size, p.size), m_sharedBrush.Get());
                    break;
                case ParticleShape::Diamond:
                    rt->FillRectangle(D2D1::RectF(p.x - p.size, p.y - p.size*0.6f, 
                        p.x + p.size, p.y + p.size*0.6f), m_sharedBrush.Get());
                    break;
                case ParticleShape::Star: {
                    // Simple 4-point star
                    float s = p.size;
                    D2D1_POINT_2F points[8] = {
                        {p.x, p.y - s}, {p.x + s*0.3f, p.y - s*0.3f},
                        {p.x + s, p.y}, {p.x + s*0.3f, p.y + s*0.3f},
                        {p.x, p.y + s}, {p.x - s*0.3f, p.y + s*0.3f},
                        {p.x - s, p.y}, {p.x - s*0.3f, p.y - s*0.3f}
                    };
                    Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                    Microsoft::WRL::ComPtr<ID2D1Factory> factory;
                    rt->GetFactory(&factory);
                    if (factory) {
                        factory->CreatePathGeometry(&path);
                        if (path) {
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            path->Open(&sink);
                            sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
                            for (int i = 1; i < 8; ++i) sink->AddLine(points[i]);
                            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                            sink->Close();
                            rt->FillGeometry(path.Get(), m_sharedBrush.Get());
                        }
                    }
                    break;
                }
            }

            rt->SetTransform(oldTransform);
        }
    }
}

void ParticleSystem::RenderGlow(ID2D1RenderTarget* rt, const Particle& p, float alpha) {
    if (!m_sharedBrush) return;
    float glowSize = p.size * 2.5f;
    float glowAlpha = alpha * 0.25f;
    m_sharedBrush->SetColor({p.color.r, p.color.g, p.color.b, glowAlpha});
    rt->FillEllipse(D2D1::Ellipse({p.x, p.y}, glowSize, glowSize), m_sharedBrush.Get());
}
