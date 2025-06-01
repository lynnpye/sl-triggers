#include "contexting.h"
#include "engine.h"

using namespace SLT;

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
                logger::warn("SPEL high water mark [{}]({})", highWaterMark, lastSpellId);
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

OnDataLoaded([]{
    ScriptPoolManager::GetSingleton().InitializePool();
});

bool ScriptPoolManager::ApplyScript(RE::Actor* target, std::string_view scriptName) {
    if (!target) {
        logger::error("Invalid caster or target for script application");
        return false;
    }
    
    try {
        // Find an available MGEF
        auto availableMGEF = FindAvailableMGEF(target);
        if (!availableMGEF) {
            logger::error("No available magic effects to apply script: {}", scriptName);
            return false;
        }
        
        // Find the corresponding spell
        auto spell = FindSpellForMGEF(availableMGEF);
        if (!spell) {
            logger::error("Could not find spell for MGEF when applying script: {}", scriptName);
            return false;
        }
        
        // Create or get the target context and set up the script
        auto& contextManager = ContextManager::GetSingleton();
        auto targetContext = contextManager.CreateTargetContext(target);
        if (!targetContext) {
            logger::error("Failed to create target context for script: {}", scriptName);
            return false;
        }
        
        // Cast the spell
        auto* magicCaster = target->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (magicCaster) {
            magicCaster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, target);
            return true;
        } else {
            logger::error("Failed to get magic caster for script application");
        }
    } catch (...) {
        logger::error("Unknown/unexpected exception in ApplyScript");
    }

    return false;
}
#pragma endregion

#pragma region SerializationHelper

template<typename T>
bool WriteData(SKSE::SerializationInterface* a_intfc, const T& data) {
    return a_intfc->WriteRecordData(&data, sizeof(T));
}

template<typename T>
bool ReadData(SKSE::SerializationInterface* a_intfc, T& data) {
    return a_intfc->ReadRecordData(&data, sizeof(T));
}

class SerializationHelper {
public:
    template<typename T>
    static bool WriteData(SKSE::SerializationInterface* a_intfc, const T& data) {
        return a_intfc->WriteRecordData(&data, sizeof(T));
    }

    template<typename T>
    static bool ReadData(SKSE::SerializationInterface* a_intfc, T& data) {
        return a_intfc->ReadRecordData(&data, sizeof(T));
    }

    static bool WriteString(SKSE::SerializationInterface* a_intfc, const std::string& str) {
        std::size_t length = str.length();
        if (!WriteData(a_intfc, length)) return false;
        if (length > 0) {
            return a_intfc->WriteRecordData(str.c_str(), length);
        }
        return true;
    }

    static bool ReadString(SKSE::SerializationInterface* a_intfc, std::string& str) {
        std::size_t length;
        if (!ReadData(a_intfc, length)) return false;
        
        if (length > 0) {
            str.resize(length);
            return a_intfc->ReadRecordData(str.data(), length);
        } else {
            str.clear();
            return true;
        }
    }

