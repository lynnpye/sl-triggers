#include "forge/forge.h"

#include "engine.h"

namespace SLT {
using namespace SLT;

#pragma region ScriptPoolManager

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

#pragma region GlobalContext

std::shared_mutex GlobalContext::maplock;
std::shared_mutex GlobalContext::varlock;

// Data members (protected by coordinationLock)
std::unordered_map<RE::FormID, ForgeHandle> GlobalContext::formIdTargetMap;
std::map<std::string, std::string> GlobalContext::globalVars;

bool GlobalContext::StartSLTScript(RE::Actor* target, std::string_view initialScriptName) {
    if (!target || initialScriptName.empty()) return false;

    try {
        ForgeHandle handle;
        try {
            std::unique_lock<std::shared_mutex> lock(maplock);
            // Get or create target context
            RE::FormID targetId = target->GetFormID();
            
            ForgeHandle targetHandle;
            auto tcit = formIdTargetMap.find(targetId);
            if (tcit == formIdTargetMap.end()) {
                auto [newIt, inserted] =
                    formIdTargetMap.emplace(
                        targetId,
                        TargetContextManager::Create(std::move(std::make_unique<TargetContext>(target)))
                    );
                targetHandle = newIt->second;
            } else {
                targetHandle = tcit->second;
            }

            // Create new thread context
            if (auto* targetContext = TargetContextManager::GetFromHandle(targetHandle)) {
                auto threadPtr = std::make_unique<ThreadContext>(targetHandle, initialScriptName);
                ForgeHandle threadHandle = ThreadContextManager::Create(std::move(threadPtr));
                auto* thread = ThreadContextManager::GetFromHandle(threadHandle);
                if (thread && thread->Initialize()) {
                    targetContext->threads.push_back(threadHandle);
                    handle = threadHandle;
                } else {
                    ThreadContextManager::DestroyFromHandle(threadHandle);
                    handle = 0;
                }
            }
        } catch (...) {
            throw;
        }

        if (handle) {
            // eventually I would like this to be a) immediately callable like this but also b) scheduled to repeat because failure could have just meant we lacked enough MGEF objects in the pool and should retry soon
            if (ScriptPoolManager::GetSingleton().ApplyScript(target, initialScriptName)) {
                return true;
            }
        }
        
        logger::error("Unable to apply script({}) to target ({})", initialScriptName, Util::String::ToHex(target->GetFormID()));
    } catch (const std::exception& e) {
        logger::error("Exception in StartSLTScript: {}", e.what());
    } catch (...) {
        logger::error("Unknown exception in StartSLTScript");
    }
    
    return false;
}

bool GlobalContext::Register(RE::BSScript::IVirtualMachine* vm) {
    return true;
}

void GlobalContext::OnSave(SKSE::SerializationInterface* intfc) {
    using SH = SerializationHelper;

    if (!SH::WriteData(intfc, ForgeManager::nextForgeHandle)) {
        logger::error("OnSave: Failed to write next handle for ForgeManager");
        return;
    }

    // Write global variables
    if (!SH::WriteStringMap(intfc, globalVars)) return;
    
    // Write target contexts
    std::size_t mapCount = formIdTargetMap.size();
    if (!SH::WriteData(intfc, mapCount)) return;
    
    for (const auto& [formID, targetHandle] : formIdTargetMap) {
        if (!SH::WriteData(intfc, formID)) return;
        if (!SH::WriteData(intfc, targetHandle)) return;
    }
}

void GlobalContext::OnLoad(SKSE::SerializationInterface* intfc) {
    using SH = SerializationHelper;

    if (!SH::ReadData(intfc, ForgeManager::nextForgeHandle)) {
        logger::error("OnSave: Failed to read next handle for ForgeManager");
        return;
    }
    
    // Read global variables
    if (!SH::ReadStringMap(intfc, globalVars)) return;
    
    // Read target contexts
    formIdTargetMap.clear();
    
    std::size_t contextCount;
    if (!SH::ReadData(intfc, contextCount)) return;
    
    RE::FormID formId;
    ForgeHandle handle;
    for (std::size_t i = 0; i < contextCount; ++i) {
        if (!SH::ReadData(intfc, formId)) return;
        if (!SH::ReadData(intfc, handle)) return;
        
        formIdTargetMap[formId] = handle;
    }
}

void GlobalContext::OnRevert(SKSE::SerializationInterface* intfc) {
    ForgeManager::nextForgeHandle = 1;
}


// Global Variable Accessors (keep existing validation)
std::string GlobalContext::SetGlobalVar(std::string_view name, std::string_view value) {
    if (name.empty()) {
        logger::warn("SetGlobalVar: Variable name cannot be empty");
        return "";
    }
    
    std::string response = "";
    try {
        std::unique_lock<std::shared_mutex> lock(GlobalContext::varlock);
        auto result = GlobalContext::globalVars.emplace(name, value);
        if (!result.second) {
            result.first->second = value; // Update existing
        }
        response = result.first->second;
    } catch (...) {
        throw;
    }
    return response;
}

std::string GlobalContext::GetGlobalVar(std::string_view name) {
    if (name.empty()) {
        return "";
    }
    
    std::string response = "";
    try {
        std::shared_lock<std::shared_mutex> lock(GlobalContext::varlock);
        auto it = GlobalContext::globalVars.find(std::string(name));
        response = (it != GlobalContext::globalVars.end()) ? it->second : std::string{};
    } catch (...) {
        throw;
    }
    return response;
}

bool GlobalContext::HasGlobalVar(std::string_view name) {
    if (name.empty()) {
        return false;
    }
    
    try {
        std::shared_lock<std::shared_mutex> lock(GlobalContext::varlock);
        auto it = GlobalContext::globalVars.find(std::string(name));
        return (it != GlobalContext::globalVars.end()) ? true : false;
    } catch (...) {
        throw;
    }
    return false;
}

std::string GlobalContext::SetVar(FrameContext* frame, std::string_view token, std::string_view value) {
    if (!frame || token.empty()) return "";

    logger::debug("SetVar({}) to ({})", token, value);
    
    if (token[0] == '$' && token.length() > 1) {
        std::string remaining = std::string(token.substr(1));
        
        size_t hashPos = remaining.find('#');
        
        if (hashPos != std::string::npos) {
            std::string scope = remaining.substr(0, hashPos);
            std::string varName = remaining.substr(hashPos + 1);
            
            if (varName.empty()) {
                FrameLogWarn(frame, "SetVar: Variable name cannot be empty after scope separator");
                return "";
            }
            
            if (str::iEquals(scope, "session")) {
                frame->pthread()->SetThreadVar(varName, value);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->pthread() || !frame->pthread()->ptarget()) {
                    FrameLogError(frame, "SetVar: No valid target for actor scope variable");
                    return "";
                }
                frame->pthread()->ptarget()->SetTargetVar(varName, value);
            }
            else if (str::iEquals(scope, "global")) {
                GlobalContext::GetSingleton().SetGlobalVar(varName, value);
            }
            else {
                FrameLogWarn(frame, "Unknown variable scope for SET: {}", scope);
                return "";
            }
        } else {
            // Local variables
            frame->SetLocalVar(remaining, value);
        }
    } else {
        FrameLogWarn(frame, "Invalid variable token for SET (missing $): {}", token);
        return "";
    }
    return std::string(value);
}

