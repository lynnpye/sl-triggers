#pragma once

namespace SLT {

// Forward declarations
class ThreadContext;

/**
 * FrameContext - Handle-based wrapper for script execution frame context
 * Tracks local variables and state for the currently executing SLT script.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by GlobalContext
 */
class FrameContext : public ForgeObject {
private:

    mutable std::atomic<bool> threadFetchAttempted;
    mutable ThreadContext* thread;

    mutable std::shared_mutex varlock;

public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersFrameContext";

    FrameContext() = default;

    explicit FrameContext(ForgeHandle threadContextHandle, std::string_view scriptName)
        : threadContextHandle(threadContextHandle), scriptName(scriptName), currentLine(0), iterActor(nullptr),
            lastKey(0), iterActorFormID(0), isReadied(false), isReady(false),
            customResolveFormResult(nullptr), popAfterStepReturn(false), commandType(ScriptType::INVALID) {
        if (!ParseScript()) {
            std::string errorMsg = "Unable to parse script '" + std::string(scriptName) + "'";
            throw std::logic_error(errorMsg);
        }
    }

    bool ParseScript();

    // Transient (not serialized)
    RE::TESForm* customResolveFormResult;
    bool popAfterStepReturn;

    // Core data members
    ForgeHandle threadContextHandle;
    std::string scriptName;

    // Serialized state
    bool isReady;
    bool isReadied;
    std::vector<ForgeHandle> scriptTokens;
    std::size_t currentLine;
    ScriptType commandType;
    std::vector<std::string> callArgs;
    std::map<std::string, std::size_t> gotoLabelMap;
    std::map<std::string, std::size_t> gosubLabelMap;
    std::vector<std::size_t> returnStack;
    std::string mostRecentResult;
    std::uint32_t lastKey;
    RE::FormID iterActorFormID; // Serialize as formID
    RE::Actor* iterActor;
    std::map<std::string, std::string> localVars;

    // Utility methods
    ThreadContext* pthread() const;
    CommandLine* GetCommandLine(std::size_t _at) const;
    CommandLine* GetCommandLine() const;
    std::size_t GetCurrentActualLineNumber() const;

    // Variable management
    std::string SetLocalVar(std::string_view name, std::string_view value);
    std::string GetLocalVar(std::string_view name) const;
    bool HasLocalVar(std::string_view name) const;
    
    // ForgeObject implementation
    bool Serialize(SKSE::SerializationInterface* a_intfc) const override;
    bool Deserialize(SKSE::SerializationInterface* a_intfc) override;

    // Logging methods
    void LogInfo(const std::string& message) const;
    void LogWarn(const std::string& message) const;
    void LogError(const std::string& message) const;
    void LogDebug(const std::string& message) const;

    struct Helpers {
        // Factory functions
        static ForgeHandle CreateEmpty(RE::StaticFunctionTag*) {
            auto obj = std::make_unique<FrameContext>();
            return ForgeManagerTemplate<FrameContext>::Create(std::move(obj));
        }

        static ForgeHandle CreateNew(RE::StaticFunctionTag*, ForgeHandle threadContextHandle, std::string scriptName) {
            try {
                auto obj = std::make_unique<FrameContext>(threadContextHandle, scriptName);
                return ForgeManagerTemplate<FrameContext>::Create(std::move(obj));
            } catch (...) {
                return 0; // Invalid handle on failure
            }
        }

        // Basic accessors
        static std::string GetScriptName(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->scriptName : "";
        }

        static void SetScriptName(RE::StaticFunctionTag*, ForgeHandle handle, std::string scriptName) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj) obj->scriptName = scriptName;
        }

        static std::int32_t GetCurrentLine(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? static_cast<std::int32_t>(obj->currentLine) : -1;
        }

