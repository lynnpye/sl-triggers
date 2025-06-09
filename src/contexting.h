#pragma once

namespace SLT {
class ContextManager;
class TargetContext;
class ThreadContext;
class FrameContext;
class CommandLine;


#pragma region CommandLine
/**
 * CommandLine
 * "You know what the chain of command is? It's the ruttin' chain I beat you over the head with until you understand who's in command!"
 *  - a famous space mercenary
 */
class CommandLine {
public: // I know, I know

    CommandLine() = default;

    explicit CommandLine(std::size_t _lineNumber, std::vector<std::string> _tokens)
        : lineNumber(_lineNumber), tokens(_tokens) {}

    std::size_t lineNumber;
    std::vector<std::string> tokens;
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);
};
#pragma endregion

#pragma region VariableDumper

class VariableDumper {
public:
    // Main entry point - dumps a variable with optional prefix
    static std::string DumpVariable(const RE::BSScript::Variable& var, 
                                  const std::string& prefix = "",
                                  int maxDepth = 3) {
        return DumpVariableImpl(var, prefix, 0, maxDepth);
    }
    
    // Convenience function for logging
    static void LogVariable(const RE::BSScript::Variable& var, 
                           const std::string& name = "Variable",
                           spdlog::level::level_enum level = spdlog::level::info) {
        std::string dump = DumpVariable(var, name + ": ");
        
        switch (level) {
            case spdlog::level::debug:
                logger::debug("{}", dump);
                break;
            case spdlog::level::warn:
                logger::warn("{}", dump);
                break;
            case spdlog::level::err:
                logger::error("{}", dump);
                break;
            default:
                logger::info("{}", dump);
                break;
        }
    }

private:
    static std::string DumpVariableImpl(const RE::BSScript::Variable& var, 
                                       const std::string& prefix, 
                                       int currentDepth, 
                                       int maxDepth) {
        if (currentDepth > maxDepth) {
            return prefix + "[MAX_DEPTH_EXCEEDED]";
        }

        std::string result = prefix;
        
        try {
            // Get type information
            auto type = var.GetType();
            std::string typeStr = type.TypeAsString();
            auto rawType = type.GetRawType();
            
            result += std::format("Type: {} (Raw: {}) | ", typeStr, static_cast<int>(rawType));
            
            // Handle different variable types based on the actual TypeInfo enum
            switch (rawType) {
                case RE::BSScript::TypeInfo::RawType::kNone:
                    result += "Value: [NONE]";
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kBool:
                    result += std::format("Value: {}", var.GetBool() ? "true" : "false");
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kInt:
                    {
                        std::int32_t intVal = var.GetSInt();
                        result += std::format("Value: {} (0x{:X})", intVal, static_cast<std::uint32_t>(intVal));
                    }
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kFloat:
                    result += std::format("Value: {:.6f}", var.GetFloat());
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kString:
                    {
                        auto strView = var.GetString();
                        result += std::format("Value: \"{}\" (length: {})", strView, strView.length());
                    }
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kObject:
                    {
                        auto obj = var.GetObject();
                        if (obj) {
                            result += DumpObject(obj.get(), currentDepth, maxDepth);
                        } else {
                            result += "Value: [NULL_OBJECT]";
                        }
                    }
                    break;
                    
                case RE::BSScript::TypeInfo::RawType::kNoneArray:
                case RE::BSScript::TypeInfo::RawType::kObjectArray:
                case RE::BSScript::TypeInfo::RawType::kStringArray:
                case RE::BSScript::TypeInfo::RawType::kIntArray:
                case RE::BSScript::TypeInfo::RawType::kFloatArray:
                case RE::BSScript::TypeInfo::RawType::kBoolArray:
                    {
                        auto arr = var.GetArray();
                        if (arr) {
                            result += DumpArray(arr.get(), currentDepth, maxDepth);
                        } else {
                            result += "Value: [NULL_ARRAY]";
                        }
                    }
                    break;
                    
                default:
                    // Handle custom object types (class pointers as mentioned in the comment)
                    if (var.IsObject()) {
                        auto obj = var.GetObject();
                        if (obj) {
                            result += DumpObject(obj.get(), currentDepth, maxDepth);
                        } else {
                            result += "Value: [NULL_CUSTOM_OBJECT]";
                        }
                    } else {
                        result += std::format("Value: [UNKNOWN_TYPE_{}]", static_cast<int>(rawType));
                    }
                    break;
            }
            
        } catch (const std::exception& e) {
            result += std::format("[EXCEPTION: {}]", e.what());
        } catch (...) {
            result += "[UNKNOWN_EXCEPTION]";
        }
        
        return result;
    }
    
