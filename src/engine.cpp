#include "engine.h"
#include "contexting.h"
#include "parsing.h"
#include "questor.h"
#include "coroutines.h"

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

OnAfterSKSEInit([]{
    fs::path debugmsg_log = GetPluginPath() / "debugmsg.log";
    std::ofstream file(debugmsg_log, std::ios::trunc);
});

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
    
    // Core resolution function - this is where the magic happens
    // NOTE: All variable access now goes through ContextManager's coordination lock
    std::string ResolveValueVariable(std::string_view token, FrameContext* frame) {
        return ContextManager::GetSingleton().ResolveValueVariable(frame, token);
    }

    RE::Actor* ResolveActorVariable(std::string_view token, FrameContext* frame) {
        RE::TESForm* form = Salty::ResolveFormVariable(token, frame);

        if (!form) {
            return RE::PlayerCharacter::GetSingleton();
        }

        return form->As<RE::Actor>();
    }

    bool TryFunctionLibrary(const std::vector<std::string>& tokens, FrameContext* frame, RE::Actor* targetActor, RE::ActiveEffect* cmdPrimary) {
        if (!frame || !frame->thread || !frame->thread->target) {
            logger::error("TryFunctionLibrary: frame, frame->thread, or frame->thread->target is null");
            return false;
        }

        logger::debug("TryFunctionLibrary({})", str::Join(tokens, "), ("));
        
        // Convert resolved tokens to BSFixedString vector
        std::vector<RE::BSFixedString> param;
        for (const auto& token : tokens) {
            param.push_back(ResolveValueVariable(token, frame));
        }
        
        return FormResolver::RunOperationOnActor(frame, targetActor, cmdPrimary, param);
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
                str::iEquals(cmdLine->tokens[0], "beginsub")) {
                std::string subName = cmdLine->tokens[1];
                if (!subName.empty()) {
                    logger::debug("gosub label added [{}]({})", subName, i);
                    frame->gosubLabelMap[subName] = i; // Point to the beginsub line
                }
            }
        }
    }

    // Command execution functions
    void ExecuteSetCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 3) {
            FrameLogWarn(frame, "SET command requires at least 3 parameters, got {}", tokens.size());
            return;
        }
        
        std::string value = ResolveValueVariable(tokens[2], frame);
        
        // Handle arithmetic operations: set $var $val1 + $val2
        if (tokens.size() > 3 && str::iEquals(value, "resultfrom")) {
            std::string subcode = ResolveValueVariable(tokens[3], frame);
            if (!subcode.empty()) {
                std::vector<std::string> subcodeTokens(
                    tokens.size() > 3 ? tokens.begin() + 3 : tokens.end(),
                    tokens.end()
                );
                subcodeTokens[0] = subcode;

                if (TryFunctionLibrary(subcodeTokens, frame, frame->thread->target->AsActor(), frame->thread->ame)) {
                    value = frame->mostRecentResult;
                }
            } else {
                logger::error("Unable to resolve function for 'set resultfrom' with ({})", tokens[3]);
                return;
            }
        } else if (tokens.size() == 3) {
        } else if (tokens.size() == 5) {
            std::string val1 = value;
            std::string op = tokens[3];
            std::string val2 = ResolveValueVariable(tokens[4], frame);
            float _flt1 = 0.0f;
            float _flt2 = 0.0f;
            
            if (op == "+") {
                _flt1 = str::TryToFloat(val1);
                _flt2 = str::TryToFloat(val2);
                float result = _flt1 + _flt2;
                value = std::to_string(result);
            } else if (op == "-") {
                _flt1 = str::TryToFloat(val1);
                _flt2 = str::TryToFloat(val2);
                float result = _flt1 - _flt2;
                value = std::to_string(result);
            } else if (op == "*") {
                _flt1 = str::TryToFloat(val1);
                _flt2 = str::TryToFloat(val2);
                float result = _flt1 * _flt2;
                value = std::to_string(result);
            } else if (op == "/") {
                _flt1 = str::TryToFloat(val1);
                _flt2 = str::TryToFloat(val2);
                if (_flt2 != 0.0f) {
                    float result = _flt1 / _flt2;
                    value = std::to_string(result);
                } else {
                    FrameLogWarn(frame, "Division by zero in SET command");
                    return;
                }
            } else if (op == "&") {
                value = val1 + val2; // String concatenation
            } else {
                FrameLogWarn(frame, "Unknown operator in SET command: {}", op);
                return;
            }
        }
        
        // Use the new unified SetVar function
        ContextManager::GetSingleton().SetVar(frame, tokens[1], value);
    }

    void ExecuteIfCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 5) {
            FrameLogWarn(frame, "IF command requires exactly 5 parameters, got {}", tokens.size());
            return;
        }
        
        std::string val1 = ResolveValueVariable(tokens[1], frame);
        std::string op = tokens[2];
        std::string val2 = ResolveValueVariable(tokens[3], frame);
        std::string target = ResolveValueVariable(tokens[4], frame);
        float _flt1 = 0.0f;
        float _flt2 = 0.0f;
        
        bool condition = false;
        if (op == "=" || op == "==" || op == "&=") {
            condition = SmartComparator::SmartEquals(val1, val2);
        } else if (op == "!=" || op == "<>" || op == "&!=") {
            condition = !SmartComparator::SmartEquals(val1, val2);
        } else if (op == ">") {
            _flt1 = str::TryToFloat(val1);
            _flt2 = str::TryToFloat(val2);
            condition = _flt1 > _flt2;
        } else if (op == ">=") {
            _flt1 = str::TryToFloat(val1);
            _flt2 = str::TryToFloat(val2);
            condition = _flt1 >= _flt2;
        } else if (op == "<") {
            _flt1 = str::TryToFloat(val1);
            _flt2 = str::TryToFloat(val2);
            condition = _flt1 < _flt2;
        } else if (op == "<=") {
            _flt1 = str::TryToFloat(val1);
            _flt2 = str::TryToFloat(val2);
            condition = _flt1 <= _flt2;
        } else {
            FrameLogWarn(frame, "Unknown operator in IF command: {}", op);
            return;
        }
        
        if (condition) {
            // Jump to label - subtract 1 because RunStep will increment currentLine
            auto it = frame->gotoLabelMap.find(target);
            if (it != frame->gotoLabelMap.end()) {
                frame->currentLine = it->second - 1;
            } else {
                FrameLogWarn(frame, "IF command: label '{}' not found", target);
            }
        }
    }

    void ExecuteGotoCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 2) {
            FrameLogWarn(frame, "GOTO command requires exactly 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string target = ResolveValueVariable(tokens[1], frame);
        auto it = frame->gotoLabelMap.find(target);
        if (it != frame->gotoLabelMap.end()) {
            frame->currentLine = it->second; // -1 because RunStep will increment
        } else {
            FrameLogWarn(frame, "GOTO command: label '{}' not found", target);
        }
    }

    void ExecuteGosubCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 2) {
            FrameLogWarn(frame, "GOSUB command requires exactly 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string target = ResolveValueVariable(tokens[1], frame);
        auto it = frame->gosubLabelMap.find(target);
        if (it != frame->gosubLabelMap.end()) {
            logger::debug("gosub pushing back line({}) and switching to line({})", frame->currentLine, it->second - 1);
            frame->returnStack.push_back(frame->currentLine); // Save return address
            frame->currentLine = it->second; // -1 because RunStep will increment
        } else {
            FrameLogWarn(frame, "GOSUB command: label '{}' not found", target);
        }
    }

    void ExecuteCallCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 2) {
            FrameLogWarn(frame, "CALL command requires at least 2 parameters, got {}", tokens.size());
            return;
        }
        
        std::string scriptName = ResolveValueVariable(tokens[1], frame);
        
        // Push new frame context - THIS IS THE ONLY PLACE THAT SHOULD PUSH CALL STACK
        FrameContext* newFrame = frame->thread->PushFrameContext(scriptName);
        if (!newFrame) {
            FrameLogError(frame, "Failed to create new frame for CALL to: {}", scriptName);
        }
        else {
            for (size_t i = 2; i < tokens.size(); ++i) {
                newFrame->callArgs.push_back(ResolveValueVariable(tokens[i], frame));
            }
            newFrame->IsReady();
        }
    }

    void ExecuteReturnCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        // Pop current frame from call stack - THIS IS THE ONLY PLACE THAT SHOULD POP CALL STACK
        frame->currentLine = frame->scriptTokens.size(); // signal we are done
        // If PopFrameContext() returns true, execution will continue with the previous frame
        // If it returns false, the thread has no more work to do
    }

    void ExecuteIncCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() < 2) {
            FrameLogWarn(frame, "INC command requires at least 2 parameters, got {}", tokens.size());
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
            FrameLogWarn(frame, "CAT command requires at least 3 parameters, got {}", tokens.size());
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
            FrameLogWarn(frame, "ENDSUB without matching GOSUB");
        }
    }

     void ExecuteBeginsubCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        // Skip to matching endsub when executing normally (not via gosub)
        // This ensures that if execution flows into a beginsub naturally,
        // it jumps over the subroutine body
        for (size_t i = frame->currentLine + 1; i < frame->scriptTokens.size(); ++i) {
            const auto& cmdLine = frame->scriptTokens[i];
            if (!cmdLine || cmdLine->tokens.empty()) continue;
            
            std::string cmd = cmdLine->tokens[0];
            if (str::iEquals(cmd, "endsub")) {
                frame->currentLine = i; // -1 because RunStep will increment
                return;
            }
        }
        
        FrameLogWarn(frame, "BEGINSUB without matching ENDSUB");
    }

    void ExecuteCallargCommand(const std::vector<std::string>& tokens, FrameContext* frame) {
        if (tokens.size() != 3) {
            FrameLogWarn(frame, "CALLARG command requires exactly 3 parameters, got {}", tokens.size());
            return;
        }
        
        int argIndex = std::stoi(ResolveValueVariable(tokens[1], frame));
        
        if (argIndex >= 0 && argIndex < static_cast<int>(frame->callArgs.size())) {
            std::string argValue = frame->callArgs[argIndex];
            
            // Use the new SetVar function
            ContextManager::GetSingleton().SetVar(frame, tokens[2], argValue);
        } else {
            FrameLogWarn(frame, "CALLARG: invalid argument index {} (available: {})", 
                          argIndex, frame->callArgs.size());
        }
    }

}

