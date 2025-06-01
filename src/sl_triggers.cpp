#pragma push(warning)
#pragma warning(disable:4100)

#include "contexting.h"
#include "engine.h"

namespace SLT {

#pragma region SLTNativeFunctions declaration
class SLTNativeFunctions {
public:
static void SetLibrariesForExtensionAllowed(const RE::VMStackID stackID, std::string_view extensionKey, 
                                            bool allowed);

static bool PrepareContextForTargetedScript(const RE::VMStackID stackID, RE::Actor* targetActor, 
                                                        std::string_view scriptName);

static std::int32_t GetActiveScriptCount();

static SLTSessionId GetSessionId();

static bool IsLoaded();

static std::string GetTranslatedString(std::string_view input);

static std::vector<std::string> Tokenize(std::string_view input);

static bool DeleteTrigger(std::string_view extKeyStr, std::string_view trigKeyStr);

static RE::TESForm* GetForm(std::string_view a_editorID);

static bool SmartEquals(std::string_view a, std::string_view b);
                                
// These will attempt auto-contexting
static void Pung(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackId);

static bool ExecuteStepAndPending(RE::BSScript::Internal::VirtualMachine* vm, 
                                    const RE::VMStackID stackId);

static void CleanupThreadContext(RE::BSScript::Internal::VirtualMachine* vm, 
                                    const RE::VMStackID stackId);
};
#pragma endregion

#pragma region SLTPapyrusFunctionProvider

class SLTPapyrusFunctionProvider : public SLT::binding::PapyrusFunctionProvider<SLTPapyrusFunctionProvider> {
public:
    // Static Papyrus function implementations
    static void SetLibrariesForExtensionAllowed(PAPYRUS_STATIC_ARGS, std::string extensionKey, bool allowed) {
        SLT::SLTNativeFunctions::SetLibrariesForExtensionAllowed(StackID, extensionKey, allowed);
    }

    static bool PrepareContextForTargetedScript(PAPYRUS_STATIC_ARGS, RE::Actor* targetActor, std::string scriptname) {
        return SLT::SLTNativeFunctions::PrepareContextForTargetedScript(StackID, targetActor, scriptname);
    }

    static std::int32_t GetActiveScriptCount(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetActiveScriptCount();
    }

    static std::int32_t GetSessionId(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetSessionId();
    }

    static bool IsLoaded(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::IsLoaded();
    }

    static std::string GetTranslatedString(PAPYRUS_STATIC_ARGS, const std::string input) {
        return SLT::SLTNativeFunctions::GetTranslatedString(input);
    }

    static std::vector<std::string> Tokenize(PAPYRUS_STATIC_ARGS, std::string input) {
        return SLT::SLTNativeFunctions::Tokenize(input);
    }

    static bool DeleteTrigger(PAPYRUS_STATIC_ARGS, std::string extKeyStr, std::string trigKeyStr) {
        return SLT::SLTNativeFunctions::DeleteTrigger(extKeyStr, trigKeyStr);
    }

    static RE::TESForm* GetForm(PAPYRUS_STATIC_ARGS, std::string someFormOfFormIdentification) {
        return SLT::SLTNativeFunctions::GetForm(someFormOfFormIdentification);
    }

    static bool SmartEquals(PAPYRUS_STATIC_ARGS, std::string a, std::string b) {
        return SLT::SLTNativeFunctions::SmartEquals(a, b);
    }

    // These will perform auto-contexting
    static void Pung(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::Pung(VirtualMachine, StackID);
    }

    static bool ExecuteAndPending(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::ExecuteStepAndPending(VirtualMachine, StackID);
    }

    static void CleanupThreadContext(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::CleanupThreadContext(VirtualMachine, StackID);
    }

    void RegisterAllFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) {
        SLT::binding::PapyrusRegistrar<SLTPapyrusFunctionProvider> reg(vm, className);
        
        // Only pure utility functions get tasklet access
        reg.RegisterStatic("SmartEquals", &SLTPapyrusFunctionProvider::SmartEquals);
        reg.RegisterStatic("Tokenize", &SLTPapyrusFunctionProvider::Tokenize);
        reg.RegisterStatic("GetSessionId", &SLTPapyrusFunctionProvider::GetSessionId);
        reg.RegisterStatic("IsLoaded", &SLTPapyrusFunctionProvider::IsLoaded);
        
        // Everything else stays frame-synced for safety
        reg.RegisterStatic("SetLibrariesForExtensionAllowed", &SLTPapyrusFunctionProvider::SetLibrariesForExtensionAllowed);
        reg.RegisterStatic("PrepareContextForTargetedScript", &SLTPapyrusFunctionProvider::PrepareContextForTargetedScript);
        reg.RegisterStatic("GetActiveScriptCount", &SLTPapyrusFunctionProvider::GetActiveScriptCount);
        reg.RegisterStatic("GetTranslatedString", &SLTPapyrusFunctionProvider::GetTranslatedString);
        reg.RegisterStatic("DeleteTrigger", &SLTPapyrusFunctionProvider::DeleteTrigger);
        reg.RegisterStatic("GetForm", &SLTPapyrusFunctionProvider::GetForm);
        reg.RegisterStatic("Pung", &SLTPapyrusFunctionProvider::Pung);
        reg.RegisterStatic("ExecuteAndPending", &SLTPapyrusFunctionProvider::ExecuteAndPending);
        reg.RegisterStatic("CleanupThreadContext", &SLTPapyrusFunctionProvider::CleanupThreadContext);
    }
};

