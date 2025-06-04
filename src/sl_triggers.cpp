#pragma push(warning)
#pragma warning(disable:4100)

#include "contexting.h"
#include "engine.h"
#include "coroutines.h"
#include "questor.h"
#include "papyrus_conversion.h"
#include "modevent.h"

namespace SLT {

#pragma region SLTNativeFunctions declaration
class SLTNativeFunctions {
public:
static void SetCustomResolveFormResult(PAPYRUS_NATIVE_DECL, ThreadContextHandle threadContextHandle,
                                            RE::TESForm* resultingForm);

static void SetLibrariesForExtensionAllowed(PAPYRUS_NATIVE_DECL, std::string_view extensionKey, 
                                            bool allowed);

static bool PrepareContextForTargetedScript(PAPYRUS_NATIVE_DECL, RE::Actor* targetActor, 
                                                        std::string_view scriptName);

static std::int32_t GetActiveScriptCount(PAPYRUS_NATIVE_DECL);

static SLTSessionId GetSessionId(PAPYRUS_NATIVE_DECL);

static bool IsLoaded(PAPYRUS_NATIVE_DECL);

static std::string GetTranslatedString(PAPYRUS_NATIVE_DECL, std::string_view input);

static std::vector<std::string> Tokenize(PAPYRUS_NATIVE_DECL, std::string_view input);

static bool DeleteTrigger(PAPYRUS_NATIVE_DECL, std::string_view extKeyStr, std::string_view trigKeyStr);

static RE::TESForm* GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID);

static bool SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b);

static void WalkTheStack(PAPYRUS_NATIVE_DECL);
                                
// These will attempt auto-contexting
static void Pung(PAPYRUS_NATIVE_DECL);

static RE::BSScript::LatentStatus ExecuteStepAndPending(PAPYRUS_NATIVE_DECL);

static void CleanupThreadContext(PAPYRUS_NATIVE_DECL);

static std::string ResolveValueVariable(PAPYRUS_NATIVE_DECL, std::string_view variableName);

static RE::TESForm* ResolveFormVariable(PAPYRUS_NATIVE_DECL, std::string_view variableName);

static std::vector<std::string> GetScriptsList(PAPYRUS_NATIVE_DECL);

static std::vector<std::string> GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey);

static RE::TESQuest* MainQuest(PAPYRUS_NATIVE_DECL);

static void RegisterExtension(PAPYRUS_NATIVE_DECL, RE::TESQuest* extensionQuest);
};
#pragma endregion

#pragma region SLTPapyrusFunctionProvider

class SLTPapyrusFunctionProvider : public SLT::binding::PapyrusFunctionProvider<SLTPapyrusFunctionProvider> {
public:
    // Static Papyrus function implementations
    static std::int32_t GetActiveScriptCount(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetActiveScriptCount(PAPYRUS_FN_PARMS);
    }

    static RE::TESForm* GetForm(PAPYRUS_STATIC_ARGS, std::string someFormOfFormIdentification) {
        return SLT::SLTNativeFunctions::GetForm(PAPYRUS_FN_PARMS, someFormOfFormIdentification);
    }

    static std::vector<std::string> GetScriptsList(PAPYRUS_STATIC_ARGS, std::string_view input) {
        return SLT::SLTNativeFunctions::GetScriptsList(PAPYRUS_FN_PARMS);
    }

    static std::int32_t GetSessionId(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetSessionId(PAPYRUS_FN_PARMS);
    }

    static std::string GetTranslatedString(PAPYRUS_STATIC_ARGS, const std::string input) {
        return SLT::SLTNativeFunctions::GetTranslatedString(PAPYRUS_FN_PARMS, input);
    }

    static bool IsLoaded(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::IsLoaded(PAPYRUS_FN_PARMS);
    }

    static RE::TESQuest* MainQuest(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::MainQuest(PAPYRUS_FN_PARMS);
    }

    static bool SmartEquals(PAPYRUS_STATIC_ARGS, std::string a, std::string b) {
        return SLT::SLTNativeFunctions::SmartEquals(PAPYRUS_FN_PARMS, a, b);
    }

    static std::vector<std::string> Tokenize(PAPYRUS_STATIC_ARGS, std::string input) {
        return SLT::SLTNativeFunctions::Tokenize(PAPYRUS_FN_PARMS, input);
    }

