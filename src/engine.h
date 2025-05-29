#pragma once

#include "contexting.h"

namespace SLT {
struct Salty {
public:
    Salty() = default;

    static Salty& GetSingleton() {
        static Salty singleton;
        return singleton;
    }

    bool ParseScript(FrameContext* frame);

    bool IsStepRunnable(std::vector<std::string>& step);

    bool RunStep(std::vector<std::string>& step);

    void AdvanceToNextRunnableStep(FrameContext* frame);

private:
    Salty(const Salty&) = delete;
    Salty& operator=(const Salty&) = delete;
    Salty(Salty&&) = delete;
    Salty& operator=(Salty&&) = delete;
};
}