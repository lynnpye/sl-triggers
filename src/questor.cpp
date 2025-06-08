#include "questor.h"
#include "contexting.h"
#include "coroutines.h"

using namespace RE;
using namespace RE::BSScript;
using namespace RE::BSScript::Internal;

namespace {

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
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Invalid stackId: 0x{:X}", stackId);
        return result;
    }
    
    using RE::BSScript::Internal::VirtualMachine;
    using RE::BSScript::Stack;
    using RE::BSScript::Internal::CodeTasklet;
    using RE::BSScript::StackFrame;
    
    auto* vm = VirtualMachine::GetSingleton();
    if (!vm) {
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Failed to get VM singleton");
        return result;
    }
    auto* handlePolicy = vm->GetObjectHandlePolicy();
    if (!handlePolicy) {
        logger::error("\n\n\t\t\t!!!!!!!!!!!!!  Unable to get handle policy");
        return result;
    }
    
    RE::BSScript::Stack* stackPtr = nullptr;
    
    try {
        bool success = vm->GetStackByID(stackId, &stackPtr);
        
        if (!success) {
            logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  GetStackByID returned false for ID: 0x{:X}", stackId);
            return result;
        }
        
        if (!stackPtr) {
            logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  GetStackByID succeeded but returned null pointer for ID: 0x{:X}", stackId);
            return result;
        }
    } catch (const std::exception& e) {
        logger::error("\n\n\t\t\t!!!!!!!!!!!!!  Exception in GetStackByID: {}", e.what());
        return result;
    } catch (...) {
        logger::error("\n\n\t\t\t!!!!!!!!!!!!!  Unknown exception in GetStackByID for stackId: 0x{:X}", stackId);
        return result;
    }
    
    if (!stackPtr->owningTasklet) {
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Stack has no owning tasklet for ID: 0x{:X}", stackId);
        return result;
    }
    
    auto taskletPtr = stackPtr->owningTasklet;
    
    if (!taskletPtr->topFrame) {
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Tasklet has no top frame for stack ID: 0x{:X}", stackId);
        return result;
    }
    
    auto* frame = taskletPtr->topFrame;
    
    RE::BSScript::Variable& self = frame->self;
    if (!self.IsObject()) {
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Frame self is not an object for stack ID: 0x{:X}", stackId);
        return result;
    }
    //LogVariableInfo(self, "frame self");
    
    auto selfObject = self.GetObject();
    if (!selfObject) {
        logger::warn("\n\n\t\t\t!!!!!!!!!!!!!  Failed to get self object for stack ID: 0x{:X}", stackId);
        return result;
    }

    RE::VMHandle objHandle = selfObject->GetHandle();
    if (!handlePolicy->IsHandleObjectAvailable(objHandle)) {
        logger::error("\n\n\t\t\t!!!!!!!!!!!!!  HandlePolicy says handle object not available!");
        // maybe return result;
    }

    quest = static_cast<RE::TESQuest*>(handlePolicy->GetObjectForHandle(RE::TESQuest::FORMTYPE, objHandle));
    if (!quest) {
        logger::error("\n\n\t\t\t!!!!!!!!!!!!!  Unable to obtain quest from selfObject with RE::VMHandle({})", objHandle);
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
            auto* sltex = it->second;
            sltex->enabled = qContext.enabled;
            logger::warn("Tried to add quest tracking for already tracked quest key({}), updating enabled state to ({})", sltex->key, sltex->enabled);
            return;
        }

        quests.push_back(std::make_unique<SLTExtension>(qContext.quest, qContext.enabled, qContext.key, qContext.friendlyName, qContext.priority));
        auto* addedQuest = quests.back().get();
        logger::debug("Added quest tracking for key({}) friendlyName({}) priority({}) enabled({})", addedQuest->key, addedQuest->friendlyName, addedQuest->priority,
            addedQuest->enabled);
        questsByKey[qContext.key] = addedQuest;
        questsByQuest[qContext.quest] = addedQuest;

        std::sort(quests.begin(), quests.end(), 
            [](const std::unique_ptr<SLTExtension>& a, const std::unique_ptr<SLTExtension>& b) {
                return a->priority < b->priority;
            });
    });
}

}