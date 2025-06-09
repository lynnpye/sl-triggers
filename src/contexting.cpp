#include "contexting.h"
#include "engine.h"
#include "coroutines.h"


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

ThreadContextHandle ContextManager::nextContextId = 1;

bool ContextManager::StartSLTScript(RE::Actor* target, std::string_view initialScriptName) {
    if (!target || initialScriptName.empty()) return false;

    try {
        auto& cmgr = ContextManager::GetSingleton();
        // Use WriteData to modify contexts
        ThreadContextHandle handle = WriteData([target, initialScriptName](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) -> ThreadContextHandle {
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
            ThreadContextHandle handle = ContextManager::GetNextContextId();
            auto newPtr = std::make_shared<ThreadContext>(targetContext, initialScriptName, handle);
            
            targetContext->threads.push_back(newPtr);
            activeContexts[handle] = newPtr;
            
            return handle;
        });

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

bool ContextManager::Serialize(SKSE::SerializationInterface* a_intfc) const {
    LOG_FUNCTION_SCOPE("ContextManager::Serialize");
    using SH = SerializationHelper;
    
    // Clear all form caches before serialization to avoid issues
    // We need to do this in ReadData since Serialize is const
    return ReadData([a_intfc](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
        
        // Continue with existing serialization logic using SerializationHelper
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
    LOG_FUNCTION_SCOPE("ContextManager::Deserialize");
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
                activeContexts[threadContext->threadContextHandle] = threadContext;
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

bool ContextManager::HasGlobalVar(std::string_view name) const {
    if (name.empty()) {
        return false;
    }
    
    return ReadData([name = std::string(name)](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> bool {
        auto it = globalVars.find(name);
        return (it != globalVars.end()) ? true : false;
    });
}

std::string ContextManager::SetVar(FrameContext* frame, std::string_view token, std::string_view value) {
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
                frame->thread->SetThreadVar(varName, value);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->thread || !frame->thread->target) {
                    FrameLogError(frame, "SetVar: No valid target for actor scope variable");
                    return "";
                }
                frame->thread->target->SetTargetVar(varName, value);
            }
            else if (str::iEquals(scope, "global")) {
                ContextManager::GetSingleton().SetGlobalVar(varName, value);
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

std::string ContextManager::GetVar(FrameContext* frame, std::string_view token) const {
    if (!frame || token.empty()) return "";
    
    if (token[0] == '$' && token.length() > 1) {
        std::string remaining = std::string(token.substr(1));
        
        size_t hashPos = remaining.find('#');
        
        if (hashPos != std::string::npos) {
            std::string scope = remaining.substr(0, hashPos);
            std::string varName = remaining.substr(hashPos + 1);
            
            if (varName.empty()) return "";
            
            if (str::iEquals(scope, "session")) {
                return frame->thread->GetThreadVar(varName);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->thread || !frame->thread->target) return "";
                return frame->thread->target->GetTargetVar(varName);
            }
            else if (str::iEquals(scope, "global")) {
                return ContextManager::GetSingleton().GetGlobalVar(varName);
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

bool ContextManager::HasVar(FrameContext* frame, std::string_view token) const {
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
                return frame->thread->HasThreadVar(varName);
            }
            else if (str::iEquals(scope, "actor")) {
                if (!frame->thread || !frame->thread->target)
                    return false;
                return frame->thread->target->HasTargetVar(varName);
            }
            else if (str::iEquals(scope, "global")) {
                return ContextManager::GetSingleton().HasGlobalVar(varName);
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
std::string ContextManager::ResolveValueVariable(ThreadContextHandle threadContextHandle, std::string_view token) const {
    if (token.empty()) return "";

    auto* frame = GetSingleton().GetFrameContext(threadContextHandle);

    if (!frame) return "";
    
    // Most recent result
    if (token == "$$") {
        return frame->mostRecentResult;
    }

    auto& manager = ContextManager::GetSingleton();

    auto optresult = manager.ReadData([&token, frame]
    (const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) -> std::optional<std::string> {
        if (token[0] == '$' && token.length() > 1) {
            std::string remaining = std::string(token.substr(1));
            
            size_t hashPos = remaining.find('#');
            
            if (hashPos != std::string::npos) {
                std::string scope = remaining.substr(0, hashPos);
                std::string varName = remaining.substr(hashPos + 1);
                
                if (varName.empty()) return std::nullopt;
                
                if (str::iEquals(scope, "session")) {
                    if (frame->thread->HasThreadVar(varName))
                        return frame->thread->GetThreadVar(varName);
                }
                else if (str::iEquals(scope, "actor")) {
                    if (!frame->thread || !frame->thread->target)
                        return std::nullopt;
                    if (frame->thread->target->HasTargetVar(varName))
                        return frame->thread->target->GetTargetVar(varName);
                }
                else if (str::iEquals(scope, "global")) {
                    if (ContextManager::GetSingleton().HasGlobalVar(varName))
                        return ContextManager::GetSingleton().GetGlobalVar(varName);
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
    });

    if (optresult.has_value()) {
        return optresult.value();
    }
    return std::string(token);
}

// Enhanced ResolveFormVariable that mimics the Papyrus _slt_SLTResolveForm logic
bool ContextManager::ResolveFormVariable(ThreadContextHandle threadContextHandle, std::string_view token) const {
    auto* frame = GetSingleton().GetFrameContext(threadContextHandle);
    if (!frame || token.empty()) return false;
    
    // Handle built-in form tokens first (like _slt_SLTResolveForm)
    if (str::iEquals(token, "#player") || str::iEquals(token, "$player")) {
        frame->customResolveFormResult = RE::PlayerCharacter::GetSingleton();
        return true;
    }
    if (str::iEquals(token, "#self") || str::iEquals(token, "$self")) {
        frame->customResolveFormResult = frame->thread->target->AsActor();
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
            if (!OperationRunner::RunOperationOnActor(frame->thread->target->AsActor(), frame->thread->ame, params)) {
                logger::error("Unable to resolve SexLab partner variable");
            }
        }
    }
    
    // Try direct form lookup for form IDs or editor IDs
    auto* aForm = FormUtil::Parse::GetForm(ResolveValueVariable(threadContextHandle, token)); // this may return nullptr but at this point SLT and all the extensions had a crack
    if (aForm) {
        frame->customResolveFormResult = aForm;
        return true;
    }

    frame->customResolveFormResult = nullptr;
    return false;
}

void ContextManager::CleanupContext(ThreadContextHandle contextId) {
    WriteData([contextId](auto& targetContexts, auto& activeContexts, auto& globalVars, auto& nextId) {
        auto activeIt = activeContexts.find(contextId);
        if (activeIt != activeContexts.end()) {
            auto threadContext = activeContexts[contextId];
            if (threadContext->target) {
                auto* targetContext = threadContext->target;
                if (targetContext) {
                    //targetContext->RemoveThreadContext(threadContext.get());
                    
                    auto it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(), 
                        [&](const std::shared_ptr<ThreadContext>& sptr) {
                            return sptr == threadContext;
                        });

                    if (it != targetContext->threads.end()) {
                        ThreadContextHandle handle = it->get()->threadContextHandle;
                        targetContext->threads.erase(it);
                    }
                }
            }
            activeContexts.erase(activeIt);
        }
        return nullptr;
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
        auto thread = std::make_shared<ThreadContext>(this, "", 0);
        if (!thread->Deserialize(a_intfc, this)) return false;
        threads.push_back(thread);
    }
    
    // Read target variables
    if (!SH::ReadStringMap(a_intfc, targetVars)) return false;
    
    return true;
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
        while (previousFrame && (previousFrame->currentLine >= previousFrame->scriptTokens.size())) {
            callStack.pop_back();
            if (callStack.size() < 1)
                return false;
            previousFrame = callStack.back().get();
        }
        
        return (previousFrame && (previousFrame->currentLine < previousFrame->scriptTokens.size()));
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
    bool result = ParseResult::Success == Parser::ParseScript(this);
    if (!result) {
        FrameLogError(this, "Failed to parse script '{}': error code {}", scriptName, static_cast<int>(result));
        return false;
    }
    return currentLine < scriptTokens.size();
}

#pragma endregion
}