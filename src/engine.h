#pragma once


namespace SLT {

#pragma region VoidCallbackFunctor
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
#pragma endregion

#pragma region OperationRunner
class OperationRunner {
public:
    static bool RunOperationOnActor(RE::Actor* targetActor, 
                                   RE::ActiveEffect* cmdPrimary, 
                                   const std::vector<RE::BSFixedString>& params);
    
    static bool RunOperationOnActor(RE::Actor* targetActor, 
                                   RE::ActiveEffect* cmdPrimary, 
                                   const std::vector<std::string>& params);
};
#pragma endregion

#pragma region Function Libraries declaration

struct CaseInsensitiveHash {
    std::size_t operator()(const std::string& key) const {
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(), ::tolower);
        return std::hash<std::string>{}(lower_key);
    }
};

struct CaseInsensitiveEqual {
    bool operator()(const std::string& lhs, const std::string& rhs) const {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }
};

struct FunctionLibrary {

    static const std::string_view SLTCmdLib;
    static std::vector<std::unique_ptr<FunctionLibrary>> g_FunctionLibraries;
    static std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual> functionScriptCache;


    std::string configFile;
    std::string functionFile;
    std::string extensionKey;
    std::int32_t priority;
    bool enabled;

    explicit FunctionLibrary(std::string_view _configFile, std::string_view _functionFile, std::string_view _extensionKey, std::int32_t _priority, bool _enabled = true)
        : configFile(_configFile), functionFile(_functionFile), extensionKey(_extensionKey), priority(_priority), enabled(_enabled) {}

    static FunctionLibrary* ByExtensionKey(std::string_view _extensionKey);
    static void GetFunctionLibraries();
    static bool PrecacheLibraries();
};
#pragma endregion
}