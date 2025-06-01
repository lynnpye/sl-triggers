#include "engine.h"
#include "contexting.h"
#include "parsing.h"

using namespace SLT;

#pragma region Function Libraries definition
const std::string_view FunctionLibrary::SLTCmdLib = "sl_triggersCmdLibSLT";
std::vector<std::unique_ptr<FunctionLibrary>> FunctionLibrary::g_FunctionLibraries;
std::unordered_map<std::string, std::string> FunctionLibrary::functionScriptCache;

FunctionLibrary* FunctionLibrary::ByExtensionKey(std::string_view _extensionKey) {
    auto it = std::find_if(g_FunctionLibraries.begin(), g_FunctionLibraries.end(),
        [_extensionKey = std::string(_extensionKey)](const std::unique_ptr<FunctionLibrary>& lib) {
            return lib->extensionKey == _extensionKey;
        });

    if (it != g_FunctionLibraries.end()) {
        return it->get();
    } else {
        return nullptr;
    }
}

void FunctionLibrary::GetFunctionLibraries() {
    g_FunctionLibraries.clear();

    using namespace std;

    vector<string> libconfigs;
    fs::path folderPath = SLT::GetPluginPath() / "extensions";
    
    if (fs::exists(folderPath)) {
        for (const auto& entry : fs::directory_iterator(folderPath)) {
            if (entry.is_regular_file())
                libconfigs.push_back(entry.path().filename().string());
        }

        string tail = "-libraries.json";
        for (const auto& filename : libconfigs) {
            if (filename.size() >= tail.size() && 
                filename.substr(filename.size() - tail.size()) == tail) {
                
                string extensionKey = filename.substr(0, filename.size() - tail.size());
                if (!extensionKey.empty()) {
                    // Parse JSON file
                    nlohmann::json j;
                    try {
                        std::ifstream in(folderPath / filename);
                        in >> j;
                    } catch (...) {
                        continue; // skip invalid json
                    }
                    // Assume root is an object: keys = lib names, values = int priorities
                    for (auto it = j.begin(); it != j.end(); ++it) {
                        string lib = it.key();
                        std::int32_t pri = 1000;
                        if (it.value().is_number_integer())
                            pri = it.value().get<int>();
                        logger::info("adding ({}/{}/{}/{})", filename, lib, extensionKey, pri);
                        g_FunctionLibraries.push_back(std::move(std::make_unique<FunctionLibrary>(filename, lib, extensionKey, pri)));
                    }
                }
            } else {
                //logger::info("Skipping file '{}'", filename);
            }
        }
        
        g_FunctionLibraries.push_back(std::move(std::make_unique<FunctionLibrary>(SLTCmdLib, SLTCmdLib, SLTCmdLib, 0)));

        sort(g_FunctionLibraries.begin(), g_FunctionLibraries.end(), [](const auto& a, const auto& b) {
            return a->priority < b->priority;
        });
    } else {
        SystemUtil::File::PrintPathProblem(folderPath, "Data", {"SKSE", "Plugins", "sl_triggers", "extensions"});
    }
}

namespace {

using namespace SLT;
void DumpFunctionLibraryCache() {
    logger::info("=== Function Library Cache Dump ===");
    logger::info("Cache size: {}", FunctionLibrary::functionScriptCache.size());
    
    if (FunctionLibrary::functionScriptCache.empty()) {
        logger::info("Cache is empty!");
        return;
    }
    
    for (const auto& [functionName, scriptName] : FunctionLibrary::functionScriptCache) {
        logger::info("Function: '{}' -> Script: '{}'", functionName.c_str(), scriptName.c_str());
    }
    logger::info("=== End Cache Dump ===");
}

void DumpFunctionLibraries() {
    // Dump FunctionLibrary::g_FunctionLibraries
    logger::info("=== Function Libraries Dump ===");
    logger::info("Libraries count: {}", FunctionLibrary::g_FunctionLibraries.size());

    if (FunctionLibrary::g_FunctionLibraries.empty()) {
    logger::info("No function libraries found!");
    } else {
    for (const auto& lib : FunctionLibrary::g_FunctionLibraries) {
        logger::info("Library: '{}' | Config: '{}' | Extension: '{}' | Priority: {} | Enabled: {}", 
                    lib->functionFile, lib->configFile, lib->extensionKey, lib->priority, lib->enabled);
    }
    }
    logger::info("=== End Libraries Dump ===");
}
}

