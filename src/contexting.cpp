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
        std::string_view lastSpellId;
        
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
    
    bool ApplyScript(RE::Actor* target, const std::string& scriptName);

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

bool ScriptPoolManager::ApplyScript(RE::Actor* target, const std::string& scriptName) {
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

    static bool WriteTokenizedScript(SKSE::SerializationInterface* a_intfc,
                                   const std::vector<std::vector<std::string>>& tokens) {
        std::size_t rowCount = tokens.size();
        if (!WriteData(a_intfc, rowCount)) return false;
        
        for (const auto& row : tokens) {
            if (!WriteStringVector(a_intfc, row)) return false;
        }
        return true;
    }

    static bool ReadTokenizedScript(SKSE::SerializationInterface* a_intfc,
                                  std::vector<std::vector<std::string>>& tokens) {
        std::size_t rowCount;
        if (!ReadData(a_intfc, rowCount)) return false;
        
        tokens.clear();
        tokens.reserve(rowCount);
        for (std::size_t i = 0; i < rowCount; ++i) {
            std::vector<std::string> row;
            if (!ReadStringVector(a_intfc, row)) return false;
            tokens.push_back(std::move(row));
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

ThreadContextHandle ContextManager::StartSLTScript(RE::Actor* target, const std::string& initialScriptName) {
    if (!target || initialScriptName.empty()) return 0;

    try {
        // Get ContextManager and create through existing system
        auto& contextManager = ContextManager::GetSingleton();

        TargetContext* targetContext = GetTargetContext(target);

        if (!targetContext) {
            targetContext = CreateTargetContext(target);
        }

        ThreadContext* context = ContextManager::GetSingleton().WithActiveContexts(
        [target, targetContext, &initialScriptName](auto& activeContexts){
            auto newPtr = std::make_unique<ThreadContext>(targetContext, initialScriptName);
            ThreadContext* rawPtr = newPtr.get();
            targetContext->threads.push_back(std::move(newPtr));

            activeContexts[rawPtr->threadContextHandle] = rawPtr;
            return rawPtr;
        });
        
        if (!context) {
            logger::error("Failed to create ThreadContext for script: {}", initialScriptName);
            return 0;
        }

        if (!ScriptPoolManager::GetSingleton().ApplyScript(target, initialScriptName)) {
            logger::error("Unable to apply script({}) to target ({})", initialScriptName, Util::String::ToHex(target->GetFormID()));
            return 0;
        }
        return context->threadContextHandle;
    } catch (const std::exception& e) {
        logger::error("Exception in StartSLTScript: {}", e.what());
    } catch (...) {
        logger::error("Unknown exception in StartSLTScript");
    }
    return 0;
}

bool ContextManager::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, nextContextId)) return false;
    
    // Write global variables
    {
        std::lock_guard<std::mutex> lock(globalVarsMutex);
        if (!SH::WriteStringMap(a_intfc, globalVars)) return false;
    }
    
    // Write target contexts
    {
        std::lock_guard<std::mutex> lock(targetContextsMutex);
        std::size_t contextCount = targetContexts.size();
        if (!SH::WriteData(a_intfc, contextCount)) return false;
        
        for (const auto& [formID, targetContext] : targetContexts) {
            if (!targetContext->Serialize(a_intfc)) return false;
        }
    }
    
    std::lock_guard<std::mutex> lock(activeContextsMutex);
    std::size_t contextCount = activeContexts.size();
    if (!SH::WriteData(a_intfc, contextCount)) return false;
    
    for (const auto& [contextId, threadContext] : activeContexts) {
        if (!SH::WriteData(a_intfc, contextId)) return false;
        
        RE::FormID targetFormID = threadContext->target->tesTargetFormID;
        if (!SH::WriteData(a_intfc, targetFormID)) return false;
    }
    
    return true;
}

bool ContextManager::Deserialize(SKSE::SerializationInterface* a_intfc) {
    using SH = SerializationHelper;

    if (!SH::ReadData(a_intfc, nextContextId)) return false;
    
    // Read global variables
    {
        std::lock_guard<std::mutex> lock(globalVarsMutex);
        if (!SH::ReadStringMap(a_intfc, globalVars)) return false;
    }
    
    // Read target contexts
    {
        std::lock_guard<std::mutex> lock(targetContextsMutex);
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
    }

    {
        std::ignore = WithActiveContexts([self=this](auto& activeContexts){
            std::ignore = self->WithTargetContexts([&activeContexts](auto& targetContexts){
                for (auto& [formId, targetContext] : targetContexts) {
                    for (auto& threadContext : targetContext->threads) {
                        activeContexts[threadContext->threadContextHandle] = threadContext.get();
                    }
                }
                return nullptr;
            });
            return nullptr;
        });
    }
    
    return true;
}
#pragma endregion

#pragma region TargetContext
bool TargetContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;
    
    // Write target FormID
    if (!SH::WriteData(a_intfc, tesTargetFormID)) return false;
    
    // Write threads
    std::lock_guard<std::mutex> lock(ContextManager::GetSingleton().activeContextsMutex);
    std::size_t threadCount = threads.size();
    if (!SH::WriteData(a_intfc, threadCount)) return false;
    
    for (const auto& thread : threads) {
        if (!thread->Serialize(a_intfc)) return false;
    }
    
    // Write target variables
    std::lock_guard<std::mutex> varLock(targetVarsMutex);
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
    
    std::lock_guard<std::mutex> lock(ContextManager::GetSingleton().activeContextsMutex);
    threads.clear();
    threads.reserve(threadCount);
    
    for (std::size_t i = 0; i < threadCount; ++i) {
        auto thread = std::make_unique<ThreadContext>(this, "");
        if (!thread->Deserialize(a_intfc, this)) return false;
        threads.push_back(std::move(thread));
    }
    
    // Read target variables
    std::size_t varCount;
    if (!SH::ReadData(a_intfc, threadCount)) return false;
    
    std::lock_guard<std::mutex> varLock(targetVarsMutex);
    if (!SH::ReadStringMap(a_intfc, targetVars)) return false;
    
    return true;
}