    void RegisterAllFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) {
        SLT::binding::PapyrusRegistrar<SLTPapyrusFunctionProvider> reg(vm, className);
        
        reg.RegisterStatic("GetActiveScriptCount", &SLTPapyrusFunctionProvider::GetActiveScriptCount);
        reg.RegisterStatic("GetForm", &SLTPapyrusFunctionProvider::GetForm);
        reg.RegisterStatic("GetScriptsList", &SLTPapyrusFunctionProvider::GetScriptsList);
        reg.RegisterStatic("GetSessionId", &SLTPapyrusFunctionProvider::GetSessionId);
        reg.RegisterStatic("GetTranslatedString", &SLTPapyrusFunctionProvider::GetTranslatedString);
        reg.RegisterStatic("IsLoaded", &SLTPapyrusFunctionProvider::IsLoaded);
        reg.RegisterStatic("MainQuest", &SLTPapyrusFunctionProvider::MainQuest);
        reg.RegisterStatic("SmartEquals", &SLTPapyrusFunctionProvider::SmartEquals);
        reg.RegisterStatic("Tokenize", &SLTPapyrusFunctionProvider::Tokenize);
    }
};

// Register the provider
REGISTER_PAPYRUS_PROVIDER(SLTPapyrusFunctionProvider, "sl_triggers")

#pragma endregion

#pragma region SLTInternalPapyrusFunctionProvider
class SLTInternalPapyrusFunctionProvider : public SLT::binding::PapyrusFunctionProvider<SLTInternalPapyrusFunctionProvider> {
public:
    // Static Papyrus function implementations
    // LATENT
    static RE::BSScript::LatentStatus ExecuteAndPending(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::ExecuteStepAndPending(PAPYRUS_FN_PARMS);
    }

    // NON-LATENT
    static void CleanupThreadContext(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::CleanupThreadContext(PAPYRUS_FN_PARMS);
    }

    static bool DeleteTrigger(PAPYRUS_STATIC_ARGS, std::string extKeyStr, std::string trigKeyStr) {
        return SLT::SLTNativeFunctions::DeleteTrigger(PAPYRUS_FN_PARMS, extKeyStr, trigKeyStr);
    }

    static std::vector<std::string> GetTriggerKeys(PAPYRUS_STATIC_ARGS, std::string_view extensionKey) {
        return SLT::SLTNativeFunctions::GetTriggerKeys(PAPYRUS_FN_PARMS, extensionKey);
    }

    static bool PrepareContextForTargetedScript(PAPYRUS_STATIC_ARGS, RE::Actor* targetActor, std::string scriptname) {
        return SLT::SLTNativeFunctions::PrepareContextForTargetedScript(PAPYRUS_FN_PARMS, targetActor, scriptname);
    }

    static void Pung(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::Pung(PAPYRUS_FN_PARMS);
    }

    static void RegisterExtension(PAPYRUS_STATIC_ARGS, RE::TESQuest* extensionQuest) {
        SLT::SLTNativeFunctions::RegisterExtension(PAPYRUS_FN_PARMS, extensionQuest);
    }

    static RE::TESForm* ResolveFormVariable(PAPYRUS_STATIC_ARGS, std::string_view variableName) {
        return SLT::SLTNativeFunctions::ResolveFormVariable(PAPYRUS_FN_PARMS, variableName);
    }

    static std::string ResolveValueVariable(PAPYRUS_STATIC_ARGS, std::string_view variableName) {
        return SLT::SLTNativeFunctions::ResolveValueVariable(PAPYRUS_FN_PARMS, variableName);
    }

    static void SetCustomResolveFormResult(PAPYRUS_STATIC_ARGS, std::int32_t threadContextHandle, RE::TESForm* resultingForm) {
        SLT::SLTNativeFunctions::SetCustomResolveFormResult(PAPYRUS_FN_PARMS, threadContextHandle, resultingForm);
    }

    static void SetLibrariesForExtensionAllowed(PAPYRUS_STATIC_ARGS, std::string extensionKey, bool allowed) {
        SLT::SLTNativeFunctions::SetLibrariesForExtensionAllowed(PAPYRUS_FN_PARMS, extensionKey, allowed);
    }

    static void WalkTheStack(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::WalkTheStack(PAPYRUS_FN_PARMS);
    }

    void RegisterAllFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) {
        SLT::binding::PapyrusRegistrar<SLTInternalPapyrusFunctionProvider> reg(vm, className);
        
        reg.RegisterStaticLatent<bool>("ExecuteAndPending", &SLTInternalPapyrusFunctionProvider::ExecuteAndPending);

        reg.RegisterStatic("CleanupThreadContext", &SLTInternalPapyrusFunctionProvider::CleanupThreadContext);
        reg.RegisterStatic("DeleteTrigger", &SLTInternalPapyrusFunctionProvider::DeleteTrigger);
        reg.RegisterStatic("GetTriggerKeys", &SLTInternalPapyrusFunctionProvider::GetTriggerKeys);
        reg.RegisterStatic("PrepareContextForTargetedScript", &SLTInternalPapyrusFunctionProvider::PrepareContextForTargetedScript);
        reg.RegisterStatic("Pung", &SLTInternalPapyrusFunctionProvider::Pung);
        reg.RegisterStatic("RegisterExtension", &SLTInternalPapyrusFunctionProvider::RegisterExtension);
        reg.RegisterStatic("ResolveFormVariable", &SLTInternalPapyrusFunctionProvider::ResolveFormVariable);
        reg.RegisterStatic("ResolveValueVariable", &SLTInternalPapyrusFunctionProvider::ResolveValueVariable);
        reg.RegisterStatic("SetCustomResolveFormResult", &SLTInternalPapyrusFunctionProvider::SetCustomResolveFormResult);
        reg.RegisterStatic("SetLibrariesForExtensionAllowed", &SLTInternalPapyrusFunctionProvider::SetLibrariesForExtensionAllowed);
        reg.RegisterStatic("WalkTheStack", &SLTInternalPapyrusFunctionProvider::WalkTheStack);
    }
};