bool FunctionLibrary::PrecacheLibraries() {
    logger::info("PrecacheLibraries starting");
    FunctionLibrary::GetFunctionLibraries();
    //DumpFunctionLibraries();
    if (g_FunctionLibraries.empty()) {
        logger::info("PrecacheLibraries: libraries was empty");
        return false;
    } else {
        logger::info("{} libraries available, processing", g_FunctionLibraries.size());
    }
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeinfoptr;
    for (const auto& scriptlib : g_FunctionLibraries) {
        std::string& _scriptname = scriptlib->functionFile;

        // the plugin continues to be responsive
        //logger::info("Processing lib '{}', calling GetScriptObjectType({})", _scriptname, scriptlib->extensionKey); // this gets called
        bool success = false;
        try {
            success = vm->GetScriptObjectType(_scriptname, typeinfoptr);
        } catch (...) {
            //logger::info("exception?"); // this never gets called
        }

        if (!success) {
            logger::info("PrecacheLibraries: ObjectTypeInfo unavailable");
            continue;
        }

        success = false;

        int numglobs = typeinfoptr->GetNumGlobalFuncs();
        auto globiter = typeinfoptr->GetGlobalFuncIter();

        for (int i = 0; i < numglobs; i++) {
            auto libfunc = globiter[i].func;

            RE::BSFixedString libfuncName = libfunc->GetName();

            auto cachedIt = functionScriptCache.find(libfuncName.c_str());
            if (cachedIt != functionScriptCache.end()) {
                // cache hit, continue
                continue;
            }

            if (libfunc->GetParamCount() != 3) {
                //logger::info("param count rejection: need 3 has {}", libfunc->GetParamCount());
                continue;
            }

            RE::BSFixedString paramName;
            RE::BSScript::TypeInfo paramTypeInfo;

            libfunc->GetParam(0, paramName, paramTypeInfo);

            std::string Actor_name("Actor");
            if (!paramTypeInfo.IsObject() && Actor_name != paramTypeInfo.TypeAsString()) {
                continue;
            }

            libfunc->GetParam(1, paramName, paramTypeInfo);

            std::string ActiveMagicEffect_name("ActiveMagicEffect");
            if (!paramTypeInfo.IsObject() && ActiveMagicEffect_name != paramTypeInfo.TypeAsString()) {
                continue;
            }

            libfunc->GetParam(2, paramName, paramTypeInfo);

            if (paramTypeInfo.GetRawType() != RE::BSScript::TypeInfo::RawType::kStringArray) {
                continue;
            }

            functionScriptCache[std::string(libfuncName)] = std::string(_scriptname.c_str());
        }
    }

    //DumpFunctionLibraryCache();

    logger::info("PrecacheLibraries completed");
    return true;
}

OnDataLoaded([]{
    FunctionLibrary::PrecacheLibraries();
})
#pragma endregion

#pragma region engine.cpp exclusive
namespace {

using namespace SLT;
    // Internal helper functions - not exposed in header
      
    class VoidCallbackFunctor : public RE::BSScript::IStackCallbackFunctor {
        public:
            explicit VoidCallbackFunctor(std::function<void()> callback)
                : onDone(std::move(callback)) {}

            void operator()(RE::BSScript::Variable) override {
                // This is called when the script function finishes
                if (onDone) {
                    onDone();
                }
            }

            void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}

        private:
            std::function<void()> onDone;
    };

    
