#include "coroutines.h"
#include "contexting.h"
#include "engine.h"
#include "questor.h"
#include "papyrus_conversion.h"
#include <thread>

namespace SLT {

bool FormResolver::RunOperationOnActor(FrameContext* frame, RE::Actor* targetActor, RE::ActiveEffect* cmdPrimary, std::vector<RE::BSFixedString> params) {
    if (!cmdPrimary || !targetActor || params.size() == 0) {
        logger::error("Cannot call FormResolver::RunOperationOnActor with invalid cmdPrimary, targetActor, or params");
        return false;
    }
    
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) {
        logger::error("Failed to get VM singleton for RunOperationOnActor");
        return false;
    }
    
    frame->mostRecentResult = "";
    
    // Create promise/future pair for string result
    std::promise<SLT::Optional*> resultPromise;
    std::future<SLT::Optional*> resultFuture = resultPromise.get_future();
    
    // Create callback
    RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
    callback.reset(new FormResolutionCallback(std::move(resultPromise), cmdPrimary));

    auto* args =
        RE::MakeFunctionArguments(static_cast<RE::Actor*>(targetActor), static_cast<RE::ActiveEffect*>(cmdPrimary),
                                static_cast<std::vector<RE::BSFixedString>>(params));
                                
    auto cachedIt = FunctionLibrary::functionScriptCache.find(params[0].c_str());
    if (cachedIt != FunctionLibrary::functionScriptCache.end()) {
        auto& cachedScript = cachedIt->second;
        bool success = vm->DispatchStaticCall(cachedScript, params[0], args, callback);
        if (!success) {
            logger::error("Failed to dispatch static call for RunOperationOnActor ({})", params[0].c_str());
            return false;
        }
    } else {
        logger::error("Unable to find operation {} in function library cache", params[0].c_str());
        return false;
    }
    
    // I know a wait_for() was originally used, but unfortunately my users
    // could very well run a command like Utility.Wait(600) to wait for 10 minutes.
    // I can't predict how long any given actor operation will be.
    // This, I assume, has ramifications for the serialization/deserialization issues
    // though I am tempted to say that if a step gets interrupted, well, we will try
    // again on next launch won't we?

    const auto checkInterval = std::chrono::milliseconds(100);
    auto& contextManager = ContextManager::GetSingleton();

    while (true) {
        auto status = resultFuture.wait_for(checkInterval);
        
        if (status == std::future_status::ready) {
            // Operation completed normally
            try {
                //bool resolved = resultFuture.get();
                auto* opt = resultFuture.get();
                if (opt && opt->IsValid()) {
                    switch (opt->GetType()) {
                        case Optional::Type::String:
                        {
                            frame->mostRecentResult = opt->GetString();
                        }
                        break;
                        case Optional::Type::Int:
                        {
                            frame->mostRecentResult = std::to_string(opt->GetInt());   
                        }
                        break;
                        case Optional::Type::Float:
                        {
                            frame->mostRecentResult = std::to_string(opt->GetFloat());
                        }
                        break;
                        case Optional::Type::Bool:
                        {
                            frame->mostRecentResult = std::to_string(opt->GetBool() ? 1 : 0);
                        }
                        break;
                        case Optional::Type::None:
                        {
                            frame->mostRecentResult = "";
                        }
                        break;
                        case Optional::Type::Form:
                        {
                            frame->customResolveFormResult = opt->GetForm();
                        }
                        break;
                    }
                }
                return true;
            } catch (const std::exception& e) {
                logger::error("Exception in RunOperationOnActor result: {}", e.what());
                return false;
            }
        }
        
        // Check if execution should be interrupted
        if (!contextManager.ShouldContinueExecution()) {
            logger::warn("RunOperationOnActor interrupted due to execution pause");
            return false;
        }
        
        // Check if VM is still healthy
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (!vm) {
            logger::error("VM became unavailable during RunOperationOnActor");
            return false;
        }
    }
}

bool BatchExecutionManager::StartBatch(RE::VMStackID stackId, SLTStackAnalyzer::AMEContextInfo contextInfo) {
    {
        std::lock_guard<std::mutex> lock(batchesMutex);
        if (executingBatches.count(stackId) || shutdownRequested.load()) {
            return false;
        }
        executingBatches.insert(stackId);
    }
    
    {
        std::lock_guard<std::mutex> lock(threadsMutex);
        activeThreads.emplace_back([this, stackId, contextInfo](std::stop_token stoken) {
            ExecuteBatch(stackId, contextInfo, stoken);
        });
    }
    
    return true;
}

void BatchExecutionManager::ExecuteBatch(RE::VMStackID stackId, 
                                       SLTStackAnalyzer::AMEContextInfo contextInfo,
                                       std::stop_token stoken) {
    auto& contextManager = ContextManager::GetSingleton();
    const int MAX_STEPS_PER_BATCH = 50;
    const auto MAX_BATCH_TIME = std::chrono::milliseconds(10);
    
    int stepsExecuted = 0;
    auto startTime = std::chrono::steady_clock::now();
    bool hasMoreWork = false;
    
    logger::debug("Starting batch execution for stackId 0x{:X}", stackId);
    
    try {
        bool threadHasSteps = true;
        while (threadHasSteps && stepsExecuted < MAX_STEPS_PER_BATCH) {
            // Check for cancellation FIRST
            if (stoken.stop_requested() || shutdownRequested.load()) {
                logger::debug("Batch execution cancelled for stackId 0x{:X}", stackId);
                hasMoreWork = false;
                break;
            }
            
            // Check if execution should continue
            if (!contextManager.ShouldContinueExecution()) {
                logger::debug("Execution paused during batch for stackId 0x{:X}", stackId);
                hasMoreWork = true;
                break;
            }
            
            auto threadContext = contextManager.GetContext(contextInfo.handle);
            if (threadContext) {
                threadHasSteps = threadContext->ExecuteNextStep(contextInfo);
            } else {
                logger::error("ThreadContext has become invalid during execution for handle {}", contextInfo.handle);
                threadHasSteps = false;
            }
            stepsExecuted++;
        }
    } catch (const std::exception& e) {
        logger::error("Exception in batch execution: {}", e.what());
        hasMoreWork = false;
    }
    
    // Clean up
    {
        std::lock_guard<std::mutex> lock(batchesMutex);
        executingBatches.erase(stackId);
    }
    
    // Only signal completion if we weren't cancelled
    if (!stoken.stop_requested() && !shutdownRequested.load()) {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (vm) {
            logger::debug("Batch completed for stackId 0x{:X}, hasMoreWork: {}", stackId, hasMoreWork);
            vm->ReturnLatentResult(stackId, hasMoreWork);
        }
    }
}

} // namespace SLT