    static bool WriteStringMap(SKSE::SerializationInterface* a_intfc, 
                              const std::map<std::string, std::string>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteString(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadStringMap(SKSE::SerializationInterface* a_intfc, 
                             std::map<std::string, std::string>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key, value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadString(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteSizeTMap(SKSE::SerializationInterface* a_intfc,
                              const std::map<std::string, std::size_t>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadSizeTMap(SKSE::SerializationInterface* a_intfc,
                             std::map<std::string, std::size_t>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key;
            std::size_t value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadData(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteSizeTVector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::size_t>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& value : vec) {
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadSizeTVector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::size_t>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::size_t value;
            if (!ReadData(a_intfc, value)) return false;
            vec.push_back(value);
        }
        return true;
    }

    static bool WriteUint32Map(SKSE::SerializationInterface* a_intfc,
                              const std::map<std::string, std::uint32_t>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadUint32Map(SKSE::SerializationInterface* a_intfc,
                             std::map<std::string, std::uint32_t>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key;
            std::uint32_t value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadData(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteUint32Vector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::uint32_t>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& value : vec) {
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadUint32Vector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::uint32_t>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::uint32_t value;
            if (!ReadData(a_intfc, value)) return false;
            vec.push_back(value);
        }
        return true;
    }
    
    static bool WriteStringVector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::string>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& str : vec) {
            if (!WriteString(a_intfc, str)) return false;
        }
        return true;
    }

    static bool ReadStringVector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::string>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::string str;
            if (!ReadString(a_intfc, str)) return false;
            vec.push_back(std::move(str));
        }
        return true;
    }
};
#pragma endregion

#pragma region ContextManager
static ContextManager* g_ContextManager = nullptr;

ThreadContextHandle ContextManager::nextContextId = 1;

OnAfterSKSEInit([]{
    g_ContextManager = &ContextManager::GetSingleton();
});


ThreadContextHandle ContextManager::StartSLTScript(RE::Actor* target, std::string_view initialScriptName) {
    if (!target || initialScriptName.empty()) return 0;

    ThreadContextHandle handle = 0;
    try {
        // Use WriteData to modify contexts
        handle = WriteData([target, initialScriptName](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) -> ThreadContextHandle {
            // Get or create target context
            RE::FormID targetId = target->GetFormID();
            TargetContext* targetContext = nullptr;
            
            auto tcit = targetContexts.find(targetId);
            if (tcit == targetContexts.end()) {
                auto [newIt, inserted] = targetContexts.emplace(targetId, std::make_unique<TargetContext>(target));
                targetContext = newIt->second.get();
            } else {
                targetContext = tcit->second.get();
            }

            // Create new thread context
            auto newPtr = std::make_unique<ThreadContext>(targetContext, initialScriptName);
            ThreadContext* rawPtr = newPtr.get();
            ThreadContextHandle handle = rawPtr->threadContextHandle;
            
            targetContext->threads.push_back(std::move(newPtr));
            activeContexts[handle] = rawPtr;
            
            return handle;
        });

        if (handle) {
            if (!ScriptPoolManager::GetSingleton().ApplyScript(target, initialScriptName)) {
                logger::error("Unable to apply script({}) to target ({})", initialScriptName, Util::String::ToHex(target->GetFormID()));
                return 0;
            }
        }
    } catch (const std::exception& e) {
        logger::error("Exception in StartSLTScript: {}", e.what());
    } catch (...) {
        logger::error("Unknown exception in StartSLTScript");
    }
    
    return 0;
}

bool ContextManager::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;

    // Create a consistent snapshot using ReadData
    return ReadData([a_intfc](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
        using SH = SerializationHelper;
        
        if (!SH::WriteData(a_intfc, nextId)) return false;
        
        // Write global variables
        if (!SH::WriteStringMap(a_intfc, globalVars)) return false;
        
        // Write target contexts
        std::size_t contextCount = targetContexts.size();
        if (!SH::WriteData(a_intfc, contextCount)) return false;
        
        for (const auto& [formID, targetContext] : targetContexts) {
            if (!targetContext->Serialize(a_intfc)) return false;
        }
        
        // Write active context mappings
        std::size_t activeCount = activeContexts.size();
        if (!SH::WriteData(a_intfc, activeCount)) return false;
        
        for (const auto& [contextId, threadContext] : activeContexts) {
            if (!SH::WriteData(a_intfc, contextId)) return false;
            
            RE::FormID targetFormID = threadContext->target->tesTargetFormID;
            if (!SH::WriteData(a_intfc, targetFormID)) return false;
        }
        
        return true;
    });
}

bool ContextManager::Deserialize(SKSE::SerializationInterface* a_intfc) {
    using SH = SerializationHelper;

    return WriteData([a_intfc](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
        using SH = SerializationHelper;
        
        if (!SH::ReadData(a_intfc, nextId)) return false;
        
        // Read global variables
        if (!SH::ReadStringMap(a_intfc, globalVars)) return false;
        
        // Read target contexts
        targetContexts.clear();
        
        std::size_t contextCount;
        if (!SH::ReadData(a_intfc, contextCount)) return false;
        
        for (std::size_t i = 0; i < contextCount; ++i) {
            auto targetContext = std::make_unique<TargetContext>(nullptr);
            if (!targetContext->Deserialize(a_intfc)) return false;
            
            // Only add if the target form still exists
            if (targetContext->tesTarget) {
                targetContexts[targetContext->tesTargetFormID] = std::move(targetContext);
            }
        }

        // Rebuild active contexts map
        activeContexts.clear();
        for (auto& [formId, targetContext] : targetContexts) {
            for (auto& threadContext : targetContext->threads) {
                activeContexts[threadContext->threadContextHandle] = threadContext.get();
            }
        }
        
        return true;
    });
}

// Global Variable Accessors (keep existing validation)
std::string ContextManager::SetGlobalVar(std::string_view name, std::string_view value) {
    if (name.empty()) {
        logger::warn("SetGlobalVar: Variable name cannot be empty");
        return "";
    }
    
    return WriteData([name = std::string(name), value = std::string(value)](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
        auto result = globalVars.emplace(name, value);
        if (!result.second) {
            result.first->second = value; // Update existing
        }
        return result.first->second;
    });
}

std::string ContextManager::GetGlobalVar(std::string_view name) const {
    if (name.empty()) {
        return "";
    }
    
    return ReadData([name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
        auto it = globalVars.find(name);
        return (it != globalVars.end()) ? it->second : std::string{};
    });
}

#pragma endregion

#pragma region TargetContext
bool TargetContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;
    