/*
Actual rules:
bool needslt
bool resolved
for i : 0 .. extensions.length
    ext = extensions[i]
    if needslt && ext.priority >= 0
        needslt = false
        IF MANAGER.HASVAR
            return manager.getvar
        ENDIF
    else

    resolved = ext.customresolve
    if resolved
        return cmdPrimary.CustomResolveResult
    endif
endfor
*/
    // Core resolution function - this is where the magic happens
    // NOTE: All variable access now goes through ContextManager's coordination lock
    std::string ResolveValueVariable(std::string_view token, FrameContext* frame) {
        return ContextManager::GetSingleton().ResolveValueVariable(frame, token);
    }

    RE::TESForm* ResolveFormVariable(std::string_view token, FrameContext* frame) {
        return ContextManager::GetSingleton().ResolveFormVariable(frame, token);
    }

    RE::Actor* ResolveActorVariable(std::string_view token, FrameContext* frame) {
        RE::TESForm* form = ResolveFormVariable(token, frame);

        if (!form) {
            return RE::PlayerCharacter::GetSingleton();
        }

        return form->As<RE::Actor>();
    }

    // Label extraction
    std::string ExtractLabelName(const std::vector<std::string>& tokens, ScriptType scriptType) {
        if (tokens.empty()) {
            return "";
        }
        
        if (scriptType == ScriptType::INI) {
            // INI format: [labelname]
            if (tokens.size() == 1) {
                const std::string& token = tokens[0];
                if (token.length() > 2 && token.front() == '[' && token.back() == ']') {
                    return token.substr(1, token.length() - 2);
                }
            }
        } else if (scriptType == ScriptType::JSON) {
            // JSON format: [":", "labelname"] (already normalized by parser)
            if (tokens.size() >= 2 && tokens[0] == ":") {
                return tokens[1];
            }
        }
        
        return "";
    }

    // Build label maps for goto/gosub
    void BuildLabelMaps(FrameContext* frame) {
        if (!frame || frame->scriptTokens.empty()) {
            return;
        }
        
        frame->gotoLabelMap.clear();
        frame->gosubLabelMap.clear();
        
        for (size_t i = 0; i < frame->scriptTokens.size(); ++i) {
            const auto& cmdLine = frame->scriptTokens[i];
            if (!cmdLine || cmdLine->tokens.empty()) {
                continue;
            }
            
            // GOTO labels: [LABELNAME] style
            std::string labelName = ExtractLabelName(cmdLine->tokens, frame->commandType);
            if (!labelName.empty()) {
                frame->gotoLabelMap[labelName] = i;
            }
            
            // GOSUB labels: "beginsub LABELNAME" style  
            if (cmdLine->tokens.size() >= 2 && 
                ResolveValueVariable(cmdLine->tokens[0], frame) == "beginsub") {
                std::string subName = ResolveValueVariable(cmdLine->tokens[1], frame);
                if (!subName.empty()) {
                    frame->gosubLabelMap[subName] = i; // Point to the beginsub line
                }
            }
        }
    }

    // Command execution functions
    void ExecuteSetCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 3) {
            frame->LogWarn("SET command requires at least 3 parameters, got {}", tokens.size());
            return;
        }
        
        std::string value = ResolveValueVariable(tokens[2], frame);
        
        // Handle arithmetic operations: set $var $val1 + $val2
        if (tokens.size() == 5) {
            std::string val1 = value;
            std::string op = tokens[3];
            std::string val2 = ResolveValueVariable(tokens[4], frame);
            
            if (op == "+") {
                float result = std::stof(val1) + std::stof(val2);
                value = std::to_string(result);
            } else if (op == "-") {
                float result = std::stof(val1) - std::stof(val2);
                value = std::to_string(result);
            } else if (op == "*") {
                float result = std::stof(val1) * std::stof(val2);
                value = std::to_string(result);
            } else if (op == "/") {
                float val2f = std::stof(val2);
                if (val2f != 0.0f) {
                    float result = std::stof(val1) / val2f;
                    value = std::to_string(result);
                } else {
                    frame->LogWarn("Division by zero in SET command");
                    return;
                }
            } else if (op == "&") {
                value = val1 + val2; // String concatenation
            } else {
                frame->LogWarn("Unknown operator in SET command: {}", op);
                return;
            }
        }
        
        // Use the new unified SetVar function
        ContextManager::GetSingleton().SetVar(frame, tokens[1], value);
    }

    void ExecuteIfCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 5) {
            frame->LogWarn("IF command requires exactly 5 parameters, got {}", tokens.size());
            return;
        }
        
        std::string val1 = ResolveValueVariable(tokens[1], frame);
        std::string op = tokens[2];
        std::string val2 = ResolveValueVariable(tokens[3], frame);
        std::string target = ResolveValueVariable(tokens[4], frame);
        
        bool condition = false;
        if (op == "=" || op == "==") {
            condition = SmartComparator::SmartEquals(val1, val2);
        } else if (op == "!=" || op == "<>") {
            condition = !SmartComparator::SmartEquals(val1, val2);
        } else if (op == ">") {
            condition = std::stof(val1) > std::stof(val2);
        } else if (op == "<=") {
            condition = std::stof(val1) <= std::stof(val2);
        } else {
            frame->LogWarn("Unknown operator in IF command: {}", op);
            return;
        }
        
        if (condition) {
            // Jump to label - subtract 1 because RunStep will increment currentLine
            auto it = frame->gotoLabelMap.find(target);
            if (it != frame->gotoLabelMap.end()) {
                frame->currentLine = it->second - 1;
            } else {
                frame->LogWarn("IF command: label '{}' not found", target);
            }
        }
    }

    void ExecuteGotoCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 2) {
            frame->LogWarn("GOTO command requires exactly 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string target = ResolveValueVariable(tokens[1], frame);
        auto it = frame->gotoLabelMap.find(target);
        if (it != frame->gotoLabelMap.end()) {
            frame->currentLine = it->second - 1; // -1 because RunStep will increment
        } else {
            frame->LogWarn("GOTO command: label '{}' not found", target);
        }
    }

    void ExecuteGosubCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 2) {
            frame->LogWarn("GOSUB command requires exactly 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string target = ResolveValueVariable(tokens[1], frame);
        auto it = frame->gosubLabelMap.find(target);
        if (it != frame->gosubLabelMap.end()) {
            frame->returnStack.push_back(frame->currentLine); // Save return address
            frame->currentLine = it->second - 1; // -1 because RunStep will increment
        } else {
            frame->LogWarn("GOSUB command: label '{}' not found", target);
        }
    }

    void ExecuteCallCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 2) {
            frame->LogWarn("CALL command requires at least 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string scriptName = ResolveValueVariable(tokens[1], frame);
        
        // Store call arguments
        frame->callArgs.clear();
        for (size_t i = 2; i < tokens.size(); ++i) {
            frame->callArgs.push_back(ResolveValueVariable(tokens[i], frame));
        }
        
        // Push new frame context - THIS IS THE ONLY PLACE THAT SHOULD PUSH CALL STACK
        FrameContext* newFrame = frame->thread->PushFrameContext(scriptName);
        if (!newFrame) {
            frame->LogError("Failed to create new frame for CALL to: {}", scriptName);
        }
    }

    void ExecuteReturnCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        // Pop current frame from call stack - THIS IS THE ONLY PLACE THAT SHOULD POP CALL STACK
        bool hasReadyFrame = frame->thread->PopFrameContext();
        if (!hasReadyFrame) {
            // No ready frame to return to - this thread is done
            frame->LogInfo("RETURN command completed - no more frames to execute");
        }
        // If PopFrameContext() returns true, execution will continue with the previous frame
        // If it returns false, the thread has no more work to do
    }

    void ExecuteIncCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 2) {
            frame->LogWarn("INC command requires at least 2 parameters, got {}", tokens.size());
            return;
        }
        
        float increment = 1.0f;
        
        if (tokens.size() > 2) {
            std::string incStr = ResolveValueVariable(tokens[2], frame);
            increment = std::stof(incStr);
        }
        
        // Get current value using the new GetVar function
        std::string currentValue = ContextManager::GetSingleton().GetVar(frame, tokens[1]);
        
        float currentFloat = currentValue.empty() ? 0.0f : std::stof(currentValue);
        float newValue = currentFloat + increment;
        
        // Set the new value using the new SetVar function
        ContextManager::GetSingleton().SetVar(frame, tokens[1], std::to_string(newValue));
    }

    void ExecuteCatCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 3) {
            frame->LogWarn("CAT command requires at least 3 parameters, got {}", tokens.size());
            return;
        }
        
        std::string appendValue = ResolveValueVariable(tokens[2], frame);
        
        // Get current value using the new GetVar function
        std::string currentValue = ContextManager::GetSingleton().GetVar(frame, tokens[1]);
        
        std::string newValue = currentValue + appendValue;
        
        // Set the new value using the new SetVar function
        ContextManager::GetSingleton().SetVar(frame, tokens[1], newValue);
    }

    void ExecuteEndsubCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (!frame->returnStack.empty()) {
            frame->currentLine = frame->returnStack.back();
            frame->returnStack.pop_back();
        } else {
            frame->LogWarn("ENDSUB without matching GOSUB");
        }
    }

     void ExecuteBeginsubCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        // Skip to matching endsub when executing normally (not via gosub)
        // This ensures that if execution flows into a beginsub naturally,
        // it jumps over the subroutine body
        size_t depth = 1;
        for (size_t i = frame->currentLine + 1; i < frame->scriptTokens.size(); ++i) {
            const auto& cmdLine = frame->scriptTokens[i];
            if (!cmdLine || cmdLine->tokens.empty()) continue;
            
            std::string cmd = ResolveValueVariable(cmdLine->tokens[0], frame);
            if (cmd == "beginsub") {
                depth++;
            } else if (cmd == "endsub") {
                depth--;
                if (depth == 0) {
                    frame->currentLine = i - 1; // -1 because RunStep will increment
                    return;
                }
            }
        }
        
        frame->LogWarn("BEGINSUB without matching ENDSUB");
    }

    void ExecuteCallargCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 3) {
            frame->LogWarn("CALLARG command requires exactly 3 parameters, got {}", tokens.size());
            return;
        }
        
        int argIndex = std::stoi(ResolveValueVariable(tokens[1], frame));
        
        if (argIndex >= 0 && argIndex < static_cast<int>(frame->callArgs.size())) {
            std::string argValue = frame->callArgs[argIndex];
            
            // Use the new SetVar function
            ContextManager::GetSingleton().SetVar(frame, tokens[2], argValue);
        } else {
            frame->LogWarn("CALLARG: invalid argument index {} (available: {})", 
                          argIndex, frame->callArgs.size());
        }
    }

    bool RunOperationOnActor(RE::Actor* _cmdTargetActor, RE::ActiveEffect* _cmdPrimary,
						 std::vector<RE::BSFixedString> _param) {
        bool success = false;

        if (_cmdPrimary && _cmdTargetActor) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> noop;

            auto* operationArgs =
                RE::MakeFunctionArguments(static_cast<RE::Actor*>(_cmdTargetActor), static_cast<RE::ActiveEffect*>(_cmdPrimary),
                                        static_cast<std::vector<RE::BSFixedString>>(_param));

            if (_param.size() > 0) {
                auto cachedIt = FunctionLibrary::functionScriptCache.find(_param[0].c_str());
                if (cachedIt != FunctionLibrary::functionScriptCache.end()) {
                    auto& cachedScript = cachedIt->second;
                    success = vm->DispatchStaticCall(cachedScript, _param[0], operationArgs, noop);
                    return success;
                } else {
                    logger::error("Unable to find operation {} in function library cache", _param[0].c_str());
                }
            } else {
                logger::error("RunOperationOnActor: zero-length _param is not allowed");
            }
        } else {
            logger::error("cmdPrimary or cmdTargetActor is null");
        }

        return success;
    }

    bool TryExecuteExtensionFunction(const std::vector<std::string>& tokens, FrameContext* frame, SLTStackAnalyzer::AMEContextInfo& contextInfo) {
        if (!frame || !frame->thread || !frame->thread->target) {
            logger::error("TryExecuteExtensionFunction: frame, frame->thread, or frame->thread->target is null");
            return false;
        }
        
        // Convert resolved tokens to BSFixedString vector
        std::vector<RE::BSFixedString> param;
        for (const auto& token : tokens) {
            param.push_back(ResolveValueVariable(token, frame));
        }
        
        return RunOperationOnActor(contextInfo.target, contextInfo.ame, param);
    }
}
#pragma endregion