    static std::string DumpObject(RE::BSScript::Object* obj, int currentDepth, int maxDepth) {
        if (!obj) {
            return "Value: [NULL_OBJECT]";
        }
        
        std::string result;
        
        try {
            // Get object type info
            auto typeInfo = obj->GetTypeInfo();
            if (typeInfo) {
                std::string typeName = typeInfo->GetName();
                result += std::format("Value: [OBJECT: {}]", typeName);
                
                // Add inheritance chain if there's a parent
                auto parent = typeInfo->GetParent();
                if (parent) {
                    result += std::format(" extends {}", parent->GetName());
                }
                
                // Add linking status
                result += std::format(" (Linked: {})", typeInfo->IsLinked() ? "Yes" : "No");
                
                // If we haven't hit max depth, show some properties
                if (currentDepth < maxDepth) {
                    result += DumpObjectProperties(obj, currentDepth + 1, maxDepth);
                    
                    // Optionally show some method counts for debugging
                    if (currentDepth == 0) { // Only show on top level to avoid clutter
                        result += std::format(" Methods: [Global:{}, Member:{}, States:{}]",
                                            typeInfo->GetNumGlobalFuncs(),
                                            typeInfo->GetNumMemberFuncs(),
                                            typeInfo->GetNumNamedStates());
                    }
                }
            } else {
                result += "Value: [OBJECT: UNKNOWN_TYPE]";
            }
            
            // Add handle information if available
            auto handle = obj->GetHandle();
            result += std::format(" Handle: 0x{:X}", handle);
            
        } catch (const std::exception& e) {
            result += std::format("Value: [OBJECT_EXCEPTION: {}]", e.what());
        } catch (...) {
            result += "Value: [OBJECT_UNKNOWN_EXCEPTION]";
        }
        
        return result;
    }
    
    static std::string DumpObjectProperties(RE::BSScript::Object* obj, int currentDepth, int maxDepth) {
        if (!obj || currentDepth > maxDepth) {
            return "";
        }
        
        std::string result = " Properties: {";
        
        try {
            auto typeInfo = obj->GetTypeInfo();
            if (!typeInfo) {
                return " Properties: [NO_TYPE_INFO]";
            }
            
            // Get property count and iterate through them using the real API
            auto propCount = typeInfo->GetNumProperties();
            bool first = true;
            
            // Get the property iterator from ObjectTypeInfo
            const auto* propIter = typeInfo->GetPropertyIter();

            std::uint32_t MAX_PROPERTIES = 40;
            
            for (std::uint32_t i = 0; i < propCount && i < MAX_PROPERTIES; ++i) { // Limit to first 10 properties
                try {
                    if (propIter) {
                        const auto& propInfo = propIter[i];
                        RE::BSFixedString propName = propInfo.name;
                        
                        if (!first) result += ", ";
                        first = false;
                        
                        // Use the PropertyTypeInfo to get type information
                        const auto& propTypeInfo = propInfo.info.type;
                        result += std::format("{}({})", propName.c_str(), propTypeInfo.TypeAsString());
                        
                        // Try to get the actual property value
                        auto propVar = obj->GetProperty(propName);
                        if (propVar) {
                            result += DumpSimpleValue(*propVar);
                        } else {
                            result += "=[NULL_PROP]";
                        }
                    }
                } catch (...) {
                    result += std::format("prop{}=[ERROR]", i);
                }
            }
            
            if (propCount > 10) {
                result += std::format(", ... ({} more)", propCount - 10);
            }
            
        } catch (const std::exception& e) {
            result += std::format("[PROP_EXCEPTION: {}]", e.what());
        } catch (...) {
            result += "[PROP_UNKNOWN_EXCEPTION]";
        }
        
        result += "}";
        return result;
    }
    
    static std::string DumpArray(RE::BSScript::Array* arr, int currentDepth, int maxDepth) {
        if (!arr) {
            return "Value: [NULL_ARRAY]";
        }
        
        std::string result;
        
        try {
            auto size = arr->size();
            const auto& elementTypeInfo = arr->type_info();  // This returns a TypeInfo object
            
            result += std::format("Value: [ARRAY: {} elements, type: {}]", 
                                size, elementTypeInfo.TypeAsString());
            
            // If we haven't hit max depth and array isn't too big, show some elements
            if (currentDepth < maxDepth && size > 0) {
                result += " Elements: [";
                
                std::uint32_t elementsToShow = std::min(size, 5u); // Show first 5 elements
                
                for (std::uint32_t i = 0; i < elementsToShow; ++i) {
                    if (i > 0) result += ", ";
                    
                    try {
                        // Use array subscript operator which is more direct
                        const auto& element = (*arr)[i];
                        
                        // Use our helper function for simple values
                        std::string elementValue = DumpSimpleValue(element);
                        // Remove the '=' prefix from the simple value dump
                        if (!elementValue.empty() && elementValue[0] == '=') {
                            elementValue = elementValue.substr(1);
                        }
                        result += elementValue;
                    } catch (...) {
                        result += "[ERROR]";
                    }
                }
                
                if (size > elementsToShow) {
                    result += std::format(", ... ({} more)", size - elementsToShow);
                }
                
                result += "]";
            }
            
        } catch (const std::exception& e) {
            result += std::format("Value: [ARRAY_EXCEPTION: {}]", e.what());
        } catch (...) {
            result += "Value: [ARRAY_UNKNOWN_EXCEPTION]";
        }
        
        return result;
    }
    
