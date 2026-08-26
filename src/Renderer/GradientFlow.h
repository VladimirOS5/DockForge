#pragma once
#include <d2d1.h>

class GradientFlow {
public:
    void Update(float deltaTime);
    void Render(ID2D1RenderTarget* rt, float x, float y, float w, float h);
private:
    float m_offset = 0.0f;
};