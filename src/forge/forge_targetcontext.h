#pragma once

namespace SLT {

class GlobalContext;

#pragma region TargetContext
/**
 * TargetContext
 * One per target (Actor). Tracks all of the threads (e.g. execution contexts, i.e. how many times did someone request a script be run on this Actor).
 * Provides host for target-specific variables and state.
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

    struct Helpers {
        // Factory functions
        static ForgeHandle CreateEmpty(RE::StaticFunctionTag*) {
            auto obj = std::make_unique<TargetContext>();
            return ForgeManagerTemplate<TargetContext>::Create(std::move(obj));
        }

        static ForgeHandle CreateNew(RE::StaticFunctionTag*, RE::TESForm* target) {
            try {
                auto obj = std::make_unique<TargetContext>(target);
                return ForgeManagerTemplate<TargetContext>::Create(std::move(obj));
            } catch (...) {
                return 0; // Invalid handle on failure
            }
        }

        static ForgeHandle CreateForActor(RE::StaticFunctionTag*, RE::Actor* actor) {
            try {
                auto obj = std::make_unique<TargetContext>(actor);
                return ForgeManagerTemplate<TargetContext>::Create(std::move(obj));
            } catch (...) {
                return 0; // Invalid handle on failure
            }
        }

        // Basic accessors
        static RE::TESForm* GetTarget(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->tesTarget : nullptr;
        }

        static void SetTarget(RE::StaticFunctionTag*, ForgeHandle handle, RE::TESForm* target) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj) {
                obj->tesTarget = target;
                obj->tesTargetFormID = target ? target->GetFormID() : 0;
            }
        }

        static RE::Actor* GetActor(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->AsActor() : nullptr;
        }

        static std::uint32_t GetTargetFormID(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->tesTargetFormID : 0;
        }

        // Target variable management
        static std::string SetTargetVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name, std::string value) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->SetTargetVar(name, value) : "";
        }

        static std::string GetTargetVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->GetTargetVar(name) : "";
        }

        static bool HasTargetVar(RE::StaticFunctionTag*, ForgeHandle handle, std::string name) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->HasTargetVar(name) : false;
        }

        // Thread management
        static std::int32_t GetThreadCount(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? static_cast<std::int32_t>(obj->threads.size()) : 0;
        }

        static ForgeHandle GetThread(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t index) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj && index >= 0 && index < static_cast<std::int32_t>(obj->threads.size())) {
                return obj->threads[index];
            }
            return 0;
        }

        static void AddThread(RE::StaticFunctionTag*, ForgeHandle handle, ForgeHandle threadHandle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj && threadHandle != 0) {
                obj->threads.push_back(threadHandle);
            }
        }

        static bool RemoveThread(RE::StaticFunctionTag*, ForgeHandle handle, ForgeHandle threadHandle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj) {
                auto it = std::find(obj->threads.begin(), obj->threads.end(), threadHandle);
                if (it != obj->threads.end()) {
                    obj->threads.erase(it);
                    return true;
                }
            }
            return false;
        }

        static bool RemoveThreadAt(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t index) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj && index >= 0 && index < static_cast<std::int32_t>(obj->threads.size())) {
                obj->threads.erase(obj->threads.begin() + index);
                return true;
            }
            return false;
        }

        static void ClearThreads(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj) {
                obj->threads.clear();
            }
        }

        static std::vector<ForgeHandle> GetAllThreads(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            return obj ? obj->threads : std::vector<ForgeHandle>{};
        }

        static void SetAllThreads(RE::StaticFunctionTag*, ForgeHandle handle, std::vector<ForgeHandle> threads) {
            auto* obj = ForgeManagerTemplate<TargetContext>::GetFromHandle(handle);
            if (obj) {
                obj->threads = threads;
            }
        }

        static void Destroy(RE::StaticFunctionTag*, ForgeHandle handle) {
            ForgeManagerTemplate<TargetContext>::DestroyFromHandle(handle);
        }
    };

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        vm->RegisterFunction("CreateEmpty", SCRIPT_NAME, Helpers::CreateEmpty);
        vm->RegisterFunction("CreateNew", SCRIPT_NAME, Helpers::CreateNew);
        vm->RegisterFunction("CreateForActor", SCRIPT_NAME, Helpers::CreateForActor);
        vm->RegisterFunction("GetTarget", SCRIPT_NAME, Helpers::GetTarget);
        vm->RegisterFunction("SetTarget", SCRIPT_NAME, Helpers::SetTarget);
        vm->RegisterFunction("GetActor", SCRIPT_NAME, Helpers::GetActor);
        vm->RegisterFunction("GetTargetFormID", SCRIPT_NAME, Helpers::GetTargetFormID);
        vm->RegisterFunction("SetTargetVar", SCRIPT_NAME, Helpers::SetTargetVar);
        vm->RegisterFunction("GetTargetVar", SCRIPT_NAME, Helpers::GetTargetVar);
        vm->RegisterFunction("HasTargetVar", SCRIPT_NAME, Helpers::HasTargetVar);
        vm->RegisterFunction("GetThreadCount", SCRIPT_NAME, Helpers::GetThreadCount);
        vm->RegisterFunction("GetThread", SCRIPT_NAME, Helpers::GetThread);
        vm->RegisterFunction("AddThread", SCRIPT_NAME, Helpers::AddThread);
        vm->RegisterFunction("RemoveThread", SCRIPT_NAME, Helpers::RemoveThread);
        vm->RegisterFunction("RemoveThreadAt", SCRIPT_NAME, Helpers::RemoveThreadAt);
        vm->RegisterFunction("ClearThreads", SCRIPT_NAME, Helpers::ClearThreads);
        vm->RegisterFunction("GetAllThreads", SCRIPT_NAME, Helpers::GetAllThreads);
        vm->RegisterFunction("SetAllThreads", SCRIPT_NAME, Helpers::SetAllThreads);
        vm->RegisterFunction("Destroy", SCRIPT_NAME, Helpers::Destroy);

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