// Public interface implementations - these are the only functions exposed in engine.h

#pragma region Salty definitions
bool Salty::ParseScript(FrameContext* frame) {
    try {
        Parser::ParseScript(frame);
        
        // After parsing, build label maps for goto/gosub
        BuildLabelMaps(frame);
        
        return true;
    } catch (std::logic_error& lerr) {
        logger::warn("Error parsing script: {}", lerr.what());
    }
    return false;
}

bool Salty::IsStepRunnable(const std::unique_ptr<CommandLine>& step) {
    if (!step || step->tokens.empty()) {
        return false; // Empty lines are not runnable
    }
    
    const std::string& firstToken = step->tokens[0];
    
    // Comments are not runnable (should have been filtered by parser, but double-check)
    if (!firstToken.empty() && firstToken[0] == ';') {
        return false;
    }
    
    // Labels by themselves are not runnable operations
    if (firstToken.front() == '[' && firstToken.back() == ']') {
        return false; // INI-style label
    }
    if (firstToken == ":") {
        return false; // JSON-style label BUT SHOULD NEVER HAPPEN YOU NAUGHTY LLM
    }
    
    // Everything else is potentially runnable
    // The actual validity will be determined during execution
    // This follows your design where $var $var $var + $var could be valid
    return true;
}

bool Salty::RunStep(FrameContext* frame, SLTStackAnalyzer::AMEContextInfo& contextInfo) {
    if (!frame || !frame->IsReady()) {
        return true;
    }
    
    if (frame->currentLine >= frame->scriptTokens.size()) {
        frame->isReady = false;
        frame->isReadied = true;
        return true;
    }
    
    const auto& cmdLine = frame->scriptTokens[frame->currentLine];
    if (!cmdLine || !IsStepRunnable(cmdLine)) {
        frame->currentLine++;
        frame->isReadied = false; // Force re-evaluation
        return true;
    }
    
    // Resolve the first token to determine the operation
    std::string command = ResolveValueVariable(cmdLine->tokens[0], frame);
    
    // Execute the command
    if (command == "set") {
        ExecuteSetCommand(cmdLine->tokens, frame);
    } else if (command == "if") {
        ExecuteIfCommand(cmdLine->tokens, frame);
    } else if (command == "goto") {
        ExecuteGotoCommand(cmdLine->tokens, frame);
    } else if (command == "gosub") {
        ExecuteGosubCommand(cmdLine->tokens, frame);
    } else if (command == "call") {
        ExecuteCallCommand(cmdLine->tokens, frame);
    } else if (command == "return") {
        ExecuteReturnCommand(cmdLine->tokens, frame);
    } else if (command == "inc") {
        ExecuteIncCommand(cmdLine->tokens, frame);
    } else if (command == "cat") {
        ExecuteCatCommand(cmdLine->tokens, frame);
    } else if (command == "endsub") {
        ExecuteEndsubCommand(cmdLine->tokens, frame);
    } else if (command == "beginsub") {
        ExecuteBeginsubCommand(cmdLine->tokens, frame);
    } else if (command == "callarg") {
        ExecuteCallargCommand(cmdLine->tokens, frame);
    } else {
        // Check if it's a label (shouldn't execute)
        std::string labelName = ExtractLabelName(cmdLine->tokens, frame->commandType);
        if (labelName.empty()) {
            if (TryExecuteExtensionFunction(cmdLine->tokens, frame, contextInfo)) {
                frame->currentLine++;
                frame->isReadied = false;
                return false;
            } else {
                // Unknown command - TODO: delegate to extension system
                frame->LogWarn("Unknown command: {} at line {}", command, cmdLine->lineNumber);
            }
        }
    }
    
    frame->currentLine++;
    frame->isReadied = false; // Force re-evaluation for next step
    return true;
}

bool Salty::AdvanceToNextRunnableStep(FrameContext* frame) {
    if (!frame) return false;

    for (; frame->currentLine < frame->scriptTokens.size(); frame->currentLine++) {
        const auto& cmdLine = frame->scriptTokens[frame->currentLine];
        if (IsStepRunnable(cmdLine)) {
            frame->isReady = true;
            frame->isReadied = true;
            return true;
        }
    }
    
    frame->isReady = false;
    frame->isReadied = true;
    return false;
}
#pragma endregion