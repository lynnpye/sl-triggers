#pragma once

namespace SLT {
class ContextManager;
class FrameContext;
class TargetContext;
class ThreadContext;


/**
 * ContextManager
 * Tracks all targets with active contexts. Provides host for global variables and state.
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

    // Map of formID to target contexts (one per target actor)
    mutable std::mutex targetContextsMutex;
    std::unordered_map<RE::FormID, std::unique_ptr<TargetContext>> targetContexts;
    
    mutable std::mutex activeContextsMutex;
    std::unordered_map<ThreadContextHandle, ThreadContext*> activeContexts;

    mutable std::mutex globalVarsMutex;
    std::map<std::string, std::string> globalVars;

    template <typename Func>
    TargetContext* WithTargetContexts(Func&& fn) {
        std::lock_guard<std::mutex> lock(targetContextsMutex);
        return fn(targetContexts);
    }

    template <typename Func>
    ThreadContext* WithActiveContexts(Func&& fn) {
        std::lock_guard<std::mutex> lock(activeContextsMutex);
        return fn(activeContexts);
    }

    template <typename Func>
    std::string WithGlobalVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(globalVarsMutex);
        return fn(globalVars);
    }

    ContextManager() = default;
    static ContextManager& GetSingleton() {
        static ContextManager singleton;
        return singleton;
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;

    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    TargetContext* CreateTargetContext(RE::TESForm* target) {
        if (target == nullptr)
            return nullptr;
            
        return WithTargetContexts([&tgt = target](auto& contexts) {
            if (!tgt) {
                tgt = RE::PlayerCharacter::GetSingleton();
            }
            RE::FormID targetId = tgt->GetFormID();

            TargetContext* theTargetContext;
            auto tcit = contexts.find(targetId);
            if (tcit == contexts.end()) {
                auto [newIt, inserted] = contexts.emplace(targetId, std::make_unique<TargetContext>(tgt));
                return newIt->second.get();
            } else {
                return tcit->second.get();
            }
        });
    }

    TargetContext* GetTargetContext(RE::TESForm* target) {
        if (target == nullptr)
            return nullptr;
            
        logger::info("WithTargetContexts get target context for target form");
        logger::info("formID :{}:", target->GetFormID());
        return WithTargetContexts([&tgt = target](auto& contexts) -> TargetContext* {
            if (!tgt) {
                tgt = RE::PlayerCharacter::GetSingleton();
            }
            RE::FormID targetId = tgt->GetFormID();

            TargetContext* theTargetContext;
            auto tcit = contexts.find(targetId);
            if (tcit != contexts.end()) {
                return tcit->second.get();
            } else {
                return nullptr;
            }
        });
    }

    std::string SetGlobalVar(std::string name, std::string value) {
        return WithGlobalVars([&nm = name, &val = value](auto& vars){
            vars[nm] = val;
            return val;
        });
    }

    std::string GetGlobalVar(std::string name) {
        return WithGlobalVars([&nm = name](auto& vars){
            if (vars.contains(nm)) {
                return vars[nm];
            } else {
                return std::string("");
            }
        });
    }

    ThreadContextHandle StartSLTScript(RE::Actor* target, const std::string& scriptName);

    ThreadContext* GetContext(ThreadContextHandle contextId) {
        std::lock_guard<std::mutex> lock(activeContextsMutex);
        auto it = activeContexts.find(contextId);
        return (it != activeContexts.end()) ? it->second : nullptr;
    }
    
    void CleanupContext(ThreadContextHandle contextId) {
        {
            std::lock_guard<std::mutex> lock(activeContextsMutex);
            activeContexts.erase(contextId);
        }
        
        logger::info("Cleaned up SLT context ID: {}", contextId);
    }

    void CleanupAllContexts() {
        {
            std::lock_guard<std::mutex> lock(activeContextsMutex);
            activeContexts.clear();
        }
        
        logger::info("Cleaned up all SLT contexts");
    }
    
    std::size_t GetActiveContextCount() const {
        std::lock_guard<std::mutex> lock(activeContextsMutex);
        return activeContexts.size();
    }

private:
    ContextManager(const ContextManager&) = delete;
    ContextManager& operator=(const ContextManager&) = delete;
    ContextManager(ContextManager&&) = delete;
    ContextManager& operator=(ContextManager&&) = delete;
};

/**
 * TargetContext
 * One per target (Actor). Tracks all of the threads (e.g. execution contexts, i.e. how many times did someone request a script be run on this Actor).
 * Provides host for target-specific variables and state.
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

    //mutable std::mutex threadsMutex;
    std::vector<std::unique_ptr<ThreadContext>> threads;

    mutable std::mutex targetVarsMutex;
    std::map<std::string, std::string> targetVars;

    // template <typename Func>
    // ThreadContext* xWithThreads(Func&& fn) {
    //     std::lock_guard<std::mutex> lock(threadsMutex);
    //     return fn(threads);
    // }

    template <typename Func>
    std::string WithTargetVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(targetVarsMutex);
        return fn(targetVars);
    }
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    std::string SetTargetVar(std::string name, std::string value) {
        return WithTargetVars([&nm = name, &val = value](auto& vars){
            vars[nm] = val;
            return val;
        });
    }

    std::string GetTargetVar(std::string name) {
        return WithTargetVars([&nm = name](auto& vars){
            if (vars.contains(nm)) {
                return vars[nm];
            } else {
                return std::string("");
            }
        });
    }

    void RemoveThreadContext(ThreadContext* threadContextToRemove) {
        if (threadContextToRemove == nullptr)
            return;
        
        std::ignore = ContextManager::GetSingleton().WithActiveContexts([self=this, threadContextToRemove](auto& activeContexts){
            auto it = std::find_if(self->threads.begin(), self->threads.end(), 
                [threadContextToRemove](const std::unique_ptr<ThreadContext>& uptr) {
                    return uptr.get() == threadContextToRemove;
                });

            if (it != self->threads.end()) {
                ThreadContextHandle handle = it->get()->threadContextHandle;
                auto activeIt = std::find_if(activeContexts.begin(), activeContexts.end(),
                    [handle](const auto& pair) {
                        return pair.first == handle;
                    });
                if (activeIt != activeContexts.end()) {
                    activeContexts.erase(activeIt);
                }
                self->threads.erase(it);
            }
            return nullptr;
        });
    }
};

/**
 * ThreadContext
 * One per script request sent to SLT. Tracks the stack of FrameContexts for this thread.
 * A ThreadContext is not associated with an actual C++ thread, but rather tracking the path of execution for a singular request on a target.
 */
class ThreadContext {
public:
    explicit ThreadContext(TargetContext* target, std::string _initialScriptName)
        : target(target), initialScriptName(_initialScriptName), isClaimed(false), wasClaimed(false) {
        threadContextHandle = ContextManager::GetNextContextId();
        PushFrameContext(_initialScriptName);
    }