std::string GlobalContext::GetVar(FrameContext* frame, std::string_view token) {
    if (!frame || token.empty()) return "";
    
    if (token[0] == '$' && token.length() > 1) {
        std::string remaining = std::string(token.substr(1));
        
        size_t hashPos = remaining.find('#');
        
        if (hashPos != std::string::npos) {
            std::string scope = remaining.substr(0, hashPos);
            std::string varName = remaining.substr(hashPos + 1);
            
            if (varName.empty()) return "";
            
            if (str::iEquals(scope, "session")) {
                return frame->pthread()->GetThreadVar(varName);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->pthread() || !frame->pthread()->ptarget()) return "";
                return frame->pthread()->ptarget()->GetTargetVar(varName);
            }
            else if (str::iEquals(scope, "global")) {
                return GlobalContext::GetSingleton().GetGlobalVar(varName);
            }
            else {
                FrameLogWarn(frame, "Unknown variable scope for GET: {}", scope);
                return "";
            }
        } else {
            // Local variables
            return frame->GetLocalVar(remaining);
        }
    }
    
    return "";
}

bool GlobalContext::HasVar(FrameContext* frame, std::string_view token) {
    if (!frame || token.empty()) return false;
    
    if (token[0] == '$' && token.length() > 1) {
        std::string remaining = std::string(token.substr(1));
        
        size_t hashPos = remaining.find('#');
        
        if (hashPos != std::string::npos) {
            std::string scope = remaining.substr(0, hashPos);
            std::string varName = remaining.substr(hashPos + 1);
            
            if (varName.empty())
                return false;
            
            if (str::iEquals(scope, "session")) {
                return frame->pthread()->HasThreadVar(varName);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->pthread() || !frame->pthread()->ptarget())
                    return false;
                return frame->pthread()->ptarget()->HasTargetVar(varName);
            }
            else if (str::iEquals(scope, "global")) {
                return GlobalContext::GetSingleton().HasGlobalVar(varName);
            }
            else {
                FrameLogWarn(frame, "Unknown variable scope for GET: {}", scope);
                return false;
            }
        } else {
            // Local variables
            return frame->HasLocalVar(remaining);
        }
    }
    
    return false;
}

