#include "coroutines.h"
#include "engine.h"

namespace SLT {

bool OperationRunner::RunOperationOnActor(RE::Actor* targetActor, 
                                         RE::ActiveEffect* cmdPrimary, 
                                         const std::vector<RE::BSFixedString>& params) {
    if (!cmdPrimary || !targetActor || params.empty()) {
        logger::error("RunOperationOnActor: Invalid parameters");
        return false;
    }
    
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) {
        logger::error("RunOperationOnActor: Failed to get VM singleton");
        return false;
    }
    
    auto cachedIt = FunctionLibrary::functionScriptCache.find(params[0].c_str());
    if (cachedIt == FunctionLibrary::functionScriptCache.end()) {
        logger::error("RunOperationOnActor: Unable to find operation {} in function library cache", params[0].c_str());
        return false;
    }
    
    auto vmhandle = vm->GetObjectHandlePolicy()->GetHandleForObject(RE::ActiveEffect::VMTYPEID, cmdPrimary);
    RE::BSFixedString callbackEvent("OnSetOperationCompleted");
    
    auto resultCallback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>(
        new VoidCallbackFunctor([vm, vmhandle, callbackEvent]() {
            SKSE::GetTaskInterface()->AddTask([vm, vmhandle, callbackEvent]() {
                vm->SendEvent(vmhandle, callbackEvent, RE::MakeFunctionArguments());
            });
        })
    );
    
    auto* operationArgs = RE::MakeFunctionArguments(
        static_cast<RE::Actor*>(targetActor), 
        static_cast<RE::ActiveEffect*>(cmdPrimary),
        static_cast<std::vector<RE::BSFixedString>>(params)
    );
    
    auto& cachedScript = cachedIt->second;
    bool success = vm->DispatchStaticCall(cachedScript, params[0], operationArgs, resultCallback);
    
    if (!success) {
        logger::error("RunOperationOnActor: Failed to dispatch static call for operation {}", params[0].c_str());
    }
    
    return success;
}

bool OperationRunner::RunOperationOnActor(RE::Actor* targetActor, 
                                         RE::ActiveEffect* cmdPrimary, 
                                         const std::vector<std::string>& params) {
    if (params.empty()) {
        return false;
    }
    
    std::vector<RE::BSFixedString> bsParams;
    bsParams.reserve(params.size());
    for (const auto& param : params) {
        bsParams.emplace_back(param);
    }
    
    return RunOperationOnActor(targetActor, cmdPrimary, bsParams);
}

} // namespace SLT