// Register the provider
REGISTER_PAPYRUS_PROVIDER(SLTPapyrusFunctionProvider, "sl_triggers_internal")

#pragma endregion

// SLTNativeFunctions implementation remains the same below

#pragma region SLTNativeFunctions definition
void SLTNativeFunctions::SetLibrariesForExtensionAllowed(const RE::VMStackID stackId,std::string_view extensionKey, 
                                        bool allowed) {
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    //SLTStackAnalyzer::Walk(stackId);
    if (funlib) {
        funlib->enabled = allowed;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, allowed);
    }
}

bool SLTNativeFunctions::PrepareContextForTargetedScript(const RE::VMStackID stackId, RE::Actor* targetActor, 
                                        std::string_view scriptName) {
    if (!targetActor || scriptName.empty()) {
        logger::error("Invalid parameters for PrepareContextForTargetedScript");
        return false;
    }
    
    //SLTStackAnalyzer::Walk(stackId);
    auto& manager = ContextManager::GetSingleton();
    return static_cast<std::int32_t>(manager.StartSLTScript(targetActor, scriptName));
}

std::int32_t SLTNativeFunctions::GetActiveScriptCount() {
    return static_cast<std::int32_t>(ContextManager::GetSingleton().GetActiveContextCount());
}
    
SLTSessionId SLTNativeFunctions::GetSessionId() {
    return SLT::GenerateNewSessionId();
}

bool SLTNativeFunctions::IsLoaded() {
    return true;
}

std::string SLTNativeFunctions::GetTranslatedString(std::string_view input) {
    auto sfmgr = RE::BSScaleformManager::GetSingleton();
    if (!(sfmgr)) {
        return std::string(input);
    }

    auto gfxloader = sfmgr->loader;
    if (!(gfxloader)) {
        return std::string(input);
    }

    auto translator =
        (RE::BSScaleformTranslator*) gfxloader->GetStateBagImpl()->GetStateAddRef(RE::GFxState::StateType::kTranslator);

    if (!(translator)) {
        return std::string(input);
    }

    RE::GFxTranslator::TranslateInfo transinfo;
    RE::GFxWStringBuffer result;

    std::wstring key_utf16 = stl::utf8_to_utf16(input).value_or(L""s);
    transinfo.key = key_utf16.c_str();

    transinfo.result = std::addressof(result);

    translator->Translate(std::addressof(transinfo));

    if (!result.empty()) {
        std::string actualresult = stl::utf16_to_utf8(result).value();
        return actualresult;
    }

    // Fallback: return original string if no translation found
    return std::string(input);
}

std::vector<std::string> SLTNativeFunctions::Tokenize(std::string_view input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool inBrackets = false;
    size_t i = 0;

    while (i < input.size()) {
        char c = input[i];

        if (!inQuotes && !inBrackets && c == ';') {
            // Comment detected — ignore rest of line
            break;
        }

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < input.size() && input[i + 1] == '"') {
                    current += '"';  // Escaped quote
                    i += 2;
                } else {
                    inQuotes = false;
                    tokens.push_back(current);
                    current.clear();
                    i++;
                }
            } else {
                current += c;
                i++;
            }
        } else if (inBrackets) {
            if (c == ']') {
                inBrackets = false;
                current = '[' + current + c;
                tokens.push_back(current);
                current.clear();
                i++;
            } else {
                current += c;
                i++;
            }
        } else {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                i++;
            } else if (c == '"') {
                inQuotes = true;
                i++;
            } else if (c == '[') {
                inBrackets = true;
                i++;
            } else {
                current += c;
                i++;
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }

    //logger::info("With input({}) Token count: {}", input, tokens.size());
    for (i = 0; i < tokens.size(); ++i) {
        //logger::info("Token {}: [{}]", i, tokens[i]);
    }
    return tokens;
}

