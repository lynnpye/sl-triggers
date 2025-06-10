#pragma push(warning)
#pragma warning(disable:4100)


namespace SLT {

// SLTNativeFunctions implementation remains the same below

#pragma region SLTNativeFunctions definition

// Non-latent Functions
void SLTNativeFunctions::CleanupThreadContext(PAPYRUS_NATIVE_DECL, ForgeHandle threadContextHandle) {
    GlobalContext::GetSingleton().CleanupContext(threadContextHandle);
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

bool SLTNativeFunctions::PrepareContextForTargetedScript(PAPYRUS_NATIVE_DECL, RE::Actor* targetActor, 
                                        std::string_view scriptName) {
    auto& manager = GlobalContext::GetSingleton();
    return manager.StartSLTScript(targetActor, scriptName);
}

ForgeHandle SLTNativeFunctions::Pung(PAPYRUS_NATIVE_DECL, ForgeHandle threadContextHandle) {
    ForgeHandle outHandle = threadContextHandle;

    [&]()
    {
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
            if (!vm->GetStackByID(stackId, &stackPtr)) {
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
                    ForgeHandle cid = threadContextHandle;

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
                        auto* targetContext = TargetContext::CreateTargetContext(actor);

                        auto it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(),
                        [&](const ForgeHandle tchandle) -> bool {
                            auto* tcptr = ThreadContextManager::GetFromHandle(tchandle);
                            return tcptr && !tcptr->isClaimed && !tcptr->wasClaimed;
                        });

                        if (it == targetContext->threads.end()) {
                            it = std::find_if(targetContext->threads.begin(), targetContext->threads.end(),
                            [&](const ForgeHandle tchandle) -> bool {
                                auto* tcptr = ThreadContextManager::GetFromHandle(tchandle);
                                return tcptr && !tcptr->isClaimed;
                            });
                        }

                        if (it != targetContext->threads.end()) {
                            if (auto* threadCon = ThreadContextManager::GetFromHandle(*it)) {
                                threadCon->ame = ame;
                                outHandle = threadCon->GetHandle();
                            }
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
    }();

    return outHandle;
}

RE::TESForm* SLTNativeFunctions::ResolveFormVariable(PAPYRUS_NATIVE_DECL, ForgeHandle threadContextHandle, std::string_view token) {
    if (!vm) return nullptr;

    RE::TESForm* finalResolution = nullptr;
    if (GlobalContext::GetSingleton().ResolveFormVariable(threadContextHandle, token)) {
        auto* thread = ThreadContextManager::GetFromHandle(threadContextHandle);
        if (thread) {
            auto* frame = thread->GetCurrentFrame();
            if (frame)
                finalResolution = frame->customResolveFormResult;
        }
    }
    return finalResolution;
}

std::string SLTNativeFunctions::ResolveValueVariable(PAPYRUS_NATIVE_DECL, ForgeHandle threadContextHandle, std::string_view token) {
    if (!vm) return "";

    std::string finalResolution = GlobalContext::GetSingleton().ResolveValueVariable(threadContextHandle, token);
    return finalResolution;
}

bool SLTNativeFunctions::RunOperationOnActor(PAPYRUS_NATIVE_DECL, RE::Actor* cmdTarget, RE::ActiveEffect* cmdPrimary,
    std::vector<std::string> tokens) {
    return OperationRunner::RunOperationOnActor(cmdTarget, cmdPrimary, tokens);
}

void SLTNativeFunctions::SetExtensionEnabled(PAPYRUS_NATIVE_DECL, std::string_view extensionKey, bool enabledState) {
    //SLTExtensionTracker::SetEnabled(extensionKey, enabledState);
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    //SLTStackAnalyzer::Walk(stackId);
    if (funlib) {
        funlib->enabled = enabledState;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, enabledState);
    }
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


#pragma endregion

};

#pragma pop()