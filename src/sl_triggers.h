#pragma once

namespace SLT {

#pragma region SLTNativeFunctions declaration
class SLTNativeFunctions {
public:
// Non-latent functions
static bool DeleteTrigger(PAPYRUS_NATIVE_DECL, std::string_view extKeyStr, std::string_view trigKeyStr);

static RE::TESForm* GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID);

static std::vector<std::string> GetScriptsList(PAPYRUS_NATIVE_DECL);

static SLTSessionId GetSessionId(PAPYRUS_NATIVE_DECL);

static std::string GetTranslatedString(PAPYRUS_NATIVE_DECL, std::string_view input);

static std::vector<std::string> GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey);

static bool RunOperationOnActor(PAPYRUS_NATIVE_DECL, RE::Actor* cmdTarget, RE::ActiveEffect* cmdPrimary,
                                            std::vector<std::string> tokens);

static void SetExtensionEnabled(PAPYRUS_NATIVE_DECL, std::string_view extensionKey,
                                            bool enabledState);

static bool SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b);

static std::vector<std::string> SplitFileContents(PAPYRUS_NATIVE_DECL, std::string_view filecontents);

static bool StartScript(PAPYRUS_NATIVE_DECL, RE::Actor* cmdTarget, std::string_view initialScriptName);

static std::string Trim(PAPYRUS_NATIVE_DECL, std::string_view str);

static std::vector<std::string> Tokenize(PAPYRUS_NATIVE_DECL, std::string_view input);

static std::vector<std::string> Tokenizev2(PAPYRUS_NATIVE_DECL, std::string_view input);

static std::vector<std::string> TokenizeForVariableSubstitution(PAPYRUS_NATIVE_DECL, std::string_view input);
};
#pragma endregion


#pragma region SLTPapyrusFunctionProvider

class SLTPapyrusFunctionProvider : public SLT::binding::PapyrusFunctionProvider<SLTPapyrusFunctionProvider> {
public:
    // Static Papyrus function implementations
    static RE::TESForm* GetForm(PAPYRUS_STATIC_ARGS, std::string_view someFormOfFormIdentification) {
        return SLT::SLTNativeFunctions::GetForm(PAPYRUS_FN_PARMS, someFormOfFormIdentification);
    }

    static std::vector<std::string> GetScriptsList(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetScriptsList(PAPYRUS_FN_PARMS);
    }

    static std::int32_t GetSessionId(PAPYRUS_STATIC_ARGS) {
        return SLT::SLTNativeFunctions::GetSessionId(PAPYRUS_FN_PARMS);
    }

    static std::string GetTranslatedString(PAPYRUS_STATIC_ARGS, std::string_view input) {
        return SLT::SLTNativeFunctions::GetTranslatedString(PAPYRUS_FN_PARMS, input);
    }

    static bool SmartEquals(PAPYRUS_STATIC_ARGS, std::string_view a, std::string_view b) {
        return SLT::SLTNativeFunctions::SmartEquals(PAPYRUS_FN_PARMS, a, b);
    }

    static std::vector<std::string> SplitFileContents(PAPYRUS_STATIC_ARGS, std::string_view content) {
        return SLT::SLTNativeFunctions::SplitFileContents(PAPYRUS_FN_PARMS, content);
    }

    static std::vector<std::string> Tokenize(PAPYRUS_STATIC_ARGS, std::string_view input) {
        return SLT::SLTNativeFunctions::Tokenize(PAPYRUS_FN_PARMS, input);
    }

    static std::vector<std::string> Tokenizev2(PAPYRUS_STATIC_ARGS, std::string_view input) {
        return SLT::SLTNativeFunctions::Tokenizev2(PAPYRUS_FN_PARMS, input);
    }

    static std::vector<std::string> TokenizeForVariableSubstitution(PAPYRUS_STATIC_ARGS, std::string_view input) {
        return SLT::SLTNativeFunctions::TokenizeForVariableSubstitution(PAPYRUS_FN_PARMS, input);
    }