#pragma endregion

#pragma region ThreadContext

bool ThreadContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, wasClaimed)) return false;

    if (!SH::WriteData(a_intfc, threadContextHandle)) return false;
    if (!SH::WriteData(a_intfc, initialScriptName)) return false;
    
    // Write call stack
    std::lock_guard<std::mutex> lock(callStackMutex);
    std::size_t stackSize = callStack.size();
    if (!SH::WriteData(a_intfc, stackSize)) return false;
    
    for (const auto& frame : callStack) {
        if (!frame->Serialize(a_intfc)) return false;
    }
    
    // Write thread variables
    std::lock_guard<std::mutex> varLock(threadVarsMutex);
    if (!SH::WriteStringMap(a_intfc, threadVars)) return false;
    
    return true;
}

bool ThreadContext::Deserialize(SKSE::SerializationInterface* a_intfc, TargetContext* targetContext) {
    using SH = SerializationHelper;

    isClaimed = false;
    if (!SH::ReadData(a_intfc, wasClaimed)) return false;

    if (!SH::ReadData(a_intfc, threadContextHandle)) return false;
    if (!SH::ReadData(a_intfc, initialScriptName)) return false;
    
    // Read call stack
    std::size_t stackSize;
    if (!SH::ReadData(a_intfc, stackSize)) return false;
    
    std::lock_guard<std::mutex> lock(callStackMutex);
    callStack.clear();
    callStack.reserve(stackSize);
    
    for (std::size_t i = 0; i < stackSize; ++i) {
        auto frame = std::make_unique<FrameContext>(this, "");
        if (!frame->Deserialize(a_intfc)) return false;
        callStack.push_back(std::move(frame));
    }
    
    // Restore target pointer
    target = targetContext;
    
    // Read thread variables
    std::lock_guard<std::mutex> varLock(threadVarsMutex);
    if (!SH::ReadStringMap(a_intfc, threadVars)) return false;
    
    return true;
}

// one ping only vasily
bool ThreadContext::ExecuteNextStep() {
    return (WithCallStack([](auto& callStack) -> FrameContext* {
        if (callStack.empty())
            return nullptr;

        auto* currentFrame = callStack.back().get();
        if (currentFrame && currentFrame->HasStep())
            return currentFrame->RunStep() ? currentFrame : nullptr;
        
        return nullptr;
    }) != nullptr);
}
#pragma endregion

#pragma region FrameContext

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
    if (!SH::WriteTokenizedScript(a_intfc, scriptTokens)) return false;
    
    // Write call arguments
    if (!SH::WriteStringVector(a_intfc, callArgs)) return false;
    
    // Write label maps
    if (!SH::WriteSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::WriteSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Write return stack
    if (!SH::WriteSizeTVector(a_intfc, returnStack)) return false;
    
    // Write local variables (thread-safe)
    std::lock_guard<std::mutex> lock(localVarsMutex);
    if (!SH::WriteStringMap(a_intfc, localVars)) return false;
    
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
    
    // Read tokenized script
    if (!SH::ReadTokenizedScript(a_intfc, scriptTokens)) return false;
    
    // Read call arguments
    if (!SH::ReadStringVector(a_intfc, callArgs)) return false;
    
    // Read label maps
    if (!SH::ReadSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::ReadSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Read return stack
    if (!SH::ReadSizeTVector(a_intfc, returnStack)) return false;
    
    // Read local variables (thread-safe)
    std::lock_guard<std::mutex> lock(localVarsMutex);
    if (!SH::ReadStringMap(a_intfc, localVars)) return false;
    
    return true;
}

bool FrameContext::ParseScript() {
    return Salty::GetSingleton().ParseScript(this);
}


bool FrameContext::HasStep() {
    if (currentLine >= scriptTokens.size()) {
        return false;
    }
    
    if (!Salty::GetSingleton().IsStepRunnable(scriptTokens[currentLine])) {
        Salty::GetSingleton().AdvanceToNextRunnableStep(this);
    }

    return currentLine < scriptTokens.size();
}

// returns false when the FrameContext has nothing else to run
// i.e. end of script or final return
bool FrameContext::RunStep() {
/*
    currentLine initializes to 0, so assume correct state coming in
    so we should always advance it before exiting
    and if it is equal to or greater than scriptTokens.size() we return false

    comments below are general but the point is this will likely pass the current

    otherwise we try to run the line of code, skipping any that are unparseable or unrunnable
    need to capture errors as well to output
    perhaps make those functions/events/etc.

        line[0], line[1]...
        perform same basic logic as currently in Papyrus... i.e. check line[0] for the operation
        many things can be handled directly
        others have to be handed off

*/
    if (!HasStep()) {
        return false;
    }
    // we really do not care what happens, at least not yet...
    // SLTEngineRunStep(step) is intended to run as forgivingly as possible, much like Papyrus script
    bool success = Salty::GetSingleton().RunStep(scriptTokens[currentLine]);
    currentLine++;
    Salty::GetSingleton().AdvanceToNextRunnableStep(this);
    return HasStep();
}
#pragma endregion