    // Helper function to dump simple values without recursion
    static std::string DumpSimpleValue(const RE::BSScript::Variable& var) {
        try {
            auto rawType = var.GetType().GetRawType();
            
            switch (rawType) {
                case RE::BSScript::TypeInfo::RawType::kBool:
                    return std::format("={}", var.GetBool() ? "true" : "false");
                    
                case RE::BSScript::TypeInfo::RawType::kInt:
                    return std::format("={}", var.GetSInt());
                    
                case RE::BSScript::TypeInfo::RawType::kFloat:
                    return std::format("={:.2f}", var.GetFloat());
                    
                case RE::BSScript::TypeInfo::RawType::kString:
                    {
                        auto str = var.GetString();
                        if (str.length() > 20) {
                            return std::format("=\"{}...\"", str.substr(0, 20));
                        } else {
                            return std::format("=\"{}\"", str);
                        }
                    }
                    
                case RE::BSScript::TypeInfo::RawType::kNone:
                    return "=[NONE]";
                    
                default:
                    return "=[COMPLEX]";
            }
        } catch (...) {
            return "=[ERROR]";
        }
    }
};

// Integration with your existing logging system
template<typename... Args>
inline void LogVariableInfo(const RE::BSScript::Variable& var, 
                           std::format_string<Args...> fmt, Args&&... args) {
    std::string name = std::format(fmt, std::forward<Args>(args)...);
    logger::info("{}: {}", name, VariableDumper::DumpVariable(var));
}

template<typename... Args>
inline void LogVariableDebug(const RE::BSScript::Variable& var, 
                            std::format_string<Args...> fmt, Args&&... args) {
    std::string name = std::format(fmt, std::forward<Args>(args)...);
    logger::debug("{}: {}", name, VariableDumper::DumpVariable(var));
}

template<typename... Args>
inline void LogVariableWarn(const RE::BSScript::Variable& var, 
                           std::format_string<Args...> fmt, Args&&... args) {
    std::string name = std::format(fmt, std::forward<Args>(args)...);
    logger::warn("{}: {}", name, VariableDumper::DumpVariable(var));
}

template<typename... Args>
inline void LogVariableError(const RE::BSScript::Variable& var, 
                            std::format_string<Args...> fmt, Args&&... args) {
    std::string name = std::format(fmt, std::forward<Args>(args)...);
    logger::error("{}: {}", name, VariableDumper::DumpVariable(var));
}

#pragma endregion

#pragma region SLTStackAnalyzer
class SLTStackAnalyzer {
public:
    struct AMEContextInfo {
        RE::ActiveEffect* ame;
        FrameContext* frame;
        ThreadContextHandle handle = 0;
        std::string initialScriptName;
        bool isValid = false;
        RE::VMStackID stackId;
        RE::Actor* target;
    };

    /**
     *  Consider "digging" back down the stack until we
     *  find an object that meets the requirements
     */
    static AMEContextInfo GetAMEContextInfo(RE::VMStackID stackId);

    static void Walk(RE::VMStackID stackId);
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

    // Separate execution control (different concern, different mutex)
    mutable std::mutex executionControlMutex;
    bool executionPaused = false;  // Transient - resets to false on load
    std::string pauseReason;       // Transient - for logging only

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

    // Execution control operations (separate mutex)
    bool ShouldContinueExecution() const {
        std::lock_guard<std::mutex> lock(executionControlMutex);
        return !executionPaused;
    }
    
    void PauseExecution(std::string_view reason) {
        std::lock_guard<std::mutex> lock(executionControlMutex);
        if (!executionPaused) {
            logger::info("Pausing script execution: {}", reason);
            executionPaused = true;
            pauseReason = reason;
        }
    }
    
    void ResumeExecution() {
        std::lock_guard<std::mutex> lock(executionControlMutex);
        if (executionPaused) {
            logger::info("Resuming script execution after: {}", pauseReason);
            executionPaused = false;
            pauseReason.clear();
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

    std::string ResolveValueVariable(FrameContext* frame, std::string_view token) const;
    bool ResolveFormVariable(FrameContext* frame, std::string_view token) const;

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
    
    std::size_t GetActiveContextCount() const {
        return ReadData([](const auto& targetContexts, const auto& activeContexts, const auto& globalVars, auto nextId) {
            return activeContexts.size();
        });
    }

    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    std::string DumpState();

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

    std::string DumpState();
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

    bool IsTargetValid() const {
        return target && target->tesTarget;
    }

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
    bool ExecuteNextStep(SLTStackAnalyzer::AMEContextInfo& contextInfo);

    std::string DumpState();
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

    // returns true if RunStep() can do something (i.e. this framecontext has an op ready to run); false if nothing else to do
    bool IsReady();

    // returns false when the FrameContext has nothing else to run
    bool RunStep(SLTStackAnalyzer::AMEContextInfo& contextInfo);

    std::string DumpState();
};
#pragma endregion
}