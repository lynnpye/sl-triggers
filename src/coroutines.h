#pragma once

#include "contexting.h"
#include <future>
#include <unordered_set>

namespace SLT {

// Simple callback for CustomResolveForm calls
class FormResolutionCallback : public RE::BSScript::IStackCallbackFunctor {
private:
    std::promise<std::string> promise;
    RE::ActiveEffect* ame;

public:
    FormResolutionCallback(std::promise<std::string> p, RE::ActiveEffect* activeEffect) 
        : promise(std::move(p)), ame(activeEffect) {}
    
    void operator()(RE::BSScript::Variable result) override {
        try {







/*

This. This right here is what we have... again... diverged for... this final point... trying to return a value from Papyrus to SKSE.
I suppose it was destiny.

*/



























            logger::debug("FormResolutionCallback: result: GetType.TypeAsString({})", result.GetType().TypeAsString());
            if (result.IsString()) {
                logger::debug("FormResolutionCallback: result.AsString({})", result.GetString());
            }
            std::string resultValue = "";
            logger::debug("FormResolutionCallback: Starting, ame={}", static_cast<void*>(ame));
            
            if (ame) {
                auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                if (vm) {
                    auto* handleP = vm->GetObjectHandlePolicy();
                    auto* bindP = vm->GetObjectBindPolicy();
                    
                    if (handleP && bindP) {
                        // Try different approaches to get the right handle
                        RE::BSTSmartPointer<RE::BSScript::Object> ameObj;
                        
                        // Approach 1: Try with the specific script type
                        auto vmhandle = handleP->GetHandleForObject(RE::ActiveEffect::VMTYPEID, ame);
                        logger::debug("FormResolutionCallback: ActiveEffect vmhandle={}", vmhandle);
                        
                        if (vmhandle) {
                            bindP->BindObject(ameObj, vmhandle);
                            bool bindSuccess = !(!ameObj);
                            logger::debug("FormResolutionCallback: ActiveEffect bind success={}", bindSuccess);
                        }
                        
                        // Approach 2: If that failed, try getting all handles for this object
                        if (!ameObj) {
                            logger::debug("FormResolutionCallback: Trying to find script object directly...");
                            
                            // Try to get the script object type for sl_triggersCmd
                            RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> scriptTypeInfo;
                            if (vm->GetScriptObjectType("sl_triggersCmd", scriptTypeInfo)) {
                                logger::debug("FormResolutionCallback: Found sl_triggersCmd type info");
                                
                                // Try to get the handle using the script type
                                // I'm not sure what you were trying, but ObjectTypeInfo has no method GetFormType
                                //auto scriptHandle = handleP->GetHandleForObject(scriptTypeInfo->GetFormType(), ame);
                                //logger::debug("FormResolutionCallback: Script vmhandle={}", scriptHandle);
                                
                                //if (scriptHandle) {
                                //    bool scriptBindSuccess = bindP->BindObject(ameObj, scriptHandle);
                                //    logger::debug("FormResolutionCallback: Script bind success={}", scriptBindSuccess);
                                //}
                            }
                        }
                        
                        if (ameObj) {
                            logger::debug("FormResolutionCallback: Successfully bound object!");
                            auto* varCustomResolveResult = ameObj->GetProperty(RE::BSFixedString("CustomResolveResult"));
                            if (varCustomResolveResult && varCustomResolveResult->IsString()) {
                                resultValue = varCustomResolveResult->GetString();
                                logger::debug("FormResolutionCallback: Got result: '{}'", resultValue);
                            } else {
                                logger::debug("FormResolutionCallback: Property not found or wrong type");
                            }
                        } else {
                            logger::error("FormResolutionCallback: All binding attempts failed");
                        }
                    }
                }
            }
            
            promise.set_value(resultValue);
        } catch (...) {
            promise.set_exception(std::current_exception());
        }
    }
    
    void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
};

// Utility class for synchronous form resolution
class FormResolver {
public:
    static bool RunOperationOnActor(FrameContext* frame, RE::Actor* targetActor, RE::ActiveEffect* cmdPrimary,
        std::vector<RE::BSFixedString> params);
};

// Manager for latent batch execution (simplified)
class BatchExecutionManager {
private:
    std::unordered_set<RE::VMStackID> executingBatches;
    std::mutex batchesMutex;

    std::mutex threadsMutex;
    std::vector<std::jthread> activeThreads;  // C++20 jthread auto-cancels
    std::atomic<bool> shutdownRequested{false};

public:

    static BatchExecutionManager& GetSingleton() {
        static BatchExecutionManager instance;
        return instance;
    }

    void RequestShutdown() {
        logger::info("BatchExecutionManager shutdown requested");
        shutdownRequested.store(true);
        
        // Request stop on all threads
        {
            std::lock_guard<std::mutex> lock(threadsMutex);
            for (auto& thread : activeThreads) {
                thread.request_stop();
            }
        }
    }
    
    void WaitForShutdown(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000)) {
        logger::info("Waiting for {} active threads to complete", activeThreads.size());
        
        auto deadline = std::chrono::steady_clock::now() + timeout;
        
        std::unique_lock<std::mutex> lock(threadsMutex);
        for (auto& thread : activeThreads) {
            if (thread.joinable()) {
                auto remaining = deadline - std::chrono::steady_clock::now();
                if (remaining > std::chrono::milliseconds(0)) {
                    lock.unlock();
                    // jthread destructor will join, but we can try to join with timeout
                    // Note: jthread doesn't have timed join, so we rely on stop_token cooperation
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    lock.lock();
                }
            }
        }
        // Clear the vector - jthread destructors will join automatically
        activeThreads.clear();
        lock.unlock();
        
        logger::info("Thread shutdown complete");
    }
    
    // Periodic cleanup of finished threads
    void CleanupFinishedThreads() {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreads.erase(
            std::remove_if(activeThreads.begin(), activeThreads.end(),
                [](const std::jthread& t) { return !t.joinable(); }),
            activeThreads.end()
        );
    }
    
    // Destructor ensures cleanup
    ~BatchExecutionManager() {
        RequestShutdown();
        WaitForShutdown();
    }
    
    bool IsExecuting(RE::VMStackID stackId) {
        std::lock_guard<std::mutex> lock(batchesMutex);
        return executingBatches.count(stackId) > 0;
    }
    
    bool StartBatch(RE::VMStackID stackId, SLTStackAnalyzer::AMEContextInfo contextInfo);

private:
    void ExecuteBatch(RE::VMStackID stackId, 
                                       SLTStackAnalyzer::AMEContextInfo contextInfo,
                                       std::stop_token stoken);
};

OnQuit([]{
    // Shutdown batch manager first to stop new operations
    BatchExecutionManager::GetSingleton().RequestShutdown();
    
    // Pause execution when game is quitting
    ContextManager::GetSingleton().PauseExecution("Game quitting");
    
    // Wait for threads to finish
    BatchExecutionManager::GetSingleton().WaitForShutdown();
});

} // namespace SLT