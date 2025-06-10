#pragma once

namespace SLT {

class TargetContext;

#pragma region ThreadContext
/**
 * ThreadContext
 * One per script request sent to SLT. Tracks the stack of FrameContexts for this thread.
 * A ThreadContext is not associated with an actual C++ thread, but rather tracking the path of execution for a singular request on a target.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by GlobalContext
 */
class ThreadContext : public ForgeObject {
private:

    mutable bool targetFetchAttempted;
    mutable TargetContext* _target;

    mutable std::shared_mutex varlock;
    
public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersThreadContext";

    ThreadContext() = default;

    explicit ThreadContext(ForgeHandle _targetContextHandle, std::string_view _initialScriptName)
        : targetContextHandle(_targetContextHandle), initialScriptName(_initialScriptName), isClaimed(false), wasClaimed(false) {
        PushFrameContext(_initialScriptName);
    }

    // Transient, not serialized
    RE::ActiveEffect* ame;
    bool isClaimed; // do not serialize/deserialize, resets on game reload

    // Core data members, serialized
    ForgeHandle targetContextHandle;
    std::string initialScriptName;

    std::vector<ForgeHandle> callStack;
    std::map<std::string, std::string> threadVars;
    bool wasClaimed; // serialize/deserialize

    TargetContext* ptarget() const;

    std::string SetThreadVar(std::string_view name, std::string_view value);
    std::string GetThreadVar(std::string_view name) const;
    bool HasThreadVar(std::string_view name) const;

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc); 

    FrameContext* PushFrameContext(std::string_view initialScriptName);
    bool PopFrameContext();
    FrameContext* GetCurrentFrame() const;

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        /*
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
        */

        return true;
    }

};


using ThreadContextManager = ForgeManagerTemplate<ThreadContext>;

#pragma endregion
}