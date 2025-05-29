#pragma push(warning)
#pragma warning(disable:4100)

#include "util.h"
#include "sltutil.h"

#include "contexting.h"
#include "engine.h"

namespace {
static SLTSessionId sessionId;
static bool sessionIdGenerated = false;
void GenerateNewSessionId() {
    static std::random_device rd;
    static std::mt19937 engine(rd());
    static std::uniform_int_distribution<std::int32_t> dist(std::numeric_limits<std::int32_t>::min(),
                                                            std::numeric_limits<std::int32_t>::max());
    sessionId = dist(engine);
}
}

namespace SLT {

#pragma region SLTNativeFunctions declaration
class SLTNativeFunctions {
public:
static void SetLibrariesForExtensionAllowed(std::string extensionKey, 
                                            bool allowed);

static bool PrepareContextForTargetedScript(RE::Actor* targetActor, 
                                                        std::string scriptName);

static std::int32_t GetActiveScriptCount();

static SLTSessionId GetSessionId();

static bool IsLoaded();

static std::string GetTranslatedString(const std::string input);

static std::vector<std::string> Tokenize(const std::string input);

static bool DeleteTrigger(std::string extKeyStr, std::string trigKeyStr);

static RE::TESForm* FindFormByEditorId(const std::string a_editorID);

static bool SmartEquals(std::string a, std::string b);
                                
// These will attempt auto-contexting
static void Pung(RE::BSScript::Internal::VirtualMachine* vm, const RE::VMStackID stackId);

static bool ExecuteStepAndPending(RE::BSScript::Internal::VirtualMachine* vm, 
                                    const RE::VMStackID stackId);

static void CleanupThreadContext(RE::BSScript::Internal::VirtualMachine* vm, 
                                    const RE::VMStackID stackId);
};
#pragma endregion

#pragma region Function Libraries
struct FunctionLibrary {
    std::string configFile;
    std::string functionFile;
    std::string extensionKey;
    std::int32_t priority;
    bool enabled;

    explicit FunctionLibrary(std::string _configFile, std::string _functionFile, std::string _extensionKey, std::int32_t _priority, bool _enabled = true)
        : configFile(_configFile), functionFile(_functionFile), extensionKey(_extensionKey), priority(_priority), enabled(_enabled) {}

    static FunctionLibrary* ByExtensionKey(const std::string& _extensionKey);
};

static const std::string SLTCmdLib = "sl_triggersCmdLibSLT";
static std::vector<std::unique_ptr<FunctionLibrary>> g_FunctionLibraries;
static std::unordered_map<std::string_view, std::string_view> functionScriptCache;

FunctionLibrary* FunctionLibrary::ByExtensionKey(const std::string& _extensionKey) {
    auto it = std::find_if(g_FunctionLibraries.begin(), g_FunctionLibraries.end(),
        [&_extensionKey](const std::unique_ptr<FunctionLibrary>& lib) {
            return lib->extensionKey == _extensionKey;
        });

    if (it != g_FunctionLibraries.end()) {
        return it->get();
    } else {
        return nullptr;
    }
}

void GetFunctionLibraries() {
    g_FunctionLibraries.clear();

    using namespace std;

    vector<string> libconfigs;
    fs::path folderPath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "extensions";
    
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
                        g_FunctionLibraries.push_back(std::move(std::make_unique<FunctionLibrary>(filename, lib, extensionKey, pri)));
                    }
                }
            }
        }
        
        g_FunctionLibraries.push_back(std::move(std::make_unique<FunctionLibrary>(SLTCmdLib, SLTCmdLib, SLTCmdLib, 0)));

        sort(g_FunctionLibraries.begin(), g_FunctionLibraries.end(), [](const auto& a, const auto& b) {
            return a->priority < b->priority;
        });
    } else {
        fs::path checking = fs::path("Data");
        vector<string> parts = { "SKSE", "Plugins", "sl_triggers", "extensions" };

        auto pit = parts.begin();
        while (fs::exists(checking) && pit != parts.end()) {
            checking /= *pit;
            pit++;
        }
        if (!fs::exists(checking)) {
            logger::error("Looking for extensions path '{}' but was not able to find '{}'", folderPath.string(), checking.string());
        }
    }
}

