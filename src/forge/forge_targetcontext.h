#pragma once

namespace SLT {

class GlobalContext;

#pragma region TargetContext
/**
 * TargetContext
 * One per target (Actor). Tracks all of the threads (e.g. execution contexts, i.e. how many times did someone request a script be run on this Actor).
 * Provides host for target-specific variables and state.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by GlobalContext
 */
class TargetContext : public ForgeObject {
private:

    mutable std::shared_mutex varlock;

public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersTargetContext";

    TargetContext() = default;

    explicit TargetContext(RE::TESForm* target) {
        if (!target) {
            target = RE::PlayerCharacter::GetSingleton();
        }
        tesTarget = target;
        tesTargetFormID = target ? target->GetFormID() : 0;
    }

    // Transient, not serialized
    RE::TESForm* tesTarget;

    // Store formID for serialization
    RE::FormID tesTargetFormID;

    std::vector<ForgeHandle> threads;
    std::map<std::string, std::string> targetVars;

    RE::Actor* AsActor() const {
        return tesTarget ? tesTarget->As<RE::Actor>() : nullptr;
    }
    
    std::string SetTargetVar(std::string_view name, std::string_view value);
    std::string GetTargetVar(std::string_view name) const;
    bool HasTargetVar(std::string_view name) const;

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

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
    

    // Convenience methods using the functors
    static TargetContext* CreateTargetContext(RE::TESForm* target) {
        if (!target) target = RE::PlayerCharacter::GetSingleton();

        if (!target) return nullptr;

        auto handle = ForgeManagerTemplate<TargetContext>::Create(std::move(std::make_unique<TargetContext>(target)));

        return ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
    }
};


using TargetContextManager = ForgeManagerTemplate<TargetContext>;

#pragma endregion
}