REGISTER_PAPYRUS_PROVIDER(SLTInternalPapyrusFunctionProvider, "sl_triggers_internal")
#pragma endregion

// SLTNativeFunctions implementation remains the same below

#pragma region SLTNativeFunctions definition
void SLTNativeFunctions::SetCustomResolveFormResult(PAPYRUS_NATIVE_DECL, ThreadContextHandle threadContextHandle,
    RE::TESForm* resultingForm) {
    auto* frame = ContextManager::GetSingleton().GetFrameContext(threadContextHandle);
    if (!frame) {
        logger::error("Unable to retrieve frame with threadContextHandle ({})", threadContextHandle);
        return;
    }
    frame->customResolveFormResult = resultingForm;
}

void SLTNativeFunctions::SetLibrariesForExtensionAllowed(PAPYRUS_NATIVE_DECL, std::string_view extensionKey, 
                                        bool allowed) {
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    //SLTStackAnalyzer::Walk(stackId);
    if (funlib) {
        funlib->enabled = allowed;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, allowed);
    }
}

bool SLTNativeFunctions::PrepareContextForTargetedScript(PAPYRUS_NATIVE_DECL, RE::Actor* targetActor, 
                                        std::string_view scriptName) {
    //SLTStackAnalyzer::Walk(stackId);
    auto& manager = ContextManager::GetSingleton();
    return manager.StartSLTScript(targetActor, scriptName);
}

std::int32_t SLTNativeFunctions::GetActiveScriptCount(PAPYRUS_NATIVE_DECL) {
    return static_cast<std::int32_t>(ContextManager::GetSingleton().GetActiveContextCount());
}
    
SLTSessionId SLTNativeFunctions::GetSessionId(PAPYRUS_NATIVE_DECL) {
    return SLT::GenerateNewSessionId();
}

bool SLTNativeFunctions::IsLoaded(PAPYRUS_NATIVE_DECL) {
    return true;
}

std::string SLTNativeFunctions::GetTranslatedString(PAPYRUS_NATIVE_DECL, std::string_view input) {
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

std::vector<std::string> SLTNativeFunctions::Tokenize(PAPYRUS_NATIVE_DECL, std::string_view input) {
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

bool SLTNativeFunctions::DeleteTrigger(PAPYRUS_NATIVE_DECL, std::string_view extKeyStr, std::string_view trigKeyStr) {
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

RE::TESForm* SLTNativeFunctions::GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID) {
    return FormUtil::Parse::GetForm(a_editorID);
}

bool SLTNativeFunctions::SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b) {
    return SmartComparator::SmartEquals(std::string(a), std::string(b));
}

void SLTNativeFunctions::WalkTheStack(PAPYRUS_NATIVE_DECL) {
    SLTStackAnalyzer::Walk(stackId);
}

void SLTNativeFunctions::Pung(PAPYRUS_NATIVE_DECL) {
    if (!vm)
        return;
    
    //SLTStackAnalyzer::Walk(stackId);
    std::ignore = SLTStackAnalyzer::GetAMEContextInfo(stackId);
}

// In sl_triggers.cpp - Add this after the existing ExecuteStepAndPending function

// Forward declaration
class BatchExecutionManager;

RE::BSScript::LatentStatus SLTNativeFunctions::ExecuteStepAndPending(PAPYRUS_NATIVE_DECL) {
    if (!vm) {
        logger::error("ExecuteStepAndPending: VM is null");
        return RE::BSScript::LatentStatus::kFailed;
    }
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("ExecuteStepAndPending called without valid threadContextHandle property");
        return RE::BSScript::LatentStatus::kFailed;
    }

    auto& batchManager = BatchExecutionManager::GetSingleton();

    // Check if this batch is already running
    if (batchManager.IsExecuting(stackId)) {
        logger::debug("ExecuteStepAndPending: batch already executing for stackId 0x{:X}", stackId);
        return RE::BSScript::LatentStatus::kStarted;
    }
    
    logger::debug("ExecuteStepAndPending: starting latent batch for context {} (stackId 0x{:X})", 
                  contextInfo.handle, stackId);
    
    // Always start a batch - even if script is done, it will execute 0 steps and return false
    bool started = batchManager.StartBatch(stackId, contextInfo);
    
    if (started) {
        return RE::BSScript::LatentStatus::kStarted; // Papyrus thread suspends here
    } else {
        logger::error("Failed to start batch execution");
        return RE::BSScript::LatentStatus::kFailed;
    }
}

