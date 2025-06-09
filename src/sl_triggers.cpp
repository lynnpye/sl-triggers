#pragma push(warning)
#pragma warning(disable:4100)

#include "contexting.h"
#include "engine.h"
#include "coroutines.h"
#include "questor.h"
#include "papyrus_conversion.h"
#include "modevent.h"
#include "forge.h"

namespace SLT {

class BatchExecutionManager;

#pragma region SLTNativeFunctions declaration
class SLTNativeFunctions {
public:
// Latent functions
static RE::BSScript::LatentStatus ExecuteStepAndPending(PAPYRUS_NATIVE_DECL);

// Non-latent functions
static void CleanupThreadContext(PAPYRUS_NATIVE_DECL);

static bool DeleteTrigger(PAPYRUS_NATIVE_DECL, std::string_view extKeyStr, std::string_view trigKeyStr);

static std::int32_t GetActiveScriptCount(PAPYRUS_NATIVE_DECL);

static RE::TESForm* GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID);

static std::vector<std::string> GetScriptsList(PAPYRUS_NATIVE_DECL);

static SLTSessionId GetSessionId(PAPYRUS_NATIVE_DECL);

static std::string GetTranslatedString(PAPYRUS_NATIVE_DECL, std::string_view input);

static std::vector<std::string> GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey);

static bool IsLoaded(PAPYRUS_NATIVE_DECL);

static RE::TESQuest* MainQuest(PAPYRUS_NATIVE_DECL);

static void PauseExecution(PAPYRUS_NATIVE_DECL, std::string_view reason);

static bool PrepareContextForTargetedScript(PAPYRUS_NATIVE_DECL, RE::Actor* targetActor, 
                                                        std::string_view scriptName);
                                
static void Pung(PAPYRUS_NATIVE_DECL);

static void RegisterExtension(PAPYRUS_NATIVE_DECL, RE::TESQuest* extensionQuest);

static RE::TESForm* ResolveFormVariable(PAPYRUS_NATIVE_DECL, std::string_view variableName);

static std::string ResolveValueVariable(PAPYRUS_NATIVE_DECL, std::string_view variableName);

static void ResumeExecution(PAPYRUS_NATIVE_DECL);

static void SetExtensionEnabled(PAPYRUS_NATIVE_DECL, std::string_view extensionKey,
                                            bool enabledState);


//static void SetMostRecentResult(PAPYRUS_NATIVE_DECL, ThreadContextHandle threadContextHandle,
//                                            std::string_view mostRecentResult);

static bool SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b);

static std::vector<std::string> Tokenize(PAPYRUS_NATIVE_DECL, std::string_view input);