bool SLTNativeFunctions::DeleteTrigger(std::string_view extKeyStr, std::string_view trigKeyStr) {
    if (!SystemUtil::File::IsValidPathComponent(extKeyStr) || !SystemUtil::File::IsValidPathComponent(trigKeyStr)) {
        logger::error("Invalid characters in extensionKey ({}) or triggerKey ({})", extKeyStr, trigKeyStr);
        return false;
    }

    if (extKeyStr.empty() || trigKeyStr.empty()) {
        logger::error("extensionKey and triggerKey may not be empty extensionKey[{}]  triggerKey[{}]", extKeyStr, trigKeyStr);
        return false;
    }

    // Ensure triggerKey ends with ".json"
    if (trigKeyStr.length() < 5 || trigKeyStr.substr(trigKeyStr.length() - 5) != ".json") {
        trigKeyStr = std::string(trigKeyStr) + std::string(".json");
    }

    fs::path filePath = SLT::GetPluginPath() / "extensions" / extKeyStr / trigKeyStr;

    std::error_code ec;

    if (!fs::exists(filePath, ec)) {
        logger::info("Trigger file not found: {}", filePath.string());
        return false;
    }

    if (fs::remove(filePath, ec)) {
        logger::info("Successfully deleted: {}", filePath.string());
        return true;
    } else {
        logger::info("Failed to delete {}: {}", filePath.string(), ec.message());
        return false;
    }
}

RE::TESForm* SLTNativeFunctions::GetForm(std::string_view a_editorID) {
    return FormUtil::Parse::GetForm(a_editorID);
}

bool SLTNativeFunctions::SmartEquals(std::string_view a, std::string_view b) {
    return SmartComparator::SmartEquals(std::string(a), std::string(b));
}

void SLTNativeFunctions::Pung(RE::BSScript::Internal::VirtualMachine* vm, 
                                const RE::VMStackID stackId) {
    if (!vm)
        return;
    
    //SLTStackAnalyzer::Walk(stackId);
    // technically this should be sufficient to populate the caller if it is possible
    std::ignore = SLTStackAnalyzer::GetAMEContextInfo(stackId);
}

bool SLTNativeFunctions::ExecuteStepAndPending(RE::BSScript::Internal::VirtualMachine* vm, 
                                const RE::VMStackID stackId) {
    if (!vm) return false;
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.contextId == 0) {
        logger::error("ExecuteStepAndPending called without valid threadContextHandle property");
        return false;
    }
    
    auto& manager = ContextManager::GetSingleton();
    auto* context = manager.GetContext(contextInfo.contextId);
    
    if (!context) {
        logger::error("No active context found for ID: {}", contextInfo.contextId);
        return false;
    }
    
    // Execute the step
    return context->ExecuteNextStep(contextInfo);
}

void SLTNativeFunctions::CleanupThreadContext(RE::BSScript::Internal::VirtualMachine* vm, 
                            const RE::VMStackID stackId) {
    if (!vm) return;
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.contextId == 0) {
        logger::error("CleanupThreadContext called without valid threadContextHandle property");
        return;
    }
    
    auto& manager = ContextManager::GetSingleton();
    manager.CleanupContext(contextInfo.contextId);
    
    logger::info("Cleaned up context {} from authenticated script call", contextInfo.contextId);
}

#pragma endregion

#pragma region Event registrations
OnDataLoaded([]{
    // Register serialization callbacks
    auto* serialization = SKSE::GetSerializationInterface();
    if (serialization) {
        serialization->SetSaveCallback([](SKSE::SerializationInterface* intfc) {
            if (intfc->OpenRecord('SLTD', 1)) {
                ContextManager::GetSingleton().Serialize(intfc);
            }
        });
        
        serialization->SetLoadCallback([](SKSE::SerializationInterface* intfc) {
            std::uint32_t type, version, length;
            while (intfc->GetNextRecordInfo(type, version, length)) {
                if (type == 'SLTD' && version == 1) {
                    ContextManager::GetSingleton().Deserialize(intfc);
                }
            }
        });
        
        serialization->SetRevertCallback([](SKSE::SerializationInterface*) {
            // Clear all contexts on new game
            ContextManager::GetSingleton().CleanupAllContexts();
        });
    }
});

OnNewGame([]{
    SLT::GenerateNewSessionId(true);
});

OnPreLoadGame([]{
    ContextManager::GetSingleton().PauseExecution("Game loading");
});

OnPostLoadGame([]{
    // Resume execution after load completes
    SLT::GenerateNewSessionId(true);
    ContextManager::GetSingleton().ResumeExecution();
    logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::SLTNativeFunctions::GetSessionId());
});

OnSaveGame([]{
    // Don't pause execution for saves during play
    // The coordination lock in ContextManager handles consistency during serialization
    logger::info("Save initiated - using coordination lock for consistency");
});

OnQuit([]{
    // Pause execution when game is quitting
    ContextManager::GetSingleton().PauseExecution("Game quitting");
});

#pragma endregion

};

#pragma pop()