        static void SetCurrentLine(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t currentLine) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj && currentLine >= 0) obj->currentLine = static_cast<std::size_t>(currentLine);
        }

        static bool GetIsReady(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->isReady : false;
        }

        static void SetIsReady(RE::StaticFunctionTag*, ForgeHandle handle, bool isReady) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj) obj->isReady = isReady;
        }

        static std::string GetMostRecentResult(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->mostRecentResult : "";
        }

        static void SetMostRecentResult(RE::StaticFunctionTag*, ForgeHandle handle, std::string result) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj) obj->mostRecentResult = result;
        }

        static RE::Actor* GetIterActor(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->iterActor : nullptr;
        }

        static void SetIterActor(RE::StaticFunctionTag*, ForgeHandle handle, RE::Actor* actor) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj) {
                obj->iterActor = actor;
                obj->iterActorFormID = actor ? actor->GetFormID() : 0;
            }
        }

        // Variable management
        static std::string SetLocalVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name, std::string value) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->SetLocalVar(name, value) : "";
        }

        static std::string GetLocalVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->GetLocalVar(name) : "";
        }

        static bool HasLocalVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->HasLocalVar(name) : false;
        }

        // Script token management
        static std::int32_t GetScriptTokenCount(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? static_cast<std::int32_t>(obj->scriptTokens.size()) : 0;
        }

        static ForgeHandle GetScriptToken(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t index) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj && index >= 0 && index < static_cast<std::int32_t>(obj->scriptTokens.size())) {
                return obj->scriptTokens[index];
            }
            return 0;
        }

        // Call arguments
        static std::vector<std::string> GetCallArgs(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            return obj ? obj->callArgs : std::vector<std::string>{};
        }

        static void SetCallArgs(RE::StaticFunctionTag*, ForgeHandle handle, std::vector<std::string> args) {
            auto* obj = ForgeManagerTemplate<FrameContext>::GetFromHandle(handle);
            if (obj) obj->callArgs = args;
        }

        static void Destroy(RE::StaticFunctionTag*, ForgeHandle handle) {
            ForgeManagerTemplate<FrameContext>::DestroyFromHandle(handle);
        }
    };

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        vm->RegisterFunction("CreateEmpty", SCRIPT_NAME, Helpers::CreateEmpty);
        vm->RegisterFunction("CreateNew", SCRIPT_NAME, Helpers::CreateNew);
        vm->RegisterFunction("GetScriptName", SCRIPT_NAME, Helpers::GetScriptName);
        vm->RegisterFunction("SetScriptName", SCRIPT_NAME, Helpers::SetScriptName);
        vm->RegisterFunction("GetCurrentLine", SCRIPT_NAME, Helpers::GetCurrentLine);
        vm->RegisterFunction("SetCurrentLine", SCRIPT_NAME, Helpers::SetCurrentLine);
        vm->RegisterFunction("GetIsReady", SCRIPT_NAME, Helpers::GetIsReady);
        vm->RegisterFunction("SetIsReady", SCRIPT_NAME, Helpers::SetIsReady);
        vm->RegisterFunction("GetMostRecentResult", SCRIPT_NAME, Helpers::GetMostRecentResult);
        vm->RegisterFunction("SetMostRecentResult", SCRIPT_NAME, Helpers::SetMostRecentResult);
        vm->RegisterFunction("GetIterActor", SCRIPT_NAME, Helpers::GetIterActor);
        vm->RegisterFunction("SetIterActor", SCRIPT_NAME, Helpers::SetIterActor);
        vm->RegisterFunction("SetLocalVar", SCRIPT_NAME, Helpers::SetLocalVar);
        vm->RegisterFunction("GetLocalVar", SCRIPT_NAME, Helpers::GetLocalVar);
        vm->RegisterFunction("HasLocalVar", SCRIPT_NAME, Helpers::HasLocalVar);
        vm->RegisterFunction("GetScriptTokenCount", SCRIPT_NAME, Helpers::GetScriptTokenCount);
        vm->RegisterFunction("GetScriptToken", SCRIPT_NAME, Helpers::GetScriptToken);
        vm->RegisterFunction("GetCallArgs", SCRIPT_NAME, Helpers::GetCallArgs);
        vm->RegisterFunction("SetCallArgs", SCRIPT_NAME, Helpers::SetCallArgs);
        vm->RegisterFunction("Destroy", SCRIPT_NAME, Helpers::Destroy);

        return true;
    }
};

using FrameContextManager = ForgeManagerTemplate<FrameContext>;

// Logging macros for FrameContext
#define FrameLogInfo(frame, fmt, ...) \
    do { \
        if (frame) { \
            std::string message = std::format(fmt, ##__VA_ARGS__); \
            (frame)->LogInfo(message); \
        } \
    } while(0)

#define FrameLogWarn(frame, fmt, ...) \
    do { \
        if (frame) { \
            std::string message = std::format(fmt, ##__VA_ARGS__); \
            (frame)->LogWarn(message); \
        } \
    } while(0)

#define FrameLogError(frame, fmt, ...) \
    do { \
        if (frame) { \
            std::string message = std::format(fmt, ##__VA_ARGS__); \
            (frame)->LogError(message); \
        } \
    } while(0)

#define FrameLogDebug(frame, fmt, ...) \
    do { \
        if (frame) { \
            std::string message = std::format(fmt, ##__VA_ARGS__); \
            (frame)->LogDebug(message); \
        } \
    } while(0)

} // namespace SLT