static void WalkTheStack(PAPYRUS_NATIVE_DECL);
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

    static std::vector<std::string> GetScriptsList(PAPYRUS_STATIC_ARGS) {
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

    static void PauseExecution(PAPYRUS_STATIC_ARGS, std::string_view reason) {
        SLT::SLTNativeFunctions::PauseExecution(PAPYRUS_FN_PARMS, reason);
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

    static void ResumeExecution(PAPYRUS_STATIC_ARGS) {
        SLT::SLTNativeFunctions::ResumeExecution(PAPYRUS_FN_PARMS);
    }

    static void SetExtensionEnabled(PAPYRUS_STATIC_ARGS, std::string_view extensionKey, bool enabledState) {
        SLT::SLTNativeFunctions::SetExtensionEnabled(PAPYRUS_FN_PARMS, extensionKey, enabledState);
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
        reg.RegisterStatic("PauseExecution", &SLTInternalPapyrusFunctionProvider::PauseExecution);
        reg.RegisterStatic("PrepareContextForTargetedScript", &SLTInternalPapyrusFunctionProvider::PrepareContextForTargetedScript);
        reg.RegisterStatic("Pung", &SLTInternalPapyrusFunctionProvider::Pung);
        reg.RegisterStatic("RegisterExtension", &SLTInternalPapyrusFunctionProvider::RegisterExtension);
        reg.RegisterStatic("ResolveFormVariable", &SLTInternalPapyrusFunctionProvider::ResolveFormVariable);
        reg.RegisterStatic("ResolveValueVariable", &SLTInternalPapyrusFunctionProvider::ResolveValueVariable);
        reg.RegisterStatic("ResumeExecution", &SLTInternalPapyrusFunctionProvider::ResumeExecution);
        reg.RegisterStatic("SetExtensionEnabled", &SLTInternalPapyrusFunctionProvider::SetExtensionEnabled);
        reg.RegisterStatic("WalkTheStack", &SLTInternalPapyrusFunctionProvider::WalkTheStack);
    }
};

REGISTER_PAPYRUS_PROVIDER(SLTInternalPapyrusFunctionProvider, "sl_triggers_internal")
#pragma endregion

// SLTNativeFunctions implementation remains the same below

#pragma region SLTNativeFunctions definition
// Latent functions
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

// Non-latent Functions
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

std::int32_t SLTNativeFunctions::GetActiveScriptCount(PAPYRUS_NATIVE_DECL) {
    return static_cast<std::int32_t>(ContextManager::GetSingleton().GetActiveContextCount());
}

RE::TESForm* SLTNativeFunctions::GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID) {
    return FormUtil::Parse::GetForm(a_editorID);
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

SLTSessionId SLTNativeFunctions::GetSessionId(PAPYRUS_NATIVE_DECL) {
    return SLT::GenerateNewSessionId();
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

std::vector<std::string> SLTNativeFunctions::GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey) {
    std::vector<std::string> result;

    fs::path triggerFolderPath = GetPluginPath() / "extensions" / extensionKey;

    if (fs::exists(triggerFolderPath)) {
        for (const auto& entry : fs::directory_iterator(triggerFolderPath)) {
            if (entry.is_regular_file()) {
                if (str::iEquals(entry.path().extension().string(), ".json")) {
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

bool SLTNativeFunctions::IsLoaded(PAPYRUS_NATIVE_DECL) {
    return true;
}

RE::TESQuest* SLTNativeFunctions::MainQuest(PAPYRUS_NATIVE_DECL) {
    RE::TESQuest* result = nullptr;

    RE::TESForm* form = FormUtil::Parse::GetForm("sl_triggersMain");
    result = form->As<RE::TESQuest>();

    return result;
}

void SLTNativeFunctions::PauseExecution(PAPYRUS_NATIVE_DECL, std::string_view reason) {
    SLT::ContextManager::GetSingleton().PauseExecution(reason);
}

bool SLTNativeFunctions::PrepareContextForTargetedScript(PAPYRUS_NATIVE_DECL, RE::Actor* targetActor, 
                                        std::string_view scriptName) {
    auto& manager = ContextManager::GetSingleton();
    return manager.StartSLTScript(targetActor, scriptName);
}

void SLTNativeFunctions::Pung(PAPYRUS_NATIVE_DECL) {
    if (!vm)
        return;
    
    // Validate stackId first
    if (stackId == 0 || stackId == static_cast<RE::VMStackID>(-1)) {
        logger::warn("Invalid stackId: 0x{:X}", stackId);
        return;
    }

    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) {
        logger::error("Unable to get handle policy");
        return;
    }
    
    RE::BSScript::Stack* stackPtr = nullptr;
    
    try {
        bool success = vm->GetStackByID(stackId, &stackPtr);
        
        if (!success) {
            logger::warn("GetStackByID returned false for ID: 0x{:X}", stackId);
            return;
        }
        
        if (!stackPtr) {
            logger::warn("GetStackByID succeeded but returned null pointer for ID: 0x{:X}", stackId);
            return;
        }
    } catch (const std::exception& e) {
        logger::error("Exception in GetStackByID: {}", e.what());
        return;
    } catch (...) {
        logger::error("Unknown exception in GetStackByID for stackId: 0x{:X}", stackId);
        return;
    }
    
    if (!stackPtr->owningTasklet) {
        logger::warn("Stack has no owning tasklet for ID: 0x{:X}", stackId);
        return;
    }
    
    auto taskletPtr = stackPtr->owningTasklet;
    
    if (!taskletPtr->topFrame) {
        logger::warn("Tasklet has no top frame for stack ID: 0x{:X}", stackId);
        return;
    }

    RE::BSFixedString propNameThreadContextHandle("threadContextHandle");
    RE::BSFixedString propNameInitialScriptName("initialScriptName");
    RE::BSFixedString targetCmdScriptName("sl_triggersCmd");

    RE::BSScript::Variable& self = taskletPtr->topFrame->self;
    if (self.IsObject()) {
        auto selfObject = self.GetObject();
        if (selfObject) {
            auto* typeInfo = selfObject->GetTypeInfo();
            std::string typeName = "<unknown or irretrievable>";
            if (typeInfo)
                typeName = typeInfo->GetName();
            RE::BSFixedString selfReportedScriptName(typeName);
            if (selfReportedScriptName == targetCmdScriptName) {
                RE::BSScript::Variable* propThreadContextHandle = selfObject->GetProperty(propNameThreadContextHandle);
                RE::BSScript::Variable* propInitialScriptName = selfObject->GetProperty(propNameInitialScriptName);

                if (!propThreadContextHandle || !propInitialScriptName) {
                    logger::debug("Malformed SLT AME, missing threadContextHandle and/or initialScriptName properties");
                    return;
                }
        
                ThreadContextHandle cid = 0;
                if (propThreadContextHandle && propThreadContextHandle->IsInt()) {
                    cid = propThreadContextHandle->GetSInt();
                }

                if (cid) {
                    logger::warn("AME already setup with threadContextHandle, ignoring");
                } else {
                    // find target based on actor and get threadContextHandle for !isClaimed
                    auto* handlePolicy = vm->GetObjectHandlePolicy();
                    auto* bindPolicy = vm->GetObjectBindPolicy();
                    if (!handlePolicy || !bindPolicy) {
                        logger::error("Unable to obtain vm policies");
                        return;
                    }
                    auto* ameRawPtr = handlePolicy->GetObjectForHandle(RE::ActiveEffect::VMTYPEID, selfObject->GetHandle());
                    if (!ameRawPtr) {
                        logger::error("GetObjectForHandle returned null AME");
                        return;
                    }
                    RE::ActiveEffect* ame = static_cast<RE::ActiveEffect*>(ameRawPtr);
                    if (!ame) {
                        logger::error("Unable to cast to correct AME");
                        return;
                    }
                    RE::Actor* actor = ame->GetCasterActor().get();
                    if (!actor) {
                        logger::error("SLT AME missing a caster Actor");
                        return;
                    }
                    ContextManager& contextManager = ContextManager::GetSingleton();
                    auto* targetContext = contextManager.CreateTargetContext(actor);

                    auto it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(),
                    [&](const std::shared_ptr<ThreadContext>& tcptr) -> bool {
                        return !tcptr->isClaimed && !tcptr->wasClaimed;
                    });

                    if (it == targetContext->threads.end()) {
                        it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(),
                        [&](const std::shared_ptr<ThreadContext>& tcptr) -> bool {
                            return !tcptr->isClaimed;
                        });
                    }

                    if (it != targetContext->threads.end()) {
                        auto& threadCon = *it;
                        threadCon->ame = ame;
                        propThreadContextHandle->SetSInt(threadCon->threadContextHandle);
                        propInitialScriptName->SetString(threadCon->initialScriptName);
                    }
                    else {
                        logger::error("Unable to find available unclaimed threadContext");
                        return;
                    }
                }
            }
            else {
                logger::error("AME is not {}", targetCmdScriptName.c_str());
                return;
            }
        }
        else {
            logger::error("AME could not obtain Object");
            return;
        }
    }
    else {
        logger::error("AME is not Object");
        return;
    }
}

void SLTNativeFunctions::RegisterExtension(PAPYRUS_NATIVE_DECL, RE::TESQuest* extensionQuest) {
    if (!extensionQuest) {
        logger::error("extensionQuest is required for RegisterExtension");
        return;
    }
    SLTExtensionTracker::AddQuest(extensionQuest, stackId);
    SLT::ModEvent::SendToAll("OnSLTRegisterExtension", static_cast<RE::TESQuest*>(extensionQuest));
}

RE::TESForm* SLTNativeFunctions::ResolveFormVariable(PAPYRUS_NATIVE_DECL, std::string_view token) {
    if (!vm) return nullptr;
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("ResolveFormVariable called without valid threadContextHandle property");
        return nullptr;
    }

    RE::TESForm* finalResolution = Salty::ResolveFormVariable(token, contextInfo.frame);
    return finalResolution;
}

std::string SLTNativeFunctions::ResolveValueVariable(PAPYRUS_NATIVE_DECL, std::string_view token) {
    if (!vm) return "";
    
    auto contextInfo = SLTStackAnalyzer::GetAMEContextInfo(stackId);

    
    if (!contextInfo.isValid || contextInfo.handle == 0) {
        logger::error("ResolveValueVariable called without valid threadContextHandle property");
        return "";
    }

    std::string finalResolution = ContextManager::GetSingleton().ResolveValueVariable(contextInfo.frame, token);
    return finalResolution;
}

void SLTNativeFunctions::ResumeExecution(PAPYRUS_NATIVE_DECL) {
    SLT::ContextManager::GetSingleton().ResumeExecution();
}

void SLTNativeFunctions::SetExtensionEnabled(PAPYRUS_NATIVE_DECL, std::string_view extensionKey, bool enabledState) {
    SLTExtensionTracker::SetEnabled(extensionKey, enabledState);
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    //SLTStackAnalyzer::Walk(stackId);
    if (funlib) {
        funlib->enabled = enabledState;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, enabledState);
    }
}

bool SLTNativeFunctions::SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b) {
    return SmartComparator::SmartEquals(std::string(a), std::string(b));
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
    return tokens;
}

void SLTNativeFunctions::WalkTheStack(PAPYRUS_NATIVE_DECL) {
    SLTStackAnalyzer::Walk(stackId);
}


#pragma endregion

#pragma region Event registrations
OnDataLoaded([]{
    // Register serialization callbacks
    auto* serialization = SKSE::GetSerializationInterface();
    if (serialization) {
        serialization->SetSaveCallback([](SKSE::SerializationInterface* intfc) {
            if (intfc->OpenRecord('SLTR', 1)) {
                OnOptionalSave(intfc);
                ContextManager::GetSingleton().Serialize(intfc);
            }
        });
        
        serialization->SetLoadCallback([](SKSE::SerializationInterface* intfc) {
            std::uint32_t type, version, length;
            while (intfc->GetNextRecordInfo(type, version, length)) {
                if (type == 'SLTR' && version == 1) {
                    OnOptionalLoad(intfc);
                    ContextManager::GetSingleton().Deserialize(intfc);
                }
            }
        });
        
        serialization->SetRevertCallback([](SKSE::SerializationInterface* intfc) {
            // Clear all contexts on new game
            OnOptionalRevert(intfc);
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
    logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::GetSessionId());
});

#pragma endregion

};

#pragma pop()