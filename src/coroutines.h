#pragma once

#include "contexting.h"
#include <future>
#include <unordered_set>
#include "forge.h"

namespace SLT {

class FormResolutionCallback : public RE::BSScript::IStackCallbackFunctor {
private:
    std::promise<SLT::Optional*> promise;
    RE::ActiveEffect* ame;

public:
    FormResolutionCallback(std::promise<SLT::Optional*> p, RE::ActiveEffect* activeEffect) 
        : promise(std::move(p)), ame(activeEffect) {}
    
    void operator()(RE::BSScript::Variable result) override {
        try {
            // check for handle
            if (result.IsInt()) {
                std::uint32_t handle = result.GetUInt();
                auto* opt = SLT::OptionalManager::GetOptional(handle);
                if (opt) {
                    promise.set_value(opt);
                    return;
                }
            }

            promise.set_value(nullptr);
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