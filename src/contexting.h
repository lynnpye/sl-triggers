#pragma once

namespace SLT {
class ContextManager;
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

#pragma region CommandLine
/**
 * CommandLine - container for an execution-relevant line of SLTScript; may contain additional contextual information useful for output and debugging for the end-user
 * std::size_t lineNumber : line number in the original user SLTScript (i.e. inclusive of empty lines, a command may be on line 5 but be the first command, e.g. lineNumber would be 5)
 * std::vector<std::string> tokens : raw, unresolved string tokens (parsed but not necessarily lexed)
 */
class CommandLine {
public:

    CommandLine() = default;

    explicit CommandLine(std::size_t _lineNumber, std::vector<std::string> _tokens)
        : lineNumber(_lineNumber), tokens(_tokens) {}

    std::size_t lineNumber;
    std::vector<std::string> tokens;
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);
};
#pragma endregion

#pragma region ContextManager
/**
 * ContextManager
 * Tracks all targets with active contexts. Provides host for global variables and state.
 * 

 * THREAD SAFETY: Uses single coordination lock for all operations
 * - ReadData() operations: std::shared_lock on coordinationLock
 * - WriteData() operations: std::unique_lock on coordinationLock
 * - Individual context objects have no internal mutexes
 */
class ContextManager {

public:
    
    static ThreadContextHandle nextContextId;
    
    static ThreadContextHandle GetNextContextId() {
        if (nextContextId == std::numeric_limits<ThreadContextHandle>::max() || nextContextId < 1) {
            nextContextId = 1;
        }
        return nextContextId++;
    }

private:
    // Single coordination lock for all data operations
    mutable std::shared_mutex coordinationLock;
    
    // Data members (protected by coordinationLock)
    std::unordered_map<RE::FormID, std::unique_ptr<TargetContext>> targetContexts;
    std::unordered_map<ThreadContextHandle, std::shared_ptr<ThreadContext>> activeContexts;
    std::map<std::string, std::string> globalVars;

public:
    ContextManager() = default;
    static ContextManager& GetSingleton() {
        static ContextManager singleton;
        return singleton;
    }

    // READ OPERATIONS - Use shared lock, explicit naming
    template <typename Func>
    auto ReadData(Func&& fn) const {
        try {
            std::shared_lock<std::shared_mutex> lock(coordinationLock);
            return fn(targetContexts, activeContexts, globalVars, nextContextId);
        } catch (...) {
            throw;
        }
    }
    
    // WRITE OPERATIONS - Use exclusive lock, explicit naming
    template <typename Func>
    auto WriteData(Func&& fn) {
        try {
            std::unique_lock<std::shared_mutex> lock(coordinationLock);
            return fn(targetContexts, activeContexts, globalVars, nextContextId);
        } catch (...) {
            throw;
        }
    }

    // Convenience methods using the functors
    TargetContext* CreateTargetContext(RE::TESForm* target) {
        if (!target) target = RE::PlayerCharacter::GetSingleton();
        
        return WriteData([target](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) -> TargetContext* {
            RE::FormID targetId = target->GetFormID();
            
            auto it = targetContexts.find(targetId);
            if (it == targetContexts.end()) {
                auto [newIt, inserted] = targetContexts.emplace(targetId, std::make_unique<TargetContext>(target));
                return newIt->second.get();
            } else {
                return it->second.get();
            }
        });
    }

