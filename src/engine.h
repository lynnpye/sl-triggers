#pragma once

#include "contexting.h"
#include "parsing.h"

namespace SLT {

#pragma region Function Libraries declaration
struct FunctionLibrary {

    static const std::string_view SLTCmdLib;
    static std::vector<std::unique_ptr<FunctionLibrary>> g_FunctionLibraries;
    static std::unordered_map<std::string, std::string> functionScriptCache;


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

#pragma region Salty, the SLTScript Engine
struct Salty {
public:
    Salty() = default;

    static Salty& GetSingleton() {
        static Salty singleton;
        return singleton;
    }

    /**
     * Salty will call the parser and pass it the FrameContext. When the parser is done,
     * the FrameContext will have its scriptTokens and co
     */
    static bool ParseScript(FrameContext* frame);

    static bool IsStepRunnable(const std::unique_ptr<CommandLine>& step);

    /**
     * RunStep
     * Will attempt to run the command pointed to currently by the FrameContext.
     * Will attempt to increment the currentLine value by 1 if possible afterward.
     */
    static bool RunStep(FrameContext* frame, SLTStackAnalyzer::AMEContextInfo& contextInfo);

    static bool AdvanceToNextRunnableStep(FrameContext* frame);

    static RE::TESForm* ResolveFormVariable(std::string_view token, FrameContext* frame);

private:
    Salty(const Salty&) = delete;
    Salty& operator=(const Salty&) = delete;
    Salty(Salty&&) = delete;
    Salty& operator=(Salty&&) = delete;
};
#pragma endregion
}