// Leave this as is. We will only do custom form resolution
std::string GlobalContext::ResolveValueVariable(ForgeHandle threadContextHandle, std::string_view token) {
    if (token.empty()) return "";

    auto* thread = ThreadContextManager::GetFromHandle(threadContextHandle);
    if (!thread) {
        return "";
    }
    auto* frame = thread->GetCurrentFrame();

    if (!frame) return "";
    
    // Most recent result
    if (token == "$$") {
        return frame->mostRecentResult;
    }

    auto& manager = GlobalContext::GetSingleton();

    auto optresult = ([&token, frame]() -> std::optional<std::string> {
        if (token[0] == '$' && token.length() > 1) {
            std::string remaining = std::string(token.substr(1));
            
            size_t hashPos = remaining.find('#');
            
            if (hashPos != std::string::npos) {
                std::string scope = remaining.substr(0, hashPos);
                std::string varName = remaining.substr(hashPos + 1);
                
                if (varName.empty()) return std::nullopt;
                
                if (str::iEquals(scope, "session")) {
                    if (frame->pthread()->HasThreadVar(varName))
                        return frame->pthread()->GetThreadVar(varName);
                }
                else if (str::iEquals(scope, "actor")) {
                    if (!frame->pthread() || !frame->pthread()->ptarget())
                        return std::nullopt;
                    if (frame->pthread()->ptarget()->HasTargetVar(varName))
                        return frame->pthread()->ptarget()->GetTargetVar(varName);
                }
                else if (str::iEquals(scope, "global")) {
                    if (GlobalContext::GetSingleton().HasGlobalVar(varName))
                        return GlobalContext::GetSingleton().GetGlobalVar(varName);
                }
                else {
                    FrameLogWarn(frame, "Unknown variable scope for GET: {}", scope);
                    return std::nullopt;
                }
            } else {
                // Local variables
                if (frame->HasLocalVar(remaining))
                    return frame->GetLocalVar(remaining);
            }
        }
                
        return std::nullopt; // Not a variable
    })();

    if (optresult.has_value()) {
        return optresult.value();
    }
    return std::string(token);
}

// Enhanced ResolveFormVariable that mimics the Papyrus _slt_SLTResolveForm logic
bool GlobalContext::ResolveFormVariable(ForgeHandle threadContextHandle, std::string_view token) {
    auto* thread = ThreadContextManager::GetFromHandle(threadContextHandle);
    if (!thread) {
        return false;
    }
    auto* frame = thread->GetCurrentFrame();
    if (!frame || token.empty()) return false;
    
    // Handle built-in form tokens first (like _slt_SLTResolveForm)
    if (str::iEquals(token, "#player") || str::iEquals(token, "$player")) {
        frame->customResolveFormResult = RE::PlayerCharacter::GetSingleton();
        return true;
    }
    if (str::iEquals(token, "#self") || str::iEquals(token, "$self")) {
        frame->customResolveFormResult = frame->pthread()->ptarget()->AsActor();
        return true;
    }
    if (str::iEquals(token, "#actor") || str::iEquals(token, "$actor")) {
        frame->customResolveFormResult = frame->iterActor;
        return true;
    }
    if (str::iEquals(token, "#none") || str::iEquals(token, "none") || token.empty()) {
        frame->customResolveFormResult = nullptr;
        return true;
    }
    // cutting out the extension.form resolution feature
    if (str::iEquals(token.substr(0, 8), "#partner")) {
        int skip = -1;
        if (token.size() == 8) {
            skip = 0;
        }
        else if (token.size() == 9) {
            switch (token.at(8)) {
                case '1': skip = 0; break;
                case '2': skip = 1; break;
                case '3': skip = 2; break;
                case '4': skip = 3; break;
            }
        }
        if (skip != -1) {
            std::vector<RE::BSFixedString> params;
            switch (skip) {
                case 0: params.push_back(RE::BSFixedString("resolve_partner1")); break;
                case 1: params.push_back(RE::BSFixedString("resolve_partner2")); break;
                case 2: params.push_back(RE::BSFixedString("resolve_partner3")); break;
                case 3: params.push_back(RE::BSFixedString("resolve_partner4")); break;
            }
            if (!OperationRunner::RunOperationOnActor(frame->pthread()->ptarget()->AsActor(), frame->pthread()->ame, params)) {
                logger::error("Unable to resolve SexLab partner variable");
            }
        }
    }
    
    // Try direct form lookup for form IDs or editor IDs
    auto* aForm = FormUtil::Parse::GetForm(ResolveValueVariable(threadContextHandle, token)); // this may return nullptr but at this point SLT and all the extensions had a crack
    if (aForm && frame) {
        frame->customResolveFormResult = aForm;
        return true;
    }

    frame->customResolveFormResult = nullptr;
    return false;
}

void GlobalContext::CleanupContext(ForgeHandle threadContextHandle) {
    auto* threadContext = ThreadContextManager::GetFromHandle(threadContextHandle);
    if (threadContext) {
        if (auto* targetContext = threadContext->ptarget()) {
            auto it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(), 
                [&](const ForgeHandle tchandle) {
                    return tchandle == threadContextHandle;
                });

            if (it != targetContext->threads.end()) {
                targetContext->threads.erase(it);
            }
        }
        ThreadContextManager::DestroyFromHandle(threadContextHandle);
    }
}

#pragma endregion


}