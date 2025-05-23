using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

#include <REX/W32.h>

#pragma push(warning)
#pragma warning(disable:4100)
namespace plugin {
    namespace Util {

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

        
        std::int32_t sessionId;

        void GenerateNewSessionId() {
            static std::random_device rd;
            static std::mt19937 engine(rd());
            static std::uniform_int_distribution<std::int32_t> dist(std::numeric_limits<std::int32_t>::min(),
                                                                    std::numeric_limits<std::int32_t>::max());
            sessionId = dist(engine);
        }

        static std::unordered_map<std::string_view, std::string_view> functionScriptCache;

        /*
        RE::BSTSmartPointer<RE::BSScript::Object> GetScriptObject_AME(RE::ActiveEffect* ae) {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            auto* handlePolicy = vm->GetObjectHandlePolicy();
            auto handle = handlePolicy->GetHandleForObject(ae->VMTYPEID, ae);

            RE::BSFixedString bsScriptName("sl_triggersCmd");

            auto it = vm->attachedScripts.find(handle);
            if (it == vm->attachedScripts.end()) {
                spdlog::error("{}: vm->attachedScripts couldn't find handle[{}] scriptName[{}]", __func__, handle, bsScriptName.c_str());
                return nullptr;
            }

            for (std::uint32_t i = 0; i < it->second.size(); i++) {
                auto& attachedScript = it->second[i];
                if (attachedScript) {
                    auto* script = attachedScript.get();
                    if (script) {
                        auto info = script->GetTypeInfo();
                        if (info) {
                            if (info->name == bsScriptName) {
                                spdlog::trace("script[{}] found attached to handle[{}]", bsScriptName.c_str(), handle);
                                RE::BSTSmartPointer<RE::BSScript::Object> result(script);
                                return result;
                            }
                        }
                    }
                }
            }

            return nullptr;
        }
        */

