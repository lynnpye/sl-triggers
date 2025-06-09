#pragma once

namespace SLT {

class VoidCallbackFunctor : public RE::BSScript::IStackCallbackFunctor {
public:
    explicit VoidCallbackFunctor(std::function<void()> callback)
        : onDone(std::move(callback)) {}

    void operator()(RE::BSScript::Variable) override {
        if (onDone) {
            onDone();
        }
    }

    void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

private:
    std::function<void()> onDone;
};

class OperationRunner {
public:
    static bool RunOperationOnActor(RE::Actor* targetActor, 
                                   RE::ActiveEffect* cmdPrimary, 
                                   const std::vector<RE::BSFixedString>& params);
    
    static bool RunOperationOnActor(RE::Actor* targetActor, 
                                   RE::ActiveEffect* cmdPrimary, 
                                   const std::vector<std::string>& params);
};

} // namespace SLT