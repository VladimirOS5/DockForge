#include "TweenEngine.h"
#include "../Utils/Logger.h"
#include <cmath>

TweenEngine& TweenEngine::Instance() {
    static TweenEngine instance;
    return instance;
}

EasingType EasingFromString(const std::string& name) {
    if (name == "easeOut") return EasingType::EaseOut;
    if (name == "easeInOut") return EasingType::EaseInOut;
    if (name == "easeOutBack") return EasingType::EaseOutBack;
    if (name == "easeOutBounce") return EasingType::EaseOutBounce;
    if (name == "easeOutCirc") return EasingType::EaseOutCirc;
    if (name == "elastic") return EasingType::Elastic;
    return EasingType::Linear;
}

EasingFunction GetEasingFunction(EasingType type) {
    switch (type) {
        case EasingType::Linear:
            return [](float t) { return t; };
        case EasingType::EaseOut:
            return [](float t) { return 1.0f - (1.0f - t) * (1.0f - t); };
        case EasingType::EaseInOut:
            return [](float t) { return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2) / 2.0f; };
        case EasingType::EaseOutBack:
            return [](float t) {
                const float c1 = 1.70158f;
                const float c3 = c1 + 1.0f;
                return 1.0f + c3 * std::pow(t - 1.0f, 3) + c1 * std::pow(t - 1.0f, 2);
            };
        case EasingType::EaseOutBounce:
            return [](float t) {
                const float n1 = 7.5625f;
                const float d1 = 2.75f;
                if (t < 1.0f / d1) return n1 * t * t;
                else if (t < 2.0f / d1) return n1 * (t -= 1.5f / d1) * t + 0.75f;
                else if (t < 2.5f / d1) return n1 * (t -= 2.25f / d1) * t + 0.9375f;
                else return n1 * (t -= 2.625f / d1) * t + 0.984375f;
            };
        case EasingType::EaseOutCirc:
            return [](float t) { return std::sqrt(1.0f - std::pow(t - 1.0f, 2)); };
        case EasingType::Elastic:
            return [](float t) {
                const float c4 = (2.0f * 3.14159265f) / 3.0f;
                if (t == 0) return 0.0f;
                if (t == 1) return 1.0f;
                return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
            };
    }
    return [](float t) { return t; };
}

int TweenEngine::AddTween(float* target, float start, float end, float durationMs, EasingType easing, std::function<void()> onComplete) {
    if (!target || durationMs <= 0) return -1;
    Tween tween;
    tween.target = target;
    tween.startValue = start;
    tween.endValue = end;
    tween.duration = durationMs / 1000.0f;
    tween.elapsed = 0.0f;
    tween.easing = GetEasingFunction(easing);
    tween.onComplete = onComplete;
    tween.active = true;
    tween.id = m_nextId++;
    m_tweens.push_back(tween);
    return tween.id;
}

void TweenEngine::Update(float deltaTime) {
    for (auto it = m_tweens.begin(); it != m_tweens.end(); ) {
        if (!it->active || !it->target) { it = m_tweens.erase(it); continue; }

        it->elapsed += deltaTime;
        float t = std::min(it->elapsed / it->duration, 1.0f);
        float eased = it->easing(t);
        *it->target = it->startValue + (it->endValue - it->startValue) * eased;

        if (t >= 1.0f) {
            *it->target = it->endValue;
            if (it->onComplete) it->onComplete();
            it = m_tweens.erase(it);
        } else {
            ++it;
        }
    }
}

void TweenEngine::RemoveTween(int id) {
    for (auto it = m_tweens.begin(); it != m_tweens.end(); ++it) {
        if (it->id == id) { it->active = false; break; }
    }
}

void TweenEngine::Clear() {
    m_tweens.clear();
}

bool TweenEngine::HasActiveTweens() const {
    for (const auto& t : m_tweens) if (t.active) return true;
    return false;
}