    FrameContext* GetFrameContext(ThreadContextHandle handle) const {
        if (!handle) return nullptr;

        return ReadData([handle](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> FrameContext* {
            FrameContext* frame = nullptr;
            if (auto search = activeContexts.find(handle); search != activeContexts.end()) {
                if (search != activeContexts.end() && search->second && search->second->callStack.size() > 0) {
                    if (auto fc = search->second->callStack.back().get())
                        return fc;
                    return nullptr;
                } else {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        });
    }

    std::string SetGlobalVar(std::string_view name, std::string_view value);
    std::string GetGlobalVar(std::string_view name) const;
    bool HasGlobalVar(std::string_view name) const;

    std::string SetVar(FrameContext* frame, std::string_view name, std::string_view value);
    std::string GetVar(FrameContext* frame, std::string_view name) const;
    bool HasVar(FrameContext* frame, std::string_view name) const;

    std::string ResolveValueVariable(ThreadContextHandle threadContextHandle, std::string_view token) const;
    bool ResolveFormVariable(ThreadContextHandle threadContextHandle, std::string_view token) const;

    bool StartSLTScript(RE::Actor* target, std::string_view initialScriptName);

    std::shared_ptr<ThreadContext> GetContext(ThreadContextHandle contextId) const {
        return ReadData([contextId](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> std::shared_ptr<ThreadContext> {
            auto it = activeContexts.find(contextId);
            return (it != activeContexts.end()) ? it->second : nullptr;
        });
    }
    
    void CleanupContext(ThreadContextHandle contextId);

    void CleanupAllContexts() {
        WriteData([](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
            activeContexts.clear();
            targetContexts.clear();
            return nullptr;
        });
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

private:
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    ContextManager(ContextManager&&) = delete;
    ContextManager& operator=(ContextManager&&) = delete;
};
#pragma endregion

#pragma region TargetContext
/**
 * TargetContext
 * One per target (Actor). Tracks all of the threads (e.g. execution contexts, i.e. how many times did someone request a script be run on this Actor).
 * Provides host for target-specific variables and state.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by ContextManager
 */
class TargetContext {
public:
    explicit TargetContext(RE::TESForm* target) {
        if (!target) {
            target = RE::PlayerCharacter::GetSingleton();
        }
        tesTarget = target;
        tesTargetFormID = target ? target->GetFormID() : 0;
    }

    // Store formID for serialization
    RE::FormID tesTargetFormID;
    RE::TESForm* tesTarget;

    std::vector<std::shared_ptr<ThreadContext>> threads;
    std::map<std::string, std::string> targetVars;

    RE::Actor* AsActor() const {
        return tesTarget ? tesTarget->As<RE::Actor>() : nullptr;
    }
    
    std::string SetTargetVar(std::string_view name, std::string_view value) {
        if (name.empty()) {
            logger::warn("SetTargetVar: Variable name cannot be empty");
            return "";
        }
        
        return ContextManager::GetSingleton().WriteData([this, name = std::string(name), value = std::string(value)](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
            auto result = this->targetVars.emplace(name, value);
            if (!result.second) {
                result.first->second = value; // Update existing
            }
            return result.first->second;
        });
    }

    std::string GetTargetVar(std::string_view name) const {
        if (name.empty()) {
            return "";
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
            auto it = this->targetVars.find(name);
            return (it != this->targetVars.end()) ? it->second : std::string{};
        });
    }

    bool HasTargetVar(std::string_view name) const {
        if (name.empty()) {
            return false;
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
            auto it = this->targetVars.find(name);
            return (it != this->targetVars.end()) ? true : false;
        });
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);
};
#pragma endregion

#pragma region ThreadContext
/**
 * ThreadContext
 * One per script request sent to SLT. Tracks the stack of FrameContexts for this thread.
 * A ThreadContext is not associated with an actual C++ thread, but rather tracking the path of execution for a singular request on a target.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by ContextManager
 */
class ThreadContext {
public:
    explicit ThreadContext(TargetContext* target, std::string_view _initialScriptName, ThreadContextHandle _threadContextHandle)
        : target(target), initialScriptName(_initialScriptName), threadContextHandle(_threadContextHandle), isClaimed(false), wasClaimed(false) {
        PushFrameContext(_initialScriptName);
    }

    TargetContext* target;
    RE::ActiveEffect* ame;

    ThreadContextHandle threadContextHandle;
    std::string initialScriptName;

    std::vector<std::unique_ptr<FrameContext>> callStack;
    std::map<std::string, std::string> threadVars;

    // Transient execution state (not serialized)
    bool isClaimed; // do not serialize/deserialize, resets on game reload
    bool wasClaimed; // serialize/deserialize

    std::string SetThreadVar(std::string_view name, std::string_view value) {
        if (name.empty()) {
            logger::warn("SetThreadVar: Variable name cannot be empty");
            return "";
        }
        
        return ContextManager::GetSingleton().WriteData([this, name = std::string(name), value = std::string(value)](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
            auto result = this->threadVars.emplace(name, value);
            if (!result.second) {
                result.first->second = value; // Update existing
            }
            return result.first->second;
        });
    }

    std::string GetThreadVar(std::string_view name) const {
        if (name.empty()) {
            return "";
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
            auto it = this->threadVars.find(name);
            return (it != this->threadVars.end()) ? it->second : std::string{};
        });
    }

    bool HasThreadVar(std::string_view name) const {
        if (name.empty()) {
            return false;
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> bool {
            auto it = this->threadVars.find(name);
            return (it != this->threadVars.end()) ? true : false;
        });
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc, TargetContext* targetContext); 

    FrameContext* PushFrameContext(std::string_view initialScriptName);
    bool PopFrameContext();
};
#pragma endregion

#pragma region FrameContext
/**
 * FrameContext
 * Tracks local variables and state for the currently executing SLT script.
 * 
 * THREAD SAFETY: No internal mutexes - all coordination handled by ContextManager
 */
class FrameContext {
public:
    FrameContext() = default;

    explicit FrameContext(ThreadContext* thread, std::string_view scriptName)
        : thread(thread), scriptName(scriptName), currentLine(0), iterActor(nullptr),
            lastKey(0), iterActorFormID(0), isReadied(false), isReady(false) {
        if (!ParseScript()) {
            std::string errorMsg = "Unable to parse script '" + std::string(scriptName) + "'";
            throw std::logic_error(errorMsg);
        }
    }

    bool ParseScript();

    ThreadContext* thread;
    std::string scriptName;

    // transient
    RE::TESForm* customResolveFormResult;
    bool popAfterStepReturn;

    bool isReady;
    bool isReadied;
    std::vector<std::unique_ptr<CommandLine>> scriptTokens;
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

    std::size_t GetCurrentActualLineNumber() {
        logger::debug("scriptTokens.size({}) currentLine({})", scriptTokens.size(), currentLine);

        if (currentLine < scriptTokens.size()) {
            auto& cmdLine = scriptTokens.at(currentLine);
            return cmdLine->lineNumber;
        }

        return -1;
    }
    
#define FrameLogInfo(frame, fmt, ...) \
    do { \
        std::string message = std::format(fmt, ##__VA_ARGS__); \
        std::string prefixed = std::format("[{}:{}] - {}", (frame)->scriptName, (frame)->GetCurrentActualLineNumber(), message); \
        logger::info("{}", prefixed); \
    } while(0)

#define FrameLogWarn(frame, fmt, ...) \
    do { \
        std::string message = std::format(fmt, ##__VA_ARGS__); \
        std::string prefixed = std::format("[{}:{}] - {}", (frame)->scriptName, (frame)->GetCurrentActualLineNumber(), message); \
        logger::warn("{}", prefixed); \
    } while(0)

#define FrameLogError(frame, fmt, ...) \
    do { \
        std::string message = std::format(fmt, ##__VA_ARGS__); \
        std::string prefixed = std::format("[{}:{}] - {}", (frame)->scriptName, (frame)->GetCurrentActualLineNumber(), message); \
        logger::error("{}", prefixed); \
    } while(0)

#define FrameLogDebug(frame, fmt, ...) \
    do { \
        std::string message = std::format(fmt, ##__VA_ARGS__); \
        std::string prefixed = std::format("[{}:{}] - {}", (frame)->scriptName, (frame)->GetCurrentActualLineNumber(), message); \
        logger::debug("{}", prefixed); \
    } while(0)
    
    std::string SetLocalVar(std::string_view name, std::string_view value) {
        if (name.empty()) {
            FrameLogWarn(this, "SetLocalVar: Variable name cannot be empty");
            return "";
        }
        
        return ContextManager::GetSingleton().WriteData([this, name = std::string(name), value = std::string(value)](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
            auto result = this->localVars.emplace(name, value);
            if (!result.second) {
                result.first->second = value; // Update existing
            }
            return result.first->second;
        });
    }

    std::string GetLocalVar(std::string_view name) const {
        if (name.empty()) {
            return "";
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
            auto it = this->localVars.find(name);
            return (it != this->localVars.end()) ? it->second : std::string{};
        });
    }

    bool HasLocalVar(std::string_view name) const {
        if (name.empty()) {
            return false;
        }
        
        return ContextManager::GetSingleton().ReadData([this, name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> bool {
            auto it = this->localVars.find(name);
            return (it != this->localVars.end()) ? true : false;
        });
    }
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);
};
#pragma endregion
}