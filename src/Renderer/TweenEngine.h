#pragma once
#include <functional>
#include <vector>
#include <string>

enum class EasingType {
    Linear,
    EaseOut,
    EaseInOut,
    EaseOutBack,
    EaseOutBounce,
    EaseOutCirc,
    Elastic
};

EasingType EasingFromString(const std::string& name);

using EasingFunction = std::function<float(float)>;
EasingFunction GetEasingFunction(EasingType type);

struct Tween {
    float* target = nullptr;
    float startValue = 0.0f;
    float endValue = 0.0f;
    float duration = 1.0f;
    float elapsed = 0.0f;
    EasingFunction easing;
    std::function<void()> onComplete;
    bool active = false;
    int id = 0;
};

class TweenEngine {
public:
    static TweenEngine& Instance();
    int AddTween(float* target, float start, float end, float durationMs, EasingType easing, std::function<void()> onComplete = nullptr);
    void Update(float deltaTime);
    void RemoveTween(int id);
    void Clear();
    bool HasActiveTweens() const;
private:
    TweenEngine() = default;
    std::vector<Tween> m_tweens;
    int m_nextId = 1;
};
