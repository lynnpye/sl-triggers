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

    mutable std::atomic<bool> targetFetchAttempted;
    mutable TargetContext* _target;

    mutable std::shared_mutex varlock;
    
    std::vector<ForgeHandle> callStack;
    
public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersThreadContext";

    ThreadContext() = default;

    explicit ThreadContext(ForgeHandle _targetContextHandle, std::string_view _initialScriptName)
        : targetContextHandle(_targetContextHandle), initialScriptName(_initialScriptName), isClaimed(false), wasClaimed(false) {
        PushFrameContext(_initialScriptName);
    }

    bool Initialize() {
        return PushFrameContext(initialScriptName) != nullptr;
    }

    // Transient, not serialized
    RE::ActiveEffect* ame;
    bool isClaimed; // do not serialize/deserialize, resets on game reload

    // Core data members, serialized
    ForgeHandle targetContextHandle;
    std::string initialScriptName;

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

    struct Helpers {
        // Factory functions
        static ForgeHandle CreateEmpty(RE::StaticFunctionTag*) {
            auto obj = std::make_unique<ThreadContext>();
            return ForgeManagerTemplate<ThreadContext>::Create(std::move(obj));
        }

        static ForgeHandle CreateNew(RE::StaticFunctionTag*, ForgeHandle targetContextHandle, std::string initialScriptName) {
            try {
                auto obj = std::make_unique<ThreadContext>(targetContextHandle, initialScriptName);
                return ForgeManagerTemplate<ThreadContext>::Create(std::move(obj));
            } catch (...) {
                return 0; // Invalid handle on failure
            }
        }

        // Basic accessors
        static ForgeHandle GetTargetContextHandle(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->targetContextHandle : 0;
        }

        static void SetTargetContextHandle(RE::StaticFunctionTag*, ForgeHandle handle, ForgeHandle targetContextHandle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) obj->targetContextHandle = targetContextHandle;
        }

        static std::string GetInitialScriptName(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->initialScriptName : "";
        }

        static void SetInitialScriptName(RE::StaticFunctionTag*, ForgeHandle handle, std::string scriptName) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) obj->initialScriptName = scriptName;
        }

        static bool GetIsClaimed(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->isClaimed : false;
        }

        static void SetIsClaimed(RE::StaticFunctionTag*, ForgeHandle handle, bool claimed) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) obj->isClaimed = claimed;
        }

        static bool GetWasClaimed(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->wasClaimed : false;
        }

        static void SetWasClaimed(RE::StaticFunctionTag*, ForgeHandle handle, bool wasClaimed) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) obj->wasClaimed = wasClaimed;
        }

        static RE::ActiveEffect* GetActiveEffect(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->ame : nullptr;
        }

        static void SetActiveEffect(RE::StaticFunctionTag*, ForgeHandle handle, RE::ActiveEffect* ame) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) obj->ame = ame;
        }

        // Thread variable management
        static std::string SetThreadVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name, std::string value) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->SetThreadVar(name, value) : "";
        }

        static std::string GetThreadVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->GetThreadVar(name) : "";
        }

        static bool HasThreadVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->HasThreadVar(name) : false;
        }

        // Frame context management
        static ForgeHandle PushFrameContext(RE::StaticFunctionTag*, ForgeHandle handle, std::string scriptName) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) {
                auto* frame = obj->PushFrameContext(scriptName);
                return frame ? frame->GetHandle() : 0;
            }
            return 0;
        }

        static bool PopFrameContext(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? obj->PopFrameContext() : false;
        }

        static ForgeHandle GetCurrentFrame(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj) {
                auto* frame = obj->GetCurrentFrame();
                return frame ? frame->GetHandle() : 0;
            }
            return 0;
        }

        // Call stack management
        static std::int32_t GetCallStackSize(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            return obj ? static_cast<std::int32_t>(obj->callStack.size()) : 0;
        }

        static ForgeHandle GetCallStackFrame(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t index) {
            auto* obj = ForgeManagerTemplate<ThreadContext>::GetFromHandle(handle);
            if (obj && index >= 0 && index < static_cast<std::int32_t>(obj->callStack.size())) {
                return obj->callStack[index];
            }
            return 0;
        }

        static void Destroy(RE::StaticFunctionTag*, ForgeHandle handle) {
            ForgeManagerTemplate<ThreadContext>::DestroyFromHandle(handle);
        }
    };

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        vm->RegisterFunction("CreateEmpty", SCRIPT_NAME, Helpers::CreateEmpty);
        vm->RegisterFunction("CreateNew", SCRIPT_NAME, Helpers::CreateNew);
        vm->RegisterFunction("GetTargetContextHandle", SCRIPT_NAME, Helpers::GetTargetContextHandle);
        vm->RegisterFunction("SetTargetContextHandle", SCRIPT_NAME, Helpers::SetTargetContextHandle);
        vm->RegisterFunction("GetInitialScriptName", SCRIPT_NAME, Helpers::GetInitialScriptName);
        vm->RegisterFunction("SetInitialScriptName", SCRIPT_NAME, Helpers::SetInitialScriptName);
        vm->RegisterFunction("GetIsClaimed", SCRIPT_NAME, Helpers::GetIsClaimed);
        vm->RegisterFunction("SetIsClaimed", SCRIPT_NAME, Helpers::SetIsClaimed);
        vm->RegisterFunction("GetWasClaimed", SCRIPT_NAME, Helpers::GetWasClaimed);
        vm->RegisterFunction("SetWasClaimed", SCRIPT_NAME, Helpers::SetWasClaimed);
        vm->RegisterFunction("GetActiveEffect", SCRIPT_NAME, Helpers::GetActiveEffect);
        vm->RegisterFunction("SetActiveEffect", SCRIPT_NAME, Helpers::SetActiveEffect);
        vm->RegisterFunction("SetThreadVar", SCRIPT_NAME, Helpers::SetThreadVar);
        vm->RegisterFunction("GetThreadVar", SCRIPT_NAME, Helpers::GetThreadVar);
        vm->RegisterFunction("HasThreadVar", SCRIPT_NAME, Helpers::HasThreadVar);
        vm->RegisterFunction("PushFrameContext", SCRIPT_NAME, Helpers::PushFrameContext);
        vm->RegisterFunction("PopFrameContext", SCRIPT_NAME, Helpers::PopFrameContext);
        vm->RegisterFunction("GetCurrentFrame", SCRIPT_NAME, Helpers::GetCurrentFrame);
        vm->RegisterFunction("GetCallStackSize", SCRIPT_NAME, Helpers::GetCallStackSize);
        vm->RegisterFunction("GetCallStackFrame", SCRIPT_NAME, Helpers::GetCallStackFrame);
        vm->RegisterFunction("Destroy", SCRIPT_NAME, Helpers::Destroy);

        return true;
    }

};


using ThreadContextManager = ForgeManagerTemplate<ThreadContext>;

#pragma endregion
}