bool PrecacheLibraries() {
    GetFunctionLibraries();
    if (g_FunctionLibraries.empty()) {
        logger::info("PrecacheLibraries: libraries was empty");
        return false;
    }
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeinfoptr;
    for (const auto& scriptlib : g_FunctionLibraries) {
        std::string& _scriptname = scriptlib->functionFile;
        bool success = vm->GetScriptObjectType1(_scriptname, typeinfoptr);

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

            auto cachedIt = functionScriptCache.find(libfuncName);
            if (cachedIt != functionScriptCache.end()) {
                // cache hit, continue
                continue;
            }

            if (libfunc->GetParamCount() != 3) {
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

            functionScriptCache[libfuncName] = _scriptname;
        }
    }

    logger::info("PrecacheLibraries completed");
    return true;
}

OnPostPostLoad([]{
    SLT::PrecacheLibraries();
})
#pragma endregion

#pragma region SLTStackAnalyzer
class SLTStackAnalyzer {
public:
    struct ContextInfo {
        ThreadContextHandle contextId = 0;
        std::string initialScriptName;
        bool isValid = false;
    };
    
    static ContextInfo GetFullContextInfo(RE::VMStackID stackId) {
        ContextInfo result;
        
        // Validate stackId first
        if (stackId == 0 || stackId == static_cast<RE::VMStackID>(-1)) {
            logger::warn("Invalid stackId: 0x{:X}", stackId);
            return result;
        }
        
        using RE::BSScript::Internal::VirtualMachine;
        using RE::BSScript::Stack;
        using RE::BSScript::Internal::CodeTasklet;
        using RE::BSScript::StackFrame;
        
        auto* vm = VirtualMachine::GetSingleton();
        if (!vm) {
            logger::warn("Failed to get VM singleton");
            return result;
        }
        
        RE::BSScript::Stack* stackPtr = nullptr;
        
        try {
            bool success = vm->GetStackByID(stackId, &stackPtr);
            
            if (!success) {
                logger::warn("GetStackByID returned false for ID: 0x{:X}", stackId);
                return result;
            }
            
            if (!stackPtr) {
                logger::warn("GetStackByID succeeded but returned null pointer for ID: 0x{:X}", stackId);
                return result;
            }
        } catch (const std::exception& e) {
            logger::error("Exception in GetStackByID: {}", e.what());
            return result;
        } catch (...) {
            logger::error("Unknown exception in GetStackByID for stackId: 0x{:X}", stackId);
            return result;
        }
        
        if (!stackPtr->owningTasklet) {
            logger::warn("Stack has no owning tasklet for ID: 0x{:X}", stackId);
            return result;
        }
        
        auto taskletPtr = stackPtr->owningTasklet;
        
        if (!taskletPtr->topFrame) {
            logger::warn("Tasklet has no top frame for stack ID: 0x{:X}", stackId);
            return result;
        }
        
        auto* frame = taskletPtr->topFrame;
        
        RE::BSScript::Variable& self = frame->self;
        if (!self.IsObject()) {
            logger::warn("Frame self is not an object for stack ID: 0x{:X}", stackId);
            return result;
        }
        
        auto selfObject = self.GetObject();
        if (!selfObject) {
            logger::warn("Failed to get self object for stack ID: 0x{:X}", stackId);
            return result;
        }
        
        RE::BSFixedString propNameThreadContextHandle("threadContextHandle");
        RE::BSScript::Variable* propThreadContextHandle = selfObject->GetProperty(propNameThreadContextHandle);
        
        RE::BSFixedString propNameInitialScriptName("initialScriptName");
        RE::BSScript::Variable* propInitialScriptName = selfObject->GetProperty(propNameInitialScriptName);

        /*
        * My intent is that the two flags, isClaimed and wasClaimed, would assist in
        * automated retrieval of contextId when an AME calls one of my functions. For example, if an AME
        * calls in but does not have a non-zero threadContextHandle in its property, this code is supposed
        * to:
        *   - find the Actor the calling thing is attached to
        *   - look up the TargetContext from that
        *   - if not found, leave because it should already be set up
        *   - if found, iterate all of its ThreadContexts
        *   - the first one that is not claimed and was never claimed should be returned and its threadContextHandle set into the property
        *  
        */
        ThreadContextHandle cid = 0;
        std::string_view initialScriptName = "";

        if (propThreadContextHandle && propThreadContextHandle->IsInt()) {
            cid = propThreadContextHandle->GetSInt();
        }

        if (propInitialScriptName && propInitialScriptName->IsString()) {
            initialScriptName = propInitialScriptName->GetString();
        }

        if (cid == 0 || initialScriptName.empty()) {
            // need to see if we have a waiting ThreadContext
            // I need my actor's formID
            auto* handlePolicy = vm->GetObjectHandlePolicy();
            if (!handlePolicy) {
                logger::error("Unable to get handle policy");
                return result;
            }
            RE::VMHandle objHandle = selfObject->GetHandle();
            if (!handlePolicy->IsHandleObjectAvailable(objHandle)) {
                logger::error("HandlePolicy says handle object not available!");
                // maybe return result;
            }
            RE::ActiveEffect* ame = static_cast<RE::ActiveEffect*>(handlePolicy->GetObjectForHandle(RE::ActiveEffect::VMTYPEID, objHandle));
            if (!ame) {
                logger::error("Unable to obtain AME from selfObject with RE::VMHandle({})", objHandle);
                return result;
            }
            RE::Actor* ameActor = ame->GetCasterActor().get();
            if (!ameActor) {
                logger::error("Unable to obtain AME.Actor from selfObject");
                return result;
            }

            auto* ameContext = ContextManager::GetSingleton().GetTargetContext(ameActor);
            if (!ameContext) {
                // I ... I don't know what to say... you probably shouldn't have come
                logger::error("No available TargetContext for formID {}", ameActor->GetFormID());
                return result;
            }

            ThreadContext* threadContext = ContextManager::GetSingleton().WithActiveContexts([&ameContext](auto& activeContexts){
                auto it = std::find_if(ameContext->threads.begin(), ameContext->threads.end(),
                    [](const std::unique_ptr<ThreadContext>& threadContext) {
                        return !threadContext->isClaimed && !threadContext->wasClaimed;
                    });
                ThreadContext* returnval = nullptr;
                if (it != ameContext->threads.end()) {
                    return it->get();
                }
                return returnval;
            });

            if (!threadContext) {
                logger::error("No ThreadContext found");
                return result;
            }
            threadContext->isClaimed = true;
            threadContext->wasClaimed = true;
            propThreadContextHandle->SetSInt(threadContext->threadContextHandle);
            propInitialScriptName->SetString(threadContext->initialScriptName);

            cid = threadContext->threadContextHandle;

            if (!cid) {
                logger::error("Unable to obtain a ThreadContextHandle for requesting executor");
                return result;
            }

            initialScriptName = threadContext->initialScriptName;
        }
    
        result.contextId = cid;
        result.initialScriptName = initialScriptName;
        
        result.isValid = (result.contextId != 0);
        
        return result;
    }
};
#pragma endregion

