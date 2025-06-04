#include "questor.h"
#include "contexting.h"
#include "coroutines.h"

using namespace RE;
using namespace RE::BSScript;
using namespace RE::BSScript::Internal;

namespace {
bool ScriptInheritsFrom(ObjectTypeInfo* typeInfo, std::string_view baseScriptName) {
    if (!typeInfo) return false;
    
    std::string currentTypeName = typeInfo->GetName();
    if (currentTypeName == baseScriptName) {
        return true;
    }
    
    auto* parentInfo = typeInfo->GetParent();
    if (parentInfo) {
        return ScriptInheritsFrom(parentInfo, baseScriptName);
    }
    
    return false;
}

bool QuestInheritsFrom(RE::TESQuest* quest, std::string_view baseScriptName,
    VirtualMachine* vm = nullptr, IObjectHandlePolicy* handlePolicy = nullptr, ObjectBindPolicy* bindPolicy = nullptr) {
    if (!quest) {
        logger::error("Quest is required to be non-null");
        return false;
    }
    if (baseScriptName.empty()) {
        logger::error("baseScriptName must be non-empty (and ostensibly valid)");
        return false;
    }
    if (!vm) {
        vm = VirtualMachine::GetSingleton();
        if (!vm) {
            logger::error("Failed to get VM singleton");
            return false;
        }
    }
    if (!handlePolicy) {
        handlePolicy = vm->GetObjectHandlePolicy();
        if (!handlePolicy) {
            logger::error("Failed to get VM handle policy");
            return false;
        }
    }
    if (!bindPolicy) {
        bindPolicy = vm->GetObjectBindPolicy();
        if (!bindPolicy) {
            logger::error("Failed to get VM bind policy");
            return false;
        }
    }

    BSTSmartPointer<Object> questObject;
    VMHandle questHandle = handlePolicy->GetHandleForObject(TESQuest::FORMTYPE, quest);

    if (!questHandle) {
        logger::warn("Failed to get VM handle for quest ({})", Util::String::ToHex(quest->GetFormID()));
        return false;
    }

    bindPolicy->BindObject(questObject, questHandle);

    if (!questObject) {
        logger::warn("Failed to bind quest to object ({})", Util::String::ToHex(quest->GetFormID()));
        return false;
    }

    auto* questTypeInfo = questObject->GetTypeInfo();
    if (!questTypeInfo) {
        logger::warn("Failed to get quest type info ({})", Util::String::ToHex(quest->GetFormID()));
        return false;
    }
    
    return ScriptInheritsFrom(questTypeInfo, baseScriptName);
}

std::vector<RE::TESQuest*> FindQuestsWithScriptInheritance(std::string_view baseScriptName) {
    std::vector<RE::TESQuest*> matchingQuests;
    
    auto* dataHandler = RE::TESDataHandler::GetSingleton();
    if (!dataHandler) {
        return matchingQuests;
    }
    auto* vm = VirtualMachine::GetSingleton();
    if (!vm) {
        logger::error("Failed to get VM singleton");
        return matchingQuests;
    }
    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) {
        logger::error("Failed to get VM handle policy");
        return matchingQuests;
    }
    auto* bindPolicy = vm->GetObjectBindPolicy();
    if (!bindPolicy) {
        logger::error("Failed to get VM bind policy");
        return matchingQuests;
    }
    
    auto& quests = dataHandler->GetFormArray<RE::TESQuest>();
    VMHandle questHandle;
    BSTSmartPointer<Object> questObject;
    
    for (auto* quest : quests) {
        if (QuestInheritsFrom(quest, baseScriptName)) {
            matchingQuests.push_back(quest);
        }
    }
    
    return matchingQuests;
}


struct ExtensionContextInfo {
    RE::TESQuest* quest;
    bool enabled;
    std::string key;
    std::string friendlyName;
    std::int32_t priority;
    bool isValid = false;
};