    // Write target FormID
    if (!SH::WriteData(a_intfc, tesTargetFormID)) return false;
    
    // Write threads
    std::size_t threadCount = threads.size();
    if (!SH::WriteData(a_intfc, threadCount)) return false;
    
    for (const auto& thread : threads) {
        if (!thread->Serialize(a_intfc)) return false;
    }
    
    // Write target variables
    if (!SH::WriteStringMap(a_intfc, targetVars)) return false;
    
    return true;
}

bool TargetContext::Deserialize(SKSE::SerializationInterface* a_intfc) {
    using SH = SerializationHelper;
    
    // Read target FormID
    std::int32_t savedFormID;
    if (!SH::ReadData(a_intfc, savedFormID)) return false;
    
    // Resolve FormID (handles form ID changes between saves)
    if (!a_intfc->ResolveFormID(savedFormID, tesTargetFormID)) {
        // Form no longer exists - this context is invalid
        return false;
    }
    
    // Look up the actual form
    tesTarget = RE::TESForm::LookupByID(tesTargetFormID);
    if (!tesTarget) {
        return false;
    }
    
    // Read threads
    std::size_t threadCount;
    if (!SH::ReadData(a_intfc, threadCount)) return false;
    
    threads.clear();
    threads.reserve(threadCount);
    
    for (std::size_t i = 0; i < threadCount; ++i) {
        auto thread = std::make_unique<ThreadContext>(this, "");
        if (!thread->Deserialize(a_intfc, this)) return false;
        threads.push_back(std::move(thread));
    }
    
    // Read target variables
    if (!SH::ReadStringMap(a_intfc, targetVars)) return false;
    
    return true;
}

void TargetContext::RemoveThreadContext(ThreadContext* threadContextToRemove) {
    if (threadContextToRemove == nullptr)
        return;
    
    // Use ContextManager to safely modify both collections
    ContextManager::GetSingleton().WriteData([this, threadContextToRemove](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
        auto it = std::find_if(threads.begin(), threads.end(), 
            [threadContextToRemove](const std::unique_ptr<ThreadContext>& uptr) {
                return uptr.get() == threadContextToRemove;
            });

        if (it != threads.end()) {
            ThreadContextHandle handle = it->get()->threadContextHandle;
            auto activeIt = activeContexts.find(handle);
            if (activeIt != activeContexts.end()) {
                activeContexts.erase(activeIt);
            }
            threads.erase(it);
        }
        return nullptr;
    });
}

#pragma endregion

#pragma region ThreadContext

FrameContext* ThreadContext::PushFrameContext(std::string_view initialScriptName) {
    if (initialScriptName.empty())
        return nullptr;
    
    try {
        callStack.push_back(std::make_unique<FrameContext>(this, initialScriptName));
        return callStack.back().get();
    } catch (...) {
        return nullptr;
    }
}

bool ThreadContext::PopFrameContext() {
    if (callStack.size() < 1)
        return false;

    try {
        if (callStack.size() > 0)
            callStack.pop_back();

        if (callStack.size() < 1)
            return false;
        
        auto* previousFrame = callStack.back().get();
        while (previousFrame && !previousFrame->IsReady()) {
            callStack.pop_back();
            if (callStack.size() < 1)
                return false;
            previousFrame = callStack.back().get();
        }
        
        return previousFrame && previousFrame->IsReady();
    } catch (...) {
        return false;
    }
}

