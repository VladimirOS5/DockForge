#pragma once
#include <d2d1.h>
#include <vector>
#include <random>

struct Particle {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    float life = 1.0f;
    float maxLife = 1.0f;
    float size = 2.0f;
    D2D1_COLOR_F color = {1,1,1,1};
};

class ParticleSystem {
public:
    void Initialize(int maxParticles);
    void Update(float deltaTime, float audioLevel);
    void Render(ID2D1RenderTarget* rt);
    void SetBounds(float w, float h) { m_width = w; m_height = h; }
private:
    void Spawn(float audioLevel);
    std::vector<Particle> m_particles;
    int m_maxParticles = 200;
    float m_width = 100, m_height = 100;
    float m_spawnTimer = 0;
    std::mt19937 m_rng;
};