void SLTNativeFunctions::CleanupThreadContext(PAPYRUS_NATIVE_DECL) {
    if (!vm) return;
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("CleanupThreadContext called without valid threadContextHandle property");
        return;
    }
    
    auto& manager = ContextManager::GetSingleton();
    manager.CleanupContext(contextInfo.handle);
}

std::string SLTNativeFunctions::ResolveValueVariable(PAPYRUS_NATIVE_DECL, std::string_view token) {
    if (!vm) return "";
    
    SLTStackAnalyzer::Walk(stackId);
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("ResolveValueVariable called without valid threadContextHandle property");
        return "";
    }

    return ContextManager::GetSingleton().ResolveValueVariable(contextInfo.frame, token);
}

RE::TESForm* SLTNativeFunctions::ResolveFormVariable(PAPYRUS_NATIVE_DECL, std::string_view token) {
    if (!vm) return nullptr;
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("ResolveFormVariable called without valid threadContextHandle property");
        return nullptr;
    }

    return Salty::ResolveFormVariable(token, contextInfo.frame);
}

std::vector<std::string> SLTNativeFunctions::GetScriptsList(PAPYRUS_NATIVE_DECL) {
    std::vector<std::string> result;

    fs::path scriptsFolderPath = GetPluginPath() / "commands";

    if (fs::exists(scriptsFolderPath)) {
        for (const auto& entry : fs::directory_iterator(scriptsFolderPath)) {
            if (entry.is_regular_file()) {
                auto scriptname = entry.path().filename().string();
                if (scriptname.ends_with(".ini") || scriptname.ends_with(".json")) {
                    result.push_back(scriptname);
                }
            }
        }
    } else {
        logger::error("Scripts folder ({}) doesn't exist. You may need to reinstall the mod.", scriptsFolderPath.string());
    }

    if (result.size() > 1) {
        std::sort(result.begin(), result.end());
    }
    
    return result;
}

std::vector<std::string> SLTNativeFunctions::GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey) {
    std::vector<std::string> result;

    fs::path triggerFolderPath = GetPluginPath() / "extensions" / extensionKey;

    if (fs::exists(triggerFolderPath)) {
        for (const auto& entry : fs::directory_iterator(triggerFolderPath)) {
            if (entry.is_regular_file()) {
                if (entry.path().extension().string() == ".json") {
                    result.push_back(entry.path().filename().string());
                }
            }
        }
    } else {
        logger::error("Trigger folder ({}) doesn't exist. You may need to reinstall the mod or at least make sure the folder is created.",
            triggerFolderPath.string());
    }
    
    if (result.size() > 1) {
        std::sort(result.begin(), result.end());
    }
    
    return result;
}

RE::TESQuest* SLTNativeFunctions::MainQuest(PAPYRUS_NATIVE_DECL) {
    RE::TESQuest* result = nullptr;

    RE::TESForm* form = FormUtil::Parse::GetForm("sl_triggersMain");
    result = form->As<RE::TESQuest>();

    return result;
}


void SLTNativeFunctions::RegisterExtension(PAPYRUS_NATIVE_DECL, RE::TESQuest* extensionQuest) {
    SLTExtensionTracker::AddQuest(extensionQuest, stackId);
    auto papquest = PapyrusConverter::QuestToPapyrus(extensionQuest, vm);
    SLT::ModEvent::SendToAll("_slt_event_slt_register_extension_", static_cast<RE::TESQuest*>(extensionQuest));
}

#pragma endregion

#pragma region Event registrations
OnDataLoaded([]{
    // Register serialization callbacks
    auto* serialization = SKSE::GetSerializationInterface();
    if (serialization) {
        serialization->SetSaveCallback([](SKSE::SerializationInterface* intfc) {
            if (intfc->OpenRecord('SLTR', 1)) {
                ContextManager::GetSingleton().Serialize(intfc);
            }
        });
        
        serialization->SetLoadCallback([](SKSE::SerializationInterface* intfc) {
            std::uint32_t type, version, length;
            while (intfc->GetNextRecordInfo(type, version, length)) {
                if (type == 'SLTR' && version == 1) {
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
    logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::GetSessionId());
});

OnSaveGame([]{
    // Don't pause execution for saves during play
    // The coordination lock in ContextManager handles consistency during serialization
    logger::info("Save initiated - using coordination lock for consistency");
});

#pragma endregion

};

#pragma pop()