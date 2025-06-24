#include "engine.h"

namespace SLT {
    
#pragma region OperationRunner
bool OperationRunner::RunOperationOnActor(RE::Actor* targetActor, 
                                         RE::ActiveEffect* cmdPrimary, 
                                         const std::vector<RE::BSFixedString>& params) {
    if (!cmdPrimary || !targetActor || params.empty()) {
        logger::error("RunOperationOnActor: Invalid parameters cmdPrimary({}) targetActor({}) params.empty({})", !cmdPrimary, !targetActor, params.empty());
        return false;
    }
    
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) {
        logger::error("RunOperationOnActor: Failed to get VM singleton");
        return false;
    }
    
    auto cachedIt = FunctionLibrary::functionScriptCache.find(params[0].c_str());
    if (cachedIt == FunctionLibrary::functionScriptCache.end()) {
        logger::error("RunOperationOnActor: Unable to find operation {} in function library cache", params[0].c_str());
        return false;
    }

    auto voidCallback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>{};
    
    auto* operationArgs = RE::MakeFunctionArguments(
        static_cast<RE::Actor*>(targetActor), 
        static_cast<RE::ActiveEffect*>(cmdPrimary),
        static_cast<std::vector<RE::BSFixedString>>(params)
    );
    
    auto& cachedScript = cachedIt->second;
    bool success = vm->DispatchStaticCall(cachedScript, params[0], operationArgs, voidCallback);
    
    if (!success) {
        logger::error("RunOperationOnActor: Failed to dispatch static call for operation {}", params[0].c_str());
    }
    
    return success;
}

bool OperationRunner::RunOperationOnActor(RE::Actor* targetActor, 
                                         RE::ActiveEffect* cmdPrimary, 
                                         const std::vector<std::string>& params) {
    if (params.empty()) {
        return false;
    }
    
    std::vector<RE::BSFixedString> bsParams;
    bsParams.reserve(params.size());
    for (const auto& param : params) {
        bsParams.emplace_back(param);
    }
    
    return RunOperationOnActor(targetActor, cmdPrimary, bsParams);
}
#pragma endregion

#pragma region Function Libraries definition
const std::string_view FunctionLibrary::SLTCmdLib = "sl_triggersCmdLibSLT";
std::vector<std::unique_ptr<FunctionLibrary>> FunctionLibrary::g_FunctionLibraries;

std::unordered_map<std::string, std::string, CaseInsensitiveHash, CaseInsensitiveEqual> FunctionLibrary::functionScriptCache;

FunctionLibrary* FunctionLibrary::ByExtensionKey(std::string_view _extensionKey) {
    auto it = std::find_if(g_FunctionLibraries.begin(), g_FunctionLibraries.end(),
        [_extensionKey = std::string(_extensionKey)](const std::unique_ptr<FunctionLibrary>& lib) {
            return Util::String::iEquals(lib->extensionKey, _extensionKey);
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

bool FunctionLibrary::PrecacheLibraries() {
    logger::info("PrecacheLibraries starting");
    FunctionLibrary::GetFunctionLibraries();
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

    logger::info("PrecacheLibraries completed");
    return true;
}

}

#pragma endregion