#pragma region Papyrus Static Function Bindings
PLUGIN_BINDINGS_PAPYRUS_CLASS(sl_triggers_internal) {

#pragma region SetLibrariesForExtensionAllowed
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(SetLibrariesForExtensionAllowed, std::string extensionKey, bool allowed) {
        SLT::SLTNativeFunctions::SetLibrariesForExtensionAllowed(extensionKey, allowed);
    };
#pragma endregion

#pragma region PrepareContextForTargetedScript
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(PrepareContextForTargetedScript, RE::Actor* targetActor, std::string scriptname) -> bool {
        return SLT::SLTNativeFunctions::PrepareContextForTargetedScript(targetActor, scriptname);
    };
#pragma endregion

#pragma region GetActiveScriptCount  
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(GetActiveScriptCount) -> std::int32_t {
        return SLT::SLTNativeFunctions::GetActiveScriptCount();
    };
#pragma endregion

#pragma region SessionId Papyrus
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(GetSessionId) -> std::int32_t {
        return SLT::SLTNativeFunctions::GetSessionId();
    };
#pragma endregion

#pragma region IsLoaded
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(IsLoaded) -> bool {
        return SLT::SLTNativeFunctions::IsLoaded();
    };
#pragma endregion

#pragma region GetTranslatedString
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(GetTranslatedString, const std::string input) -> std::string {
        return SLT::SLTNativeFunctions::GetTranslatedString(input);
    };
#pragma endregion

#pragma region Tokenize
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(Tokenize, std::string input) -> std::vector<std::string> {
        return SLT::SLTNativeFunctions::Tokenize(input);
    };
#pragma endregion

#pragma region DeleteTrigger
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(DeleteTrigger, std::string extKeyStr, std::string trigKeyStr) -> bool {
        return SLT::SLTNativeFunctions::DeleteTrigger(extKeyStr, trigKeyStr);
    };
#pragma endregion

#pragma region FindFormByEditorId NEEDS REVISIT
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(FindFormByEditorId, std::string a_editorID) -> RE::TESForm* {
        return SLT::SLTNativeFunctions::FindFormByEditorId(a_editorID);
    };
#pragma endregion

#pragma region SmartEquals
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(SmartEquals, std::string a, std::string b) -> bool {
        return SLT::SLTNativeFunctions::SmartEquals(a, b);
    };
#pragma endregion

// These will perform auto-contexting
#pragma region Pung
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(Pung) {
        SLT::SLTNativeFunctions::Pung(VirtualMachine, StackID);
    };
#pragma endregion

#pragma region ExecuteAndPending Papyrus
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(ExecuteAndPending) -> bool {
        return SLT::SLTNativeFunctions::ExecuteStepAndPending(VirtualMachine, StackID);
    };
#pragma endregion

#pragma region CleanupThreadContext Papyrus
    PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(CleanupThreadContext) {
            SLT::SLTNativeFunctions::CleanupThreadContext(VirtualMachine, StackID);
    };
#pragma endregion

}
#pragma endregion