#pragma endregion

// Public interface implementations - these are the only functions exposed in engine.h

#pragma region Salty definitions

RE::TESForm* Salty::ResolveFormVariable(std::string_view token, FrameContext* frame) {
    if (ContextManager::GetSingleton().ResolveFormVariable(frame, token)) {
        return frame->customResolveFormResult;
    }
    
    return nullptr;
}

bool Salty::ParseScript(FrameContext* frame) {
    try {
        auto parseResult = Parser::ParseScript(frame);
        
        if (parseResult == ParseResult::Success) {
            // After parsing, build label maps for goto/gosub
            BuildLabelMaps(frame);
            return true;
        }
        return false;
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
    if (str::iEquals(command, "set")) {
        ExecuteSetCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "if")) {
        ExecuteIfCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "goto")) {
        ExecuteGotoCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "gosub")) {
        ExecuteGosubCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "call")) {
        ExecuteCallCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "return")) {
        ExecuteReturnCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "inc")) {
        ExecuteIncCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "cat")) {
        ExecuteCatCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "endsub")) {
        ExecuteEndsubCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "beginsub")) {
        ExecuteBeginsubCommand(cmdLine->tokens, frame);
    } else if (str::iEquals(command, "callarg")) {
        ExecuteCallargCommand(cmdLine->tokens, frame);
    } else {
        // Check if it's a label (shouldn't execute)
        std::string labelName = ExtractLabelName(cmdLine->tokens, frame->commandType);
        if (labelName.empty()) {
            if (TryFunctionLibrary(cmdLine->tokens, frame, contextInfo.target, contextInfo.ame)) {
                frame->currentLine++;
                frame->isReadied = false;
                return false;
            } else {
                // Unknown command - TODO: delegate to extension system
                FrameLogWarn(frame, "Unknown command: {} at line {}", command, cmdLine->lineNumber);
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