bool ThreadContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, wasClaimed)) return false;
    if (!SH::WriteData(a_intfc, threadContextHandle)) return false;
    if (!SH::WriteString(a_intfc, initialScriptName)) return false;
    
    // Write call stack
    std::size_t stackSize = callStack.size();
    if (!SH::WriteData(a_intfc, stackSize)) return false;
    
    for (const auto& frame : callStack) {
        if (!frame->Serialize(a_intfc)) return false;
    }
    
    // Write thread variables
    if (!SH::WriteStringMap(a_intfc, threadVars)) return false;
    
    return true;
}

bool ThreadContext::Deserialize(SKSE::SerializationInterface* a_intfc, TargetContext* targetContext) {
    using SH = SerializationHelper;

    isClaimed = false;
    if (!SH::ReadData(a_intfc, wasClaimed)) return false;
    if (!SH::ReadData(a_intfc, threadContextHandle)) return false;
    if (!SH::ReadString(a_intfc, initialScriptName)) return false;
    
    // Read call stack
    std::size_t stackSize;
    if (!SH::ReadData(a_intfc, stackSize)) return false;
    
    callStack.clear();
    callStack.reserve(stackSize);
    
    for (std::size_t i = 0; i < stackSize; ++i) {
        callStack.push_back(std::make_unique<FrameContext>());
        auto frame = callStack.back().get();
        if (!frame->Deserialize(a_intfc)) return false;
        frame->thread = this; // Restore thread pointer
    }
    
    // Restore target pointer
    target = targetContext;
    
    // Read thread variables
    if (!SH::ReadStringMap(a_intfc, threadVars)) return false;
    
    return true;
}

bool ThreadContext::ExecuteNextStep(SLTStackAnalyzer::AMEContextInfo& contextInfo) {
    if (callStack.empty())
        return false;

    FrameContext* currentFrame = callStack.back().get();
    while (currentFrame && !currentFrame->IsReady()) {
        callStack.pop_back();
        if (callStack.size() > 0)
            currentFrame = callStack.back().get();
        else
            currentFrame = nullptr;
    }
    
    if (currentFrame && currentFrame->IsReady()) {
        return currentFrame->RunStep(contextInfo);
    }
    
    return false;
}
#pragma endregion

#pragma region FrameContext
bool CommandLine::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, lineNumber)) return false;

    std::size_t tokenCount = tokens.size();
    if (!SH::WriteData(a_intfc, tokenCount)) return false;

    for (auto& str : tokens) {
        if (!SH::WriteString(a_intfc, str)) return false;
    }

    return true;
}

bool CommandLine::Deserialize(SKSE::SerializationInterface* a_intfc) {
    using SH = SerializationHelper;

    if (!SH::ReadData(a_intfc, lineNumber)) return false;

    std::size_t tokenCount;
    if (!SH::ReadData(a_intfc, tokenCount)) return false;

    tokens.clear();
    tokens.reserve(tokenCount);
    for (std::size_t i = 0; i < tokenCount; ++i) {
        std::string str;
        if (!SH::ReadString(a_intfc, str)) return false;
        tokens.push_back(std::move(str));
    }

    return true;
}

bool FrameContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;
    
    // Write basic data
    if (!SH::WriteString(a_intfc, scriptName)) return false;
    if (!SH::WriteData(a_intfc, currentLine)) return false;
    if (!SH::WriteData(a_intfc, commandType)) return false;
    if (!SH::WriteString(a_intfc, mostRecentResult)) return false;
    if (!SH::WriteData(a_intfc, lastKey)) return false;
    if (!SH::WriteData(a_intfc, iterActorFormID)) return false;
    
    // Write tokenized script
    std::size_t scriptLength = scriptTokens.size();
    if (!SH::WriteData(a_intfc, scriptLength)) return false;

    for (const auto& cmdLine : scriptTokens) {
        if (!cmdLine->Serialize(a_intfc)) return false;
    }
    
    // Write call arguments
    if (!SH::WriteStringVector(a_intfc, callArgs)) return false;
    
    // Write label maps
    if (!SH::WriteSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::WriteSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Write return stack
    if (!SH::WriteSizeTVector(a_intfc, returnStack)) return false;
    
    // Write local variables
    if (!SH::WriteStringMap(a_intfc, localVars)) return false;

    if (!SH::WriteData(a_intfc, isReady)) return false;
    if (!SH::WriteData(a_intfc, isReadied)) return false;
    
    return true;
}