    static std::string Trim(PAPYRUS_STATIC_ARGS, std::string_view str) {
        return SLT::SLTNativeFunctions::Trim(PAPYRUS_FN_PARMS, str);
    }

    void RegisterAllFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) {
        SLT::binding::PapyrusRegistrar<SLTPapyrusFunctionProvider> reg(vm, className);
        
        reg.RegisterStatic("GetForm", &SLTPapyrusFunctionProvider::GetForm);
        reg.RegisterStatic("GetScriptsList", &SLTPapyrusFunctionProvider::GetScriptsList);
        reg.RegisterStatic("GetSessionId", &SLTPapyrusFunctionProvider::GetSessionId);
        reg.RegisterStatic("GetTranslatedString", &SLTPapyrusFunctionProvider::GetTranslatedString);
        reg.RegisterStatic("SmartEquals", &SLTPapyrusFunctionProvider::SmartEquals);
        reg.RegisterStatic("SplitFileContents", &SLTPapyrusFunctionProvider::SplitFileContents);
        reg.RegisterStatic("Tokenize", &SLTPapyrusFunctionProvider::Tokenize);
        reg.RegisterStatic("Tokenizev2", &SLTPapyrusFunctionProvider::Tokenizev2);
        reg.RegisterStatic("TokenizeForVariableSubstitution", &SLTPapyrusFunctionProvider::TokenizeForVariableSubstitution);
        reg.RegisterStatic("Trim", &SLTPapyrusFunctionProvider::Trim);
    }
};

#pragma endregion

#pragma region SLTInternalPapyrusFunctionProvider
class SLTInternalPapyrusFunctionProvider : public SLT::binding::PapyrusFunctionProvider<SLTInternalPapyrusFunctionProvider> {
public:
    // Static Papyrus function implementations
    // NON-LATENT
    static bool DeleteTrigger(PAPYRUS_STATIC_ARGS, std::string extKeyStr, std::string trigKeyStr) {
        return SLT::SLTNativeFunctions::DeleteTrigger(PAPYRUS_FN_PARMS, extKeyStr, trigKeyStr);
    }

    static std::vector<std::string> GetTriggerKeys(PAPYRUS_STATIC_ARGS, std::string_view extensionKey) {
        return SLT::SLTNativeFunctions::GetTriggerKeys(PAPYRUS_FN_PARMS, extensionKey);
    }

    static bool RunOperationOnActor(PAPYRUS_STATIC_ARGS, RE::Actor* cmdTarget, RE::ActiveEffect* cmdPrimary,
                                            std::vector<std::string> tokens) {
        return SLT::SLTNativeFunctions::RunOperationOnActor(PAPYRUS_FN_PARMS, cmdTarget, cmdPrimary, tokens);
    }

    static void SetExtensionEnabled(PAPYRUS_STATIC_ARGS, std::string_view extensionKey, bool enabledState) {
        SLT::SLTNativeFunctions::SetExtensionEnabled(PAPYRUS_FN_PARMS, extensionKey, enabledState);
    }

    static bool StartScript(PAPYRUS_STATIC_ARGS, RE::Actor* cmdTarget, std::string_view initialScriptName) {
        return SLT::SLTNativeFunctions::StartScript(PAPYRUS_FN_PARMS, cmdTarget, initialScriptName);
    }

    void RegisterAllFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) {
        SLT::binding::PapyrusRegistrar<SLTInternalPapyrusFunctionProvider> reg(vm, className);

        reg.RegisterStatic("DeleteTrigger", &SLTInternalPapyrusFunctionProvider::DeleteTrigger);
        reg.RegisterStatic("GetTriggerKeys", &SLTInternalPapyrusFunctionProvider::GetTriggerKeys);
        reg.RegisterStatic("RunOperationOnActor", &SLTInternalPapyrusFunctionProvider::RunOperationOnActor);
        reg.RegisterStatic("SetExtensionEnabled", &SLTInternalPapyrusFunctionProvider::SetExtensionEnabled);
        reg.RegisterStatic("StartScript", &SLTInternalPapyrusFunctionProvider::StartScript);
    }
};
#pragma endregion

}