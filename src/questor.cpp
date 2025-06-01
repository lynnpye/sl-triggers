#include "questor.h"

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
}