        bool PrecacheLibraries(std::vector<RE::BSFixedString> _scriptnames) {
            if (_scriptnames.empty()) {
                spdlog::info("PrecacheLibraries: _scriptnames was empty");
                return false;
            }
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeinfoptr;
            for (const auto& _scriptname: _scriptnames) {
                bool success = vm->GetScriptObjectType1(_scriptname, typeinfoptr);

                if (!success) {
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

            return true;
        }

        bool RunOperationOnActor(/*std::vector<RE::BSFixedString> _scriptnames,*/ RE::Actor* _cmdTargetActor, RE::ActiveEffect* _cmdPrimary,
                                 std::vector<RE::BSFixedString> _param) {
            bool success = false;

            if (_cmdPrimary && _cmdTargetActor) {
                RE::BSTSmartPointer<RE::BSScript::ObjectTypeInfo> typeinfoptr;
                auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
                auto* callbackArgs = RE::MakeFunctionArguments();
                auto vmhandle = vm->GetObjectHandlePolicy()->GetHandleForObject(_cmdPrimary->VMTYPEID, _cmdPrimary);
                RE::BSFixedString callbackEvent("OnSetOperationCompleted");

                auto resultCallback = RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor>(
                    new VoidCallbackFunctor([vm, vmhandle, callbackEvent, callbackArgs]() {
                        SKSE::GetTaskInterface()->AddTask(
                            [vm, vmhandle, callbackEvent, callbackArgs]() {
                            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> setterCallback{nullptr};

                            vm->SendEvent(vmhandle, callbackEvent, RE::MakeFunctionArguments());
                        });
                    })
                );

                auto* operationArgs =
                    RE::MakeFunctionArguments(static_cast<RE::Actor*>(_cmdTargetActor), static_cast<RE::ActiveEffect*>(_cmdPrimary),
                                              static_cast<std::vector<RE::BSFixedString>>(_param));

                if (_param.size() > 0) {
                    auto cachedIt = functionScriptCache.find(_param[0]);
                    if (cachedIt != functionScriptCache.end()) {
                        auto& cachedScript = cachedIt->second;
                        success = vm->DispatchStaticCall(cachedScript, _param[0], operationArgs, resultCallback);
                        return success;
                    }
                } else {
                    spdlog::error("RunOperationOnActor: zero-length _param is not allowed");
                }

                /*
                for (const auto& _scriptname: _scriptnames) {
                    success = vm->GetScriptObjectType1(_scriptname, typeinfoptr);

                    if (!success) {
                        continue;
                    }

                    success = false;

                    int numglobs = typeinfoptr->GetNumGlobalFuncs();
                    auto globiter = typeinfoptr->GetGlobalFuncIter();

                    for (int i = 0; i < numglobs; i++) {
                        if (_param[0] == globiter[i].func->GetName()) {
                            functionScriptCache[_param[0]] = _scriptname;

                            success = vm->DispatchStaticCall(_scriptname, _param[0], operationArgs, resultCallback);

                            return success;
                        }
                    }
                }*/
            }

            return success;
        }

        std::string TrimString(const std::string& str) {
            size_t first = str.find_first_not_of(" \t\n\r");
            if (first == std::string::npos)
                return "";  // this still trims whitespace-only lines to ""
            size_t last = str.find_last_not_of(" \t\n\r");
            return str.substr(first, last - first + 1);
        }

        std::vector<std::string> SplitLinesTrimmed(const std::string& content) {
            std::vector<std::string> lines;
            size_t start = 0;
            size_t i = 0;
            size_t len = content.length();

            while (i < len) {
                if (content[i] == '\r' || content[i] == '\n') {
                    std::string line = content.substr(start, i - start);
                    line = TrimString(line);
                    lines.push_back(line);

                    if (content[i] == '\r' && i + 1 < len && content[i + 1] == '\n') {
                        i += 2;  // Windows CRLF
                    } else {
                        i += 1;  // CR or LF
                    }
                    start = i;
                } else {
                    i += 1;
                }
            }

            // Add the last line but only if not empty (we are only tracking empty lines to make sense of
            // line numbers referring to code... if no code remains past this point, we no longer care
            if (start <= len) {
                std::string lastLine = content.substr(start);
                lastLine = TrimString(lastLine);
                if (!lastLine.empty()) {
                    lines.push_back(lastLine);
                }
            }

            return lines;
        }


        std::string GetTranslatedString(const std::string& input) {
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

        std::vector<RE::ActiveEffect*> GetActiveEffectsForActor(RE::Actor* _theActor) {
            std::vector<RE::ActiveEffect*> activeEffects;

            if (_theActor) {
                auto actorEffects = _theActor->AsMagicTarget()->GetActiveEffectList();

                for (const auto& effect: *actorEffects) {
                    activeEffects.push_back(effect);
                }
            }

            return activeEffects;
        }

        std::vector<std::string> SplitLines(const std::string& content) {
            std::vector<std::string> lines;
            size_t start = 0;
            size_t i = 0;
            size_t len = content.length();

            while (i < len) {
                if (content[i] == '\r') {
                    if (i > start) {
                        lines.push_back(content.substr(start, i - start));
                    }
                    if (i + 1 < len && content[i + 1] == '\n') {
                        i += 2;  // Windows CRLF
                    } else {
                        i += 1;  // Classic Mac CR
                    }
                    start = i;
                } else if (content[i] == '\n') {
                    if (i > start) {
                        lines.push_back(content.substr(start, i - start));  // Unix LF
                    }
                    i += 1;
                    start = i;
                } else {
                    i += 1;
                }
            }

            // Add last line if there's any remaining
            if (start < len) {
                lines.push_back(content.substr(start));
            }

            return lines;
        }

        std::vector<std::string> Tokenize(const std::string& input) {
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

            //spdlog::info("With input({}) Token count: {}", input, tokens.size());
            for (i = 0; i < tokens.size(); ++i) {
                //spdlog::info("Token {}: [{}]", i, tokens[i]);
            }
            return tokens;
        }

        namespace fs = std::filesystem;

        bool IsValidPathComponent(const std::string& input) {
            // Disallow characters illegal in Windows filenames
            static const std::regex validPattern(R"(^[^<>:"/\\|?*\x00-\x1F]+$)");
            return std::regex_match(input, validPattern);
        }

        bool DeleteTrigger(RE::BSFixedString extensionKey, RE::BSFixedString triggerKey) {
            std::string extKeyStr = extensionKey.c_str();
            std::string trigKeyStr = triggerKey.c_str();

            if (!IsValidPathComponent(extKeyStr) || !IsValidPathComponent(trigKeyStr)) {
                spdlog::error("Invalid characters in extensionKey ({}) or triggerKey ({})", extKeyStr, trigKeyStr);
                return false;
            }

            if (extKeyStr.empty() || trigKeyStr.empty()) {
                spdlog::error("extensionKey and triggerKey may not be empty extensionKey[{}]  triggerKey[{}]", extKeyStr, trigKeyStr);
                return false;
            }

            // Ensure triggerKey ends with ".json"
            if (trigKeyStr.length() < 5 || trigKeyStr.substr(trigKeyStr.length() - 5) != ".json") {
                trigKeyStr += ".json";
            }

            fs::path filePath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "extensions" / extKeyStr / trigKeyStr;

            std::error_code ec;

            if (!fs::exists(filePath, ec)) {
                spdlog::info("Trigger file not found: {}", filePath.string());
                return false;
            }

            if (fs::remove(filePath, ec)) {
                spdlog::info("Successfully deleted: {}", filePath.string());
                return true;
            } else {
                spdlog::info("Failed to delete {}: {}", filePath.string(), ec.message());
                return false;
            }
        }


        bool isNumeric(const std::string& str, double& outValue) {
            const char* begin = str.c_str();
            const char* end = begin + str.size();

            auto result = std::from_chars(begin, end, outValue);
            return result.ec == std::errc() && result.ptr == end;
        }

        // Main logic: numeric if both parse cleanly, else string compare
        bool SmartEquals(const std::string& a, const std::string& b) {
            double aNum = 0.0, bNum = 0.0;
            bool aIsNum = isNumeric(a, aNum);
            bool bIsNum = isNumeric(b, bNum);

            bool outcome = false;
            if (aIsNum && bIsNum) {
                outcome = (std::fabs(aNum - bNum) < 1e-9);  // safe float comparison
            } else {
                outcome = (a == b);
            }

            return outcome;
        }


        RE::TESForm* FindFormByEditorId(const std::string_view& a_editorID) {
            RE::TESForm* result = RE::TESForm::LookupByEditorID(a_editorID);
            return result;
        }

        std::vector<RE::TESObjectREFR*> FindCOCHeadingMarkers(RE::TESObjectCELL* cell) {
            std::vector<RE::TESObjectREFR*> results;

            if (!cell) {
                return results;
            }

            const auto& references = cell->GetRuntimeData().references;
            if (!references.size()) {
                return results;
            }

            for (auto& handle: references) {
                auto ref = handle.get();
                if (!ref) {
                    continue;
                }

                auto base = ref->GetBaseObject();
                if (!base || base->GetFormType() != RE::FormType::Static || base->GetFormID() != 52) {
                    continue;
                }

                results.push_back(ref);

                /*
                if (!editorID.empty() && _strnicmp(editorID.c_str(), "cocheadingmarker", 9) == 0) {
                    results.push_back(ref);
                }
                */
            }

            return results;
        }

        std::vector<RE::TESObjectREFR*> FindCOCHeadingMarkersForLocation(RE::BGSLocation* location) {
            std::vector<RE::TESObjectREFR*> results;

            if (!location) {
                return results;
            }

            for (auto& sref : location->specialRefs) {
                RE::TESForm* form = RE::TESForm::LookupByID(sref.refData.refID);
                if (form) {
                    RE::TESObjectREFR* orf = form->As<RE::TESObjectREFR>();
                    if (orf) {
                        auto base = orf->GetBaseObject();
                        if (!base || base->GetFormType() != RE::FormType::Static || base->GetFormID() != 52) {
                            continue;
                        }

                        results.push_back(orf);
                    } else {
                        spdlog::info("we are orfaned");
                    }
                }
            }

            /*
            if (!cell) {
                return results;
            }

            const auto& references = cell->GetRuntimeData().references;
            if (!references.size()) {
                return results;
            }

            for (auto& handle: references) {
                auto ref = handle.get();
                if (!ref) {
                    continue;
                }

                auto base = ref->GetBaseObject();
                if (!base || base->GetFormType() != RE::FormType::Static || base->GetFormID() != 52) {
                    continue;
                }

                results.push_back(ref);

                
                //if (!editorID.empty() && _strnicmp(editorID.c_str(), "cocheadingmarker", 9) == 0) {
                 //   results.push_back(ref);
               // }
                
            }
        */

            return results;
        }

        RE::BGSLocation* GetLocationFromCell(RE::TESObjectCELL* cell) {
            if (!cell) {
                return nullptr;
            }

            return cell->GetLocation();
        }




    }  // namespace Util

    namespace Papyrus {

        bool PrecacheLibraries(RE::StaticFunctionTag*, std::vector<RE::BSFixedString> _scriptnames) {
            return Util::PrecacheLibraries(_scriptnames);
        }

        bool RunOperationOnActor(RE::StaticFunctionTag*, /*std::vector<RE::BSFixedString> _scriptnames,*/ RE::Actor* _cmdTargetActor,
                                 RE::ActiveEffect* _cmdPrimary, std::vector<RE::BSFixedString> _param) {
            return Util::RunOperationOnActor(/*_scriptnames,*/ _cmdTargetActor, _cmdPrimary, _param);
        }

        std::vector<std::string> SplitLinesTrimmed(RE::StaticFunctionTag*, RE::BSFixedString _fileString) {
            return Util::SplitLinesTrimmed(_fileString.c_str());
        }

        std::string GetTranslatedString(RE::StaticFunctionTag*, RE::BSFixedString _translationKey) {
            std::string somevalue = Util::GetTranslatedString(_translationKey.c_str());
            return somevalue;
        }

        std::vector<RE::ActiveEffect*> GetActiveMagicEffectsForActor(RE::StaticFunctionTag*, RE::Actor* _theActor) {
            return Util::GetActiveEffectsForActor(_theActor);
        }

        bool IsLoaded(RE::StaticFunctionTag*) {
            return true;
        }

        std::vector<std::string> SplitLines(RE::StaticFunctionTag*, RE::BSFixedString _fileString) {
            return Util::SplitLines(_fileString.c_str());
        }

        std::vector<std::string> Tokenize(RE::StaticFunctionTag*, RE::BSFixedString _tokenString) {
            return Util::Tokenize(_tokenString.c_str());
        }

        bool DeleteTrigger(RE::StaticFunctionTag*, RE::BSFixedString extensionKey, RE::BSFixedString triggerKey) {
            return Util::DeleteTrigger(extensionKey, triggerKey);
        }

        int GetSessionId(RE::StaticFunctionTag*) {
            return plugin::Util::sessionId;
        }

        bool SmartEquals(RE::StaticFunctionTag*, RE::BSFixedString a, RE::BSFixedString b) {
            return Util::SmartEquals(a.c_str(), b.c_str());
        }

        RE::TESForm* FindFormByEditorId(RE::StaticFunctionTag*, RE::BSFixedString bs_editorId) {
            return Util::FindFormByEditorId(bs_editorId);
        }

        RE::BGSLocation* GetLocationFromCell(RE::StaticFunctionTag*, RE::TESObjectCELL* cell) {
            return Util::GetLocationFromCell(cell);
        }

        std::vector<RE::TESObjectREFR*> FindCOCHeadingMarkers(RE::StaticFunctionTag*, RE::TESObjectCELL* cell) {
            return Util::FindCOCHeadingMarkers(cell);
        }

        std::vector<RE::TESObjectREFR*> FindCOCHeadingMarkersForLocation(RE::StaticFunctionTag*, RE::BGSLocation* location) {
            return Util::FindCOCHeadingMarkersForLocation(location);
        }

        bool Register(RE::BSScript::IVirtualMachine* vm) {
            // I may have untreated problems?? :)
            // but the alignment... so pretty...
            spdlog::info("registering");
            vm->RegisterFunction("_PrecacheLibraries", "sl_triggers_internal", PrecacheLibraries, true);
            vm->RegisterFunction("_RunOperationOnActor", "sl_triggers_internal", RunOperationOnActor);
            vm->RegisterFunction("_SplitLinesTrimmed", "sl_triggers_internal", SplitLinesTrimmed, true);
            vm->RegisterFunction("_GetTranslatedString", "sl_triggers_internal", GetTranslatedString);
            vm->RegisterFunction("_GetActiveMagicEffectsForActor", "sl_triggers_internal", GetActiveMagicEffectsForActor);
            vm->RegisterFunction("_IsLoaded", "sl_triggers_internal", IsLoaded, true);
            vm->RegisterFunction("_SplitLines", "sl_triggers_internal", SplitLines, true);
            vm->RegisterFunction("_Tokenize", "sl_triggers_internal", Tokenize, true);
            vm->RegisterFunction("_DeleteTrigger", "sl_triggers_internal", DeleteTrigger, true);
            vm->RegisterFunction("_GetSessionId", "sl_triggers_internal", GetSessionId, true);
            vm->RegisterFunction("_SmartEquals", "sl_triggers_internal", SmartEquals, true);
            vm->RegisterFunction("_FindFormByEditorId", "sl_triggers_internal", FindFormByEditorId);
            vm->RegisterFunction("_GetLocationFromCell", "sl_triggers_internal", GetLocationFromCell);
            vm->RegisterFunction("_FindCOCHeadingMarkers", "sl_triggers_internal", FindCOCHeadingMarkers);
            vm->RegisterFunction("_FindCOCHeadingMarkersForLocation", "sl_triggers_internal", FindCOCHeadingMarkersForLocation);
            spdlog::info("registered");

            return true;
        }
    }  // namespace Papyrus
}  // namespace plugin


OnAfterSKSEInit([]{
    spdlog::info("Registering functions");
    SKSE::GetPapyrusInterface()->Register(plugin::Papyrus::Register);
});

OnNewGame([]{
    plugin::Util::GenerateNewSessionId();
})

OnPostLoadGame([]{
    plugin::Util::GenerateNewSessionId();
})

#pragma pop()