    TargetContext* target;

    ThreadContextHandle threadContextHandle;
    std::string initialScriptName;

    mutable std::mutex callStackMutex;
    std::vector<std::unique_ptr<FrameContext>> callStack;

    mutable std::mutex threadVarsMutex;
    std::map<std::string, std::string> threadVars;

    // theoretically, if an executor wakes up and wants to run something
    // we could let it look for any job that isn't claimed and has never been
    // then, if it couldn't find any, it could be allowed to run a job that
    // is not claimed but had been during a previous save
    bool isClaimed; // do not serialize/deserialize, resets on game reload
    bool wasClaimed; // serialize/deserialize

    bool IsTargetValid() {
        return target && target->tesTarget;
    }

    template <typename Func>
    FrameContext* WithCallStack(Func&& fn) {
        std::lock_guard<std::mutex> lock(callStackMutex);
        return fn(callStack);
    }

    template <typename Func>
    std::string WithThreadVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(threadVarsMutex);
        return fn(threadVars);
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;

    bool Deserialize(SKSE::SerializationInterface* a_intfc, TargetContext* targetContext); 

    std::string SetThreadVar(std::string name, std::string value) {
        return WithThreadVars([&nm = name, &val = value](auto& vars){
            vars[nm] = val;
            return val;
        });
    }

    std::string GetThreadVar(std::string name) {
        return WithThreadVars([&nm = name](auto& vars){
            if (vars.contains(nm)) {
                return vars[nm];
            } else {
                return std::string("");
            }
        });
    }

    FrameContext* PushFrameContext(std::string initialScriptName) {
        if (initialScriptName.empty())
            return nullptr;
        
        return WithCallStack([papa = this, initialScriptName](auto& callStack){
            auto newPtr = std::make_unique<FrameContext>(papa, initialScriptName);
            FrameContext* newContext = newPtr.get();
            callStack.push_back(std::move(newPtr));
            return newContext;
        });
    }

    bool ExecuteNextStep();
};

/**
 * FrameContext
 * Tracks local variables and state for the currently executing SLT script.
 */

enum ScriptType {
    INI,
    JSON,


    END
};

class FrameContext {
public:
    explicit FrameContext(ThreadContext* thread, std::string scriptName) : thread(thread), scriptName(scriptName), currentLine(0), iterActor(nullptr), lastKey(0), iterActorFormID(0) {}

    ThreadContext* thread;
    std::string scriptName;

    std::vector<std::vector<std::string>> scriptTokens;
    std::size_t currentLine;
    // "ini" or "json" or similar
    // this should be an enum
    ScriptType commandType;
    // arguments sent to this script when it was invoked via "call"; empty if none provided or the root script
    std::vector<std::string> callArgs;
    std::map<std::string, std::size_t> gotoLabelMap;
    std::map<std::string, std::size_t> gosubLabelMap;
    std::vector<std::size_t> returnStack;
    std::string mostRecentResult;
    std::uint32_t lastKey;
    // Serialize as formID
    RE::FormID iterActorFormID;
    RE::Actor* iterActor;

    mutable std::mutex localVarsMutex;
    std::map<std::string, std::string> localVars;

    template <typename Func>
    std::string WithLocalVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(localVarsMutex);
        return fn(localVars);
    }
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    std::string SetLocalVar(std::string name, std::string value) {
        return WithLocalVars([&nm = name, &val = value](auto& vars){
            vars[nm] = val;
            return val;
        });
    }

    std::string GetLocalVar(std::string name) {
        return WithLocalVars([&nm = name](auto& vars){
            if (vars.contains(nm)) {
                return vars[nm];
            } else {
                return std::string("");
            }
        });
    }

    // The presumption is that the file has been parsed and tokenized into scriptTokens before either of these functions is called
    bool ParseScript();

    // returns true if RunStep() can do something (i.e. this framecontext has an op ready to run); false if nothing else to do
    bool HasStep();

    // returns false when the FrameContext has nothing else to run
    bool RunStep();
};

}