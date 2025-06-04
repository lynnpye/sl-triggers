#pragma once

#include <shared_mutex>

namespace SLT {

class FrameContext;

class SLTExtension {
public:
    RE::TESQuest* quest;
    bool enabled;
    std::string key;
    std::string friendlyName;
    std::int32_t priority;

    explicit SLTExtension(RE::TESQuest* _quest, bool _enabled, std::string _key, std::string _friendlyName, std::int32_t _priority)
        : quest(_quest), enabled(_enabled), key(_key), friendlyName(_friendlyName), priority(_priority)
        {}

    std::optional<RE::TESForm*> CustomResolveForm(std::string_view token, FrameContext* frame);
};

class SLTExtensionTracker {
private:
    static std::shared_mutex questsMutex;
    static std::vector<std::unique_ptr<SLTExtension>> quests;

    static std::unordered_map<std::string, SLTExtension*> questsByKey;
    static std::unordered_map<RE::TESQuest*, SLTExtension*> questsByQuest;
    
    SLTExtensionTracker() = delete;

public:
    template <typename Func>
    static auto ReadData(Func&& fn) {
        try {
            std::shared_lock<std::shared_mutex> lock(questsMutex);
            return fn(quests, questsByKey, questsByQuest);
        } catch (...) {
            throw;
        }
    }

    template <typename Func>
    static auto WriteData(Func&& fn) {
        try {
            std::unique_lock<std::shared_mutex> lock(questsMutex);
            return fn(quests, questsByKey, questsByQuest);
        } catch (...) {
            throw;
        }
    }

    static SLTExtension* GetExtension(std::string_view extensionKey) {
        if (extensionKey.size() > 0) {
            std::shared_lock lock(questsMutex);
            auto it = questsByKey.find(std::string(extensionKey));
            if (it != questsByKey.end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    static bool IsEnabled(std::string_view extensionKey) {
        if (auto* ext = GetExtension(extensionKey)) {
            return ext->enabled;
        } else {
            logger::warn("No extension with key ({})", extensionKey);
        }
        return false;
    }
    
    static void AddQuest(RE::TESQuest* quest, RE::VMStackID stackId);

    // no need for RemoveQuest; SLTExtensionTracker is essentially transient itself
    // being recreated wholly at game launch
};


}