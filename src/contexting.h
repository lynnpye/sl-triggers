#pragma once

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

// Forward declarations for Skyrim classes
namespace RE {
    class TESForm;
    class Actor;
}

// Forward declarations for your own classes
class TargetContext;
class ThreadContext;
class FrameContext;

/**
 * ContextManager
 * Tracks all targets with active contexts. Provides host for global variables and state.
 */
class ContextManager {

public:

    // Map of formID to target contexts (one per target actor)
    std::mutex targetContextsMutex;
    std::unordered_map<std::uint32_t, std::unique_ptr<TargetContext>> targetContexts;

    std::mutex globalVarsMutex;
    std::map<std::string, std::string> globalVars;

    template <typename Func>
    TargetContext* WithTargetContexts(Func&& fn) {
        std::lock_guard<std::mutex> lock(targetContextsMutex);
        return fn(targetContexts);
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

    TargetContext* CreateTargetContext(RE::TESForm* target) {
        if (target == nullptr)
            return nullptr;
            
        return WithTargetContexts([&tgt = target](auto& contexts) {
            if (!tgt) {
                tgt = RE::PlayerCharacter::GetSingleton();
            }
            std::uint32_t targetId = tgt->GetFormID();

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
            
        return WithTargetContexts([&tgt = target](auto& contexts) -> TargetContext* {
            if (!tgt) {
                tgt = RE::PlayerCharacter::GetSingleton();
            }
            std::uint32_t targetId = tgt->GetFormID();

            TargetContext* theTargetContext;
            auto tcit = contexts.find(targetId);
            if (tcit != contexts.end()) {
                return tcit->second.get();
            } else {
                return nullptr;
            }
        });
    }

    ThreadContext* CreateThreadContext(RE::TESForm* target, std::string initialScriptName);

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

    void StartAllThreads() {
        WithTargetContexts([](auto& targets){
            for (auto& target : targets) {
                target.second.get()->WithThreads([](auto& threads){
                    for (auto& thread : threads) {
                        thread->StartWork();
                    }
                    return nullptr;
                });
                return nullptr;
            }
        });
    }

    void StopAllThreads() {
        WithTargetContexts([](auto& targets){
            for (auto& target : targets) {
                target.second.get()->WithThreads([](auto& threads){
                    for (auto& thread : threads) {
                        thread->StartWork();
                    }
                    return nullptr;
                });
                return nullptr;
            }
        });
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
    std::mutex tesTargetMutex;
    std::uint32_t tesTargetFormID;
    RE::TESForm* tesTarget;

    std::mutex threadsMutex;
    std::vector<std::unique_ptr<ThreadContext>> threads;

    std::mutex targetVarsMutex;
    std::map<std::string, std::string> targetVars;

    template <typename Func>
    std::pair<std::uint32_t, RE::TESForm*> WithTarget(Func&& fn) {
        std::lock_guard<std::mutex> lock(tesTargetMutex);
        return fn(tesTargetFormID, tesTarget);
    }

    template <typename Func>
    ThreadContext* WithThreads(Func&& fn) {
        std::lock_guard<std::mutex> lock(threadsMutex);
        return fn(threads);
    }

    template <typename Func>
    std::string WithTargetVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(targetVarsMutex);
        return fn(targetVars);
    }

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

    ThreadContext* CreateThreadContext(std::string initialScriptName);

    void RemoveThreadContext(ThreadContext* threadContextToRemove) {
        if (threadContextToRemove == nullptr)
            return;
        
        WithThreads([papa = this, threadContextToRemove](auto& threads){
            auto it = std::find_if(threads.begin(), threads.end(), 
                [threadContextToRemove](const std::unique_ptr<ThreadContext>& uptr) {
                    return uptr.get() == threadContextToRemove;
                });

            if (it != threads.end()) {
                std::iter_swap(it, threads.end() - 1);
                threads.pop_back();
            }
            return nullptr;
        });
    }

    void StartAllThreads() {
        WithThreads([](auto& threads){
            for (auto& thread : threads) {
                thread->StartWork();
            }
            return nullptr;
        });
    }

    void StopAllThreads() {
        WithThreads([](auto& threads){
            for (auto& thread : threads) {
                thread->StopWork();
            }
            return nullptr;
        });
    }
};

/**
 * ThreadContext
 * One per script request sent to SLT. Tracks the stack of FrameContexts for this thread.
 * Each ThreadContext is actually being run inside a separate C++ thread to execute its script.
 */
class ThreadContext {
public:
    explicit ThreadContext(TargetContext* target, std::string initialScriptName) : target(target) {
        PushFrameContext(initialScriptName);
    }

    TargetContext* target;

    std::thread worker;
    
    std::atomic<bool> shouldStop{false};

    std::mutex callStackMutex;
    std::vector<std::unique_ptr<FrameContext>> callStack;

    std::mutex threadVarsMutex;
    std::map<std::string, std::string> threadVars;
    
    void RequestStop() { shouldStop = true; }

    bool IsTargetValid() {
        if (target == nullptr)
            return false;
        
        if (target->tesTarget == nullptr)
            return false;
        
        return true;
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

    void StartWork() {
        worker = std::thread([this]{
            this->Execute();
            this->target->RemoveThreadContext(this);
        });
    }

    void StopWork() {
        RequestStop();
        if (worker.joinable()) worker.join();
    }

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

    void Execute();
};

/**
 * FrameContext
 * Tracks local variables and state for the currently executing SLT script.
 */
class FrameContext {
public:
    explicit FrameContext(ThreadContext* thread, std::string scriptName) : thread(thread), scriptName(scriptName), currentLine(0), iterActor(nullptr), lastKey(0), iterActorFormID(0) {}

    ThreadContext* thread;
    std::string scriptName;

    std::vector<std::vector<std::string>> scriptTokens;
    std::uint32_t currentLine;
    // "ini" or "json" or similar
    std::string commandType;
    // arguments sent to this script when it was invoked via "call"; empty if none provided or the root script
    std::vector<std::string> callArgs;
    std::map<std::string, std::uint32_t> gotoLabelMap;
    std::map<std::string, std::uint32_t> gosubLabelMap;
    std::vector<std::uint32_t> returnStack;
    std::string mostRecentResult;
    std::uint32_t lastKey;
    // Serialize as formID
    std::uint32_t iterActorFormID;
    RE::Actor* iterActor;

    std::mutex localVarsMutex;
    std::map<std::string, std::string> localVars;

    template <typename Func>
    std::string WithLocalVars(Func&& fn) {
        std::lock_guard<std::mutex> lock(localVarsMutex);
        return fn(localVars);
    }

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

    // returns false when the FrameContext has nothing else to run
    bool RunStep();
};