ExtensionContextInfo GetExtensionContextInfo(RE::VMStackID stackId) {
    ExtensionContextInfo result;
    
    RE::TESQuest* quest;
    std::string extensionKey;
    std::string friendlyName;
    std::int32_t priority;
    bool isEnabled;
    
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
    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) {
        logger::error("Unable to get handle policy");
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
    //LogVariableInfo(self, "frame self");
    
    auto selfObject = self.GetObject();
    if (!selfObject) {
        logger::warn("Failed to get self object for stack ID: 0x{:X}", stackId);
        return result;
    }

    RE::VMHandle objHandle = selfObject->GetHandle();
    if (!handlePolicy->IsHandleObjectAvailable(objHandle)) {
        logger::error("HandlePolicy says handle object not available!");
        // maybe return result;
    }

    quest = static_cast<RE::TESQuest*>(handlePolicy->GetObjectForHandle(RE::ActiveEffect::VMTYPEID, objHandle));
    if (!quest) {
        logger::error("Unable to obtain quest from selfObject with RE::VMHandle({})", objHandle);
        return result;
    }

    RE::BSFixedString pn_SLTExtensionKey("SLTExtensionKey");
    RE::BSFixedString pn_SLTFriendlyName("SLTFriendlyName");
    RE::BSFixedString pn_SLTPriority("SLTPriority");
    RE::BSFixedString pn_IsEnabled("IsEnabled");

    RE::BSScript::Variable* var_SLTExtensionKey;
    RE::BSScript::Variable* var_SLTFriendlyName;
    RE::BSScript::Variable* var_SLTPriority;
    RE::BSScript::Variable* var_IsEnabled;

    var_SLTExtensionKey = selfObject->GetProperty(pn_SLTExtensionKey);
    var_SLTFriendlyName = selfObject->GetProperty(pn_SLTFriendlyName);
    var_SLTPriority = selfObject->GetProperty(pn_SLTPriority);
    var_IsEnabled = selfObject->GetProperty(pn_IsEnabled);
    
    if (var_SLTExtensionKey && var_SLTExtensionKey->IsString()) {
        extensionKey = var_SLTExtensionKey->GetString();
    }
    if (var_SLTFriendlyName && var_SLTFriendlyName->IsString()) {
        friendlyName = var_SLTFriendlyName->GetString();
    }
    if (var_SLTPriority && var_SLTPriority->IsInt()) {
        priority = var_SLTPriority->GetSInt();
    }
    if (var_IsEnabled && var_IsEnabled->IsBool()) {
        isEnabled = var_IsEnabled->GetBool();
    }

    if (!extensionKey.empty()) {
        result = {
            .quest = quest,
            .enabled = isEnabled,
            .key = extensionKey,
            .friendlyName = friendlyName,
            .priority = priority,
            .isValid = true
        };
    }
    
    return result;
}

}

namespace SLT {

using namespace SLT;

/*
place the quest specific resolution code here.

when this code is called:
- it should presumably be called from within the engine during step execution, thus on a background worker thread
- it should somehow start the correct coroutine, set up the promise, then wait
- it is further assumed that an implementing extension will have, if it returns true, called SetCustomResolveFormResult on the plugin
- as a result, all this function should have to do is start the coroutine and wait for it to finish
- then it should return the same bool result received from the Papyrus function
- it is assumed that anytime this function is called, the caller is aware that the result will be in frame->customResolveFormResult, so you should not have to deal with that here
*/
std::optional<RE::TESForm*> SLTExtension::CustomResolveForm(std::string_view token, FrameContext* frame) {
    auto* targetActor = frame->thread->target->AsActor();
    if (!targetActor) {
        return std::nullopt; // Skip if we don't have a valid target actor
    }
    
    ThreadContextHandle threadHandle = frame->thread->threadContextHandle;
    
    // Use FormResolver to make the synchronous call
    auto* resolvedForm = FormResolver::CallCustomResolveForm(
        quest, 
        token, 
        targetActor, 
        threadHandle
    );
    
    if (resolvedForm) {
        // Extension successfully resolved the token
        logger::debug("Extension '{}' resolved token '{}' to form 0x{:X}", 
                        key, token, resolvedForm->GetFormID());
        return resolvedForm;
    }
    return std::nullopt;
}

std::shared_mutex SLTExtensionTracker::questsMutex;
std::vector<std::unique_ptr<SLTExtension>> SLTExtensionTracker::quests;
std::unordered_map<std::string, SLTExtension*> SLTExtensionTracker::questsByKey;
std::unordered_map<RE::TESQuest*, SLTExtension*> SLTExtensionTracker::questsByQuest;

void SLTExtensionTracker::AddQuest(RE::TESQuest* quest, RE::VMStackID stackId) {
    if (!quest) return;
    
    WriteData([quest, stackId](auto& quests, auto& questsByKey, auto& questsByQuest) -> void {
        auto qContext = GetExtensionContextInfo(stackId);

        auto it = questsByQuest.find(quest);
        if (it != questsByQuest.end()) {
            logger::warn("Tried to add quest tracking for already tracked quest ([{}] {}), updating enabled state", quest->GetFormID(), quest->GetName());
            auto* sltex = it->second;
            sltex->enabled = qContext.enabled;
            return;
        }

        quests.push_back(std::make_unique<SLTExtension>(qContext.quest, qContext.enabled, qContext.key, qContext.friendlyName, qContext.priority));
        questsByKey[qContext.key] = quests.back().get();
        questsByQuest[qContext.quest] = quests.back().get();

        std::sort(quests.begin(), quests.end(), 
            [](const std::unique_ptr<SLTExtension>& a, const std::unique_ptr<SLTExtension>& b) {
                return a->priority < b->priority;
            });
    });
}

}