#pragma region SLTNativeFunctions definition
void SLTNativeFunctions::Pung(RE::BSScript::Internal::VirtualMachine* vm, 
                                const RE::VMStackID stackId) {
    // technically this should be sufficient to populate the caller if it is possible
    std::ignore = SLTStackAnalyzer::GetFullContextInfo(stackId);
}

void SLTNativeFunctions::SetLibrariesForExtensionAllowed(std::string extensionKey, 
                                        bool allowed) {
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    if (funlib) {
        funlib->enabled = allowed;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, allowed);
    }
}

bool SLTNativeFunctions::PrepareContextForTargetedScript(RE::Actor* targetActor, 
                                        std::string scriptName) {
    if (!targetActor || scriptName.empty()) {
        logger::error("Invalid parameters for PrepareContextForTargetedScript");
        return false;
    }
    
    auto& manager = ContextManager::GetSingleton();
    return static_cast<std::int32_t>(manager.StartSLTScript(targetActor, scriptName));
}

bool SLTNativeFunctions::ExecuteStepAndPending(RE::BSScript::Internal::VirtualMachine* vm, 
                                const RE::VMStackID stackId) {
    if (!vm) return false;
    
    auto contextInfo = SLTStackAnalyzer::GetFullContextInfo(stackId);
    
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
    return context->ExecuteNextStep();
}

void SLTNativeFunctions::CleanupThreadContext(RE::BSScript::Internal::VirtualMachine* vm, 
                            const RE::VMStackID stackId) {
    if (!vm) return;
    
    auto contextInfo = SLTStackAnalyzer::GetFullContextInfo(stackId);
    
    if (!contextInfo.isValid || contextInfo.contextId == 0) {
        logger::error("CleanupThreadContext called without valid threadContextHandle property");
        return;
    }
    
    auto& manager = ContextManager::GetSingleton();
    manager.CleanupContext(contextInfo.contextId);
    
    logger::info("Cleaned up context {} from authenticated script call", contextInfo.contextId);
}

std::int32_t SLTNativeFunctions::GetActiveScriptCount() {
    return static_cast<std::int32_t>(ContextManager::GetSingleton().GetActiveContextCount());
}
    
SLTSessionId SLTNativeFunctions::GetSessionId() {
    if (!sessionIdGenerated) {
        GenerateNewSessionId();
    }
    return sessionId;
}


bool SLTNativeFunctions::IsLoaded() {
    return true;
}

std::string SLTNativeFunctions::GetTranslatedString(const std::string input) {
    auto sfmgr = RE::BSScaleformManager::GetSingleton();
    if (!(sfmgr)) {
        return input;
    }

    auto gfxloader = sfmgr->loader;
    if (!(gfxloader)) {
        return input;
    }

    auto translator =
        (RE::BSScaleformTranslator*) gfxloader->GetStateBagImpl()->GetStateAddRef(RE::GFxState::StateType::kTranslator);

    if (!(translator)) {
        return input;
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
    return input;
}

std::vector<std::string> SLTNativeFunctions::Tokenize(const std::string input) {
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

bool SLTNativeFunctions::DeleteTrigger(std::string extKeyStr, std::string trigKeyStr) {
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
        trigKeyStr += ".json";
    }

    fs::path filePath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "extensions" / extKeyStr / trigKeyStr;

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

RE::TESForm* SLTNativeFunctions::FindFormByEditorId(const std::string a_editorID) {
    return RE::TESForm::LookupByEditorID(a_editorID);
}

bool SLTNativeFunctions::SmartEquals(std::string a, std::string b) {
    return SmartComparator::SmartEquals(a, b);
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
    GenerateNewSessionId();
});

OnPostLoadGame([]{
    GenerateNewSessionId();
});


OnPostLoadGame([]{
    logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::SLTNativeFunctions::GetSessionId());
})

OnDeleteGame([]{
    logger::info("{} ending session {}", SystemUtil::File::GetPluginName(), SLT::SLTNativeFunctions::GetSessionId());
})
#pragma endregion

};



#pragma pop()