bool FrameContext::Deserialize(SKSE::SerializationInterface* a_intfc) {
    using SH = SerializationHelper;
    
    // Read basic data
    if (!SH::ReadString(a_intfc, scriptName)) return false;
    if (!SH::ReadData(a_intfc, currentLine)) return false;
    if (!SH::ReadData(a_intfc, commandType)) return false;
    if (!SH::ReadString(a_intfc, mostRecentResult)) return false;
    if (!SH::ReadData(a_intfc, lastKey)) return false;
    if (!SH::ReadData(a_intfc, iterActorFormID)) return false;
    
    // Resolve form ID for iterActor
    if (iterActorFormID != 0) {
        std::uint32_t resolvedFormID;
        if (a_intfc->ResolveFormID(iterActorFormID, resolvedFormID)) {
            iterActorFormID = resolvedFormID;
            iterActor = RE::TESForm::LookupByID<RE::Actor>(resolvedFormID);
        } else {
            // Form no longer exists
            iterActorFormID = 0;
            iterActor = nullptr;
        }
    } else {
        iterActor = nullptr;
    }
    
    std::size_t scriptLength;
    if (!SH::ReadData(a_intfc, scriptLength)) return false;

    scriptTokens.clear();
    scriptTokens.reserve(scriptLength);

    for (std::size_t i = 0; i < scriptLength; ++i) {
        scriptTokens.push_back(std::make_unique<CommandLine>());
        auto cmdLine = scriptTokens.back().get();
        if (!cmdLine->Deserialize(a_intfc)) return false;
    }
    
    // Read call arguments
    if (!SH::ReadStringVector(a_intfc, callArgs)) return false;
    
    // Read label maps
    if (!SH::ReadSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::ReadSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Read return stack
    if (!SH::ReadSizeTVector(a_intfc, returnStack)) return false;
    
    // Read local variables
    if (!SH::ReadStringMap(a_intfc, localVars)) return false;

    if (!SH::ReadData(a_intfc, isReady)) return false;
    if (!SH::ReadData(a_intfc, isReadied)) return false;
    
    return true;
}

bool FrameContext::ParseScript() {
    ParseResult result = Parser::ParseScript(this);
    if (result != ParseResult::Success) {
        LogError("Failed to parse script '{}': error code {}", scriptName, static_cast<int>(result));
        return false;
    }
    return IsReady();
}

bool FrameContext::IsReady() {
    if (isReadied) {
        return isReady;
    }

    return Salty::AdvanceToNextRunnableStep(this);
}

bool FrameContext::RunStep(SLTStackAnalyzer::AMEContextInfo& contextInfo) {
    auto& manager = ContextManager::GetSingleton();
    const int MAX_BATCHED_STEPS = 1000;
    const int TIME_CHECK_INTERVAL = 50; // Check time every 50 steps
    int stepCount = 0;
    
    // Check if execution is paused
    if (!manager.ShouldContinueExecution()) {
        LogInfo("Execution paused, yielding control");
        return IsReady(); // Don't execute, but preserve ready state
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    const auto maxFrameTime = std::chrono::microseconds(500); // 0.5ms budget
    
    while (stepCount < MAX_BATCHED_STEPS) {
        // Check for pause every 10 steps
        if (stepCount % 10 == 0 && !manager.ShouldContinueExecution()) {
            LogInfo("Execution paused mid-batch after {} steps", stepCount);
            break;
        }
        
        bool wasInternal = Salty::RunStep(this, contextInfo);
        stepCount++;
        
        if (!wasInternal) break;
        if (!IsReady()) break;
        
        // Only check time periodically to reduce overhead
        if (stepCount % TIME_CHECK_INTERVAL == 0) {
            auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
            if (elapsed > maxFrameTime) {
                LogDebug("Frame time budget exceeded after {} steps", stepCount);
                break;
            }
        }
    }
    
    if (stepCount >= MAX_BATCHED_STEPS) {
        LogWarn("Hit maximum batched steps limit, yielding control");
    }
    
    return IsReady();
}
#pragma endregion


#pragma region SLTStackAnalyzer definition
namespace {
void DumpFrame(BSScript::StackFrame* frame) {
    logger::info("BEGIN DumpFrame");
    if (!frame) {
        logger::info("null");
    } else {
        std::string owningScriptName = "OWNING SCRIPT UNAVAILABLE";
        if (frame->owningObjectType)
            owningScriptName = frame->owningObjectType->GetName();
        std::string owningFunctionName = "OWNING FUNCTION UNAVAILABLE";
        if (frame->owningFunction)
            owningFunctionName = frame->owningFunction->GetName();
        BSScript::Variable& selfObj = frame->self;
        std::string selfObjStr = selfObj.GetType().TypeAsString();
        logger::info("instructionsValid({}) size({}) instructionPointer({}) owningScript({}) owningFunction({}) selfObjStr({})",
            frame->instructionsValid, frame->size, frame->instructionPointer, owningScriptName, owningFunctionName, selfObjStr
            );
    }
    logger::info("END DumpFrame");
    if (frame->previousFrame) {
        logger::info("      WAS PRECEDED BY");
        DumpFrame(frame->previousFrame);
    }
}

void DumpCodeTasklet(BSTSmartPointer<BSScript::Internal::CodeTasklet>& owningTasklet) {
    logger::info("BEGIN DumpCodeTasklet");
    if (!owningTasklet) {
        logger::info("null");
    } else {
        logger::info("resumeReason({})", owningTasklet->resumeReason.underlying());
    }
    logger::info("END DumpCodeTasklet");
    DumpFrame(owningTasklet->topFrame);
}

void DumpStack(RE::BSScript::Stack* stackPtr) {
    logger::info("\n\n\n\n!!!!!!!!!!!!!!!!!!!!!!");
    logger::info("BEGIN DumpStack");
    if (!stackPtr) {
        logger::info("null");
    } else {
        logger::info("stackID({}) state({}) freezeState({}) stackType({}) frames({})",
            stackPtr->stackID, stackPtr->state.underlying(), stackPtr->freezeState.underlying(), stackPtr->stackType.underlying(), stackPtr->frames
        );
    }
    logger::info("END DumpStack");
    DumpCodeTasklet(stackPtr->owningTasklet);
    logger::info("\n!!!!!!!!!!!!!!!!!!!!!!\n\n\n\n");
}
}

void SLTStackAnalyzer::Walk(RE::VMStackID stackId) {
    
    // Validate stackId first
    if (stackId == 0 || stackId == static_cast<RE::VMStackID>(-1)) {
        logger::warn("Invalid stackId: 0x{:X}", stackId);
        return;
    }
    
    using RE::BSScript::Internal::VirtualMachine;
    using RE::BSScript::Stack;
    using RE::BSScript::Internal::CodeTasklet;
    using RE::BSScript::StackFrame;
    
    auto* vm = VirtualMachine::GetSingleton();
    if (!vm) {
        logger::warn("Failed to get VM singleton");
        return;
    }
    
    RE::BSScript::Stack* stackPtr = nullptr;
    
    try {
        bool success = vm->GetStackByID(stackId, &stackPtr);
        
        if (!success) {
            logger::warn("GetStackByID returned false for ID: 0x{:X}", stackId);
            return;
        }
        
        if (!stackPtr) {
            logger::warn("GetStackByID succeeded but returned null pointer for ID: 0x{:X}", stackId);
            return;
        }
        DumpStack(stackPtr);
    } catch (const std::exception& e) {
        logger::error("Exception in GetStackByID: {}", e.what());
        return;
    } catch (...) {
        logger::error("Unknown exception in GetStackByID for stackId: 0x{:X}", stackId);
        return;
    }
}

SLTStackAnalyzer::AMEContextInfo SLTStackAnalyzer::GetAMEContextInfo(RE::VMStackID stackId) {
    SLTStackAnalyzer::AMEContextInfo result;
    
    // Validate stackId first
    if (stackId == 0 || stackId == static_cast<RE::VMStackID>(-1)) {
        logger::warn("Invalid stackId: 0x{:X}", stackId);
        return result;
    }
    
    using RE::BSScript::Internal::VirtualMachine;
    using RE::BSScript::Stack;
    using RE::BSScript::Internal::CodeTasklet;
    using RE::BSScript::StackFrame;
    
    auto* vm = VirtualMachine::GetSingleton();
    if (!vm) {
        logger::warn("Failed to get VM singleton");
        return result;
    }
    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) {
        logger::error("Unable to get handle policy");
        return result;
    }
    
    RE::BSScript::Stack* stackPtr = nullptr;
    
    try {
        bool success = vm->GetStackByID(stackId, &stackPtr);
        
        if (!success) {
            logger::warn("GetStackByID returned false for ID: 0x{:X}", stackId);
            return result;
        }
        
        if (!stackPtr) {
            logger::warn("GetStackByID succeeded but returned null pointer for ID: 0x{:X}", stackId);
            return result;
        }
    } catch (const std::exception& e) {
        logger::error("Exception in GetStackByID: {}", e.what());
        return result;
    } catch (...) {
        logger::error("Unknown exception in GetStackByID for stackId: 0x{:X}", stackId);
        return result;
    }
    
    if (!stackPtr->owningTasklet) {
        logger::warn("Stack has no owning tasklet for ID: 0x{:X}", stackId);
        return result;
    }
    
    auto taskletPtr = stackPtr->owningTasklet;
    
    if (!taskletPtr->topFrame) {
        logger::warn("Tasklet has no top frame for stack ID: 0x{:X}", stackId);
        return result;
    }
    
    auto* frame = taskletPtr->topFrame;
    
    RE::BSScript::Variable& self = frame->self;
    if (!self.IsObject()) {
        logger::warn("Frame self is not an object for stack ID: 0x{:X}", stackId);
        return result;
    }
    
    auto selfObject = self.GetObject();
    if (!selfObject) {
        logger::warn("Failed to get self object for stack ID: 0x{:X}", stackId);
        return result;
    }

    RE::VMHandle objHandle = selfObject->GetHandle();
    if (!handlePolicy->IsHandleObjectAvailable(objHandle)) {
        logger::error("HandlePolicy says handle object not available!");
        // maybe return result;
    }
    RE::ActiveEffect* ame = static_cast<RE::ActiveEffect*>(handlePolicy->GetObjectForHandle(RE::ActiveEffect::VMTYPEID, objHandle));
    if (!ame) {
        logger::error("Unable to obtain AME from selfObject with RE::VMHandle({})", objHandle);
        return result;
    }
    RE::Actor* ameActor = ame->GetCasterActor().get();
    if (!ameActor) {
        logger::error("Unable to obtain AME.Actor from selfObject");
        return result;
    }
    
    RE::BSFixedString propNameThreadContextHandle("threadContextHandle");
    RE::BSScript::Variable* propThreadContextHandle = selfObject->GetProperty(propNameThreadContextHandle);
    
    RE::BSFixedString propNameInitialScriptName("initialScriptName");
    RE::BSScript::Variable* propInitialScriptName = selfObject->GetProperty(propNameInitialScriptName);

    ThreadContextHandle cid = 0;
    std::string_view initialScriptName = "";

    if (propThreadContextHandle && propThreadContextHandle->IsInt()) {
        cid = propThreadContextHandle->GetSInt();
    }

    if (propInitialScriptName && propInitialScriptName->IsString()) {
        initialScriptName = propInitialScriptName->GetString();
    }

    if (cid == 0 || initialScriptName.empty()) {
        // need to see if we have a waiting ThreadContext
        auto& contextManager = ContextManager::GetSingleton();
        auto* ameContext = contextManager.GetTargetContext(ameActor);
        if (!ameContext) {
            logger::error("No available TargetContext for formID {}", ameActor->GetFormID());
            return result;
        }

        // Use ReadData to find unclaimed thread
        ThreadContext* threadContext = contextManager.ReadData([ameContext](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> ThreadContext* {
            auto it = std::find_if(ameContext->threads.begin(), ameContext->threads.end(),
                [](const std::unique_ptr<ThreadContext>& threadContext) {
                    return !threadContext->isClaimed && !threadContext->wasClaimed;
                });
            
            return (it != ameContext->threads.end()) ? it->get() : nullptr;
        });

        if (!threadContext) {
            logger::error("No ThreadContext found");
            return result;
        }
        
        // Use WriteData to claim the thread
        contextManager.WriteData([threadContext, propThreadContextHandle, propInitialScriptName](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
            threadContext->isClaimed = true;
            threadContext->wasClaimed = true;
            propThreadContextHandle->SetSInt(threadContext->threadContextHandle);
            propInitialScriptName->SetString(threadContext->initialScriptName);
            return nullptr;
        });

        cid = threadContext->threadContextHandle;
        initialScriptName = threadContext->initialScriptName;
    }

    result.contextId = cid;
    
    result.isValid = (result.contextId != 0);

    if (result.isValid) {
        result.ame = ame;
        result.target = ameActor;
        result.initialScriptName = initialScriptName;
    } else {
        result.ame = nullptr;
        result.target = nullptr;
        result.initialScriptName = "";
    }
    
    return result;
}
#pragma endregion