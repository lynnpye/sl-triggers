#pragma once


namespace SLT {
class GlobalContext;
class TargetContext;
class ThreadContext;
class FrameContext;
class CommandLine;

#pragma region ScriptPoolManager

class ScriptPoolManager {
public:
    static ScriptPoolManager& GetSingleton() {
        static ScriptPoolManager singleton;
        return singleton;
    }

    void InitializePool() {
        spellPool.clear();
        mgefPool.clear();

        int highWaterMark = 0;
        std::string lastSpellId;
        
        for (int i = 1; i <= 99; ++i) {
            std::string spellId = "slt_cmds" + std::to_string(i).insert(0, 2 - std::to_string(i).length(), '0');
            if (auto spell = RE::TESForm::LookupByEditorID<RE::SpellItem>(spellId)) {
                spellPool.push_back(spell);
                highWaterMark = i;
                lastSpellId = spellId;
            } else {
                break;
            }
        }
        
        for (int i = 1; i <= highWaterMark; ++i) {
            std::string mgefId = "slt_cmd" + std::to_string(i).insert(0, 2 - std::to_string(i).length(), '0');
            if (auto mgef = RE::TESForm::LookupByEditorID<RE::EffectSetting>(mgefId)) {
                mgefPool.push_back(mgef);
            } else {
                highWaterMark = i;
                logger::warn("Failed to find magic effect: [{}]({})\n\
                    Fewer MGEF records than SPEL records. Using reduced high water mark {} and reducing SPEL pool to match", i, mgefId, highWaterMark);
                spellPool.resize(highWaterMark);
                break;
            }
        }
        
        logger::info("Script pool initialized: {} spells, {} magic effects", 
                    spellPool.size(), mgefPool.size());
    }

    RE::EffectSetting* FindAvailableMGEF(RE::Actor* target) {
        if (!target) return nullptr;
        
        auto magicTarget = target->AsMagicTarget();
        if (!magicTarget) return nullptr;
        
        for (auto mgef : mgefPool) {
            if (!magicTarget->HasMagicEffect(mgef)) {
                return mgef;
            }
        }
        
        logger::warn("No available magic effects in pool for target {}", 
                    target->GetDisplayFullName());
        return nullptr;
    }
    
    RE::SpellItem* FindSpellForMGEF(RE::EffectSetting* mgef) {
        if (!mgef) return nullptr;
        
        auto it = std::find(mgefPool.begin(), mgefPool.end(), mgef);
        if (it != mgefPool.end()) {
            size_t index = std::distance(mgefPool.begin(), it);
            if (index < spellPool.size()) {
                return spellPool[index];
            }
        }
        
        return nullptr;
    }
    
    bool ApplyScript(RE::Actor* target, std::string_view scriptName);

private:
    std::vector<RE::SpellItem*> spellPool;
    std::vector<RE::EffectSetting*> mgefPool;
    
    ScriptPoolManager() = default;
    ScriptPoolManager(const ScriptPoolManager&) = delete;
    ScriptPoolManager& operator=(const ScriptPoolManager&) = delete;
};
#pragma endregion

#pragma region GlobalContext
/**
 * GlobalContext
 * Tracks all targets with active contexts. Provides host for global variables and state.
 * 

 * THREAD SAFETY: Uses single coordination lock for all operations
 * - ReadData() operations: std::shared_lock on coordinationLock
 * - WriteData() operations: std::unique_lock on coordinationLock
 * - Individual context objects have no internal mutexes
 */
class GlobalContext {

private:
    // Single coordination lock for all data operations
    static std::shared_mutex maplock;
    static std::shared_mutex varlock;
    
    // Data members (protected by coordinationLock)
    static std::unordered_map<RE::FormID, ForgeHandle> formIdTargetMap;
    static std::map<std::string, std::string> globalVars;

public:
    GlobalContext() = default;
    static GlobalContext& GetSingleton() {
        static GlobalContext singleton;
        return singleton;
    }

    static std::string SetGlobalVar(std::string_view name, std::string_view value);
    static std::string GetGlobalVar(std::string_view name);
    static bool HasGlobalVar(std::string_view name);

    static std::string SetVar(FrameContext* frame, std::string_view name, std::string_view value);
    static std::string GetVar(FrameContext* frame, std::string_view name);
    static bool HasVar(FrameContext* frame, std::string_view name);

    static std::string ResolveValueVariable(ForgeHandle threadContextHandle, std::string_view token);
    static bool ResolveFormVariable(ForgeHandle threadContextHandle, std::string_view token);

    static bool StartSLTScript(RE::Actor* target, std::string_view initialScriptName);
    
    static void CleanupContext(ForgeHandle contextId);
    
    static bool Register(RE::BSScript::IVirtualMachine* vm);
    static void OnSave(SKSE::SerializationInterface* intfc);
    static void OnLoad(SKSE::SerializationInterface* intfc);
    static void OnRevert(SKSE::SerializationInterface* intfc);

private:
    GlobalContext(const GlobalContext&) = delete;
    GlobalContext& operator=(const GlobalContext&) = delete;
    GlobalContext(GlobalContext&&) = delete;
    GlobalContext& operator=(GlobalContext&&) = delete;
};
#pragma endregion



}