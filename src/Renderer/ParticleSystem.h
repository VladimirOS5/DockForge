#pragma once
#include <d2d1.h>
#include <d2d1_1.h>
#include <vector>
#include <random>
#include <wrl/client.h>

enum class ParticleShape { Circle, Diamond, Star };

struct Particle {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 2.0f;
    float baseSize = 2.0f;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
    D2D1_COLOR_F color = {1,1,1,1};
    ParticleShape shape = ParticleShape::Circle;
    float trailX[3] = {0,0,0};
    float trailY[3] = {0,0,0};
};

class ParticleSystem {
public:
    void Initialize(int maxParticles);
    void Update(float deltaTime, float audioLevel, float bassLevel, float trebleLevel);
    void Render(ID2D1RenderTarget* rt, bool glowEnabled, bool trailsEnabled);
    void SetBounds(float w, float h) { m_width = w; m_height = h; }
    void SetGravity(float g) { m_gravity = g; }
    void SetWind(float w) { m_wind = w; }
    void Clear();
private:
    void Spawn(float audioLevel, float bassLevel, float trebleLevel);
    void RenderParticle(ID2D1RenderTarget* rt, const Particle& p, float alpha);
    void RenderGlow(ID2D1RenderTarget* rt, const Particle& p, float alpha);
    
    std::vector<Particle> m_particles;
    int m_maxParticles = 200;
    float m_width = 100, m_height = 100;
    float m_gravity = 15.0f;
    float m_wind = 0.0f;
    float m_spawnTimer = 0;
    float m_time = 0;
    std::mt19937 m_rng;
    
    // Cached D2D resources
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> m_sharedBrush;
    Microsoft::WRL::ComPtr<ID2D1Effect> m_glowEffect;
    bool m_resourcesInitialized = false;
};