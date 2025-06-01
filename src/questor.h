#pragma once

#include <shared_mutex>

namespace SLT {

class SLTExtensionTracker {
private:
    std::vector<RE::TESQuest*> trackedQuests;
    std::string_view targetBaseScript;
    mutable std::shared_mutex questsMutex;
    
    SLTExtensionTracker(std::string_view baseScript) : targetBaseScript(baseScript) {}

public:
    static SLTExtensionTracker& GetSingleton() {
        static SLTExtensionTracker singleton(SLT::BASE_QUEST);
        return singleton;
    }
    
    std::vector<RE::TESQuest*> GetTrackedQuests() const {
        std::shared_lock lock(questsMutex);
        return trackedQuests;
    }
    
    bool IsQuestTracked(RE::TESQuest* quest) const {
        std::shared_lock lock(questsMutex);
        return std::find(trackedQuests.begin(), trackedQuests.end(), quest) != trackedQuests.end();
    }
    
    size_t GetTrackedQuestCount() const {
        std::shared_lock lock(questsMutex);
        return trackedQuests.size();
    }
    
    void AddQuest(RE::TESQuest* quest) {
        if (!quest) return;
        
        std::unique_lock lock(questsMutex);
        auto it = std::find(trackedQuests.begin(), trackedQuests.end(), quest);
        if (it == trackedQuests.end()) {
            trackedQuests.push_back(quest);
            logger::info("Added SLT extension quest: {}", quest->GetName());
        }
    }
    
    void RemoveQuest(RE::TESQuest* quest) {
        std::unique_lock lock(questsMutex);
        auto it = std::find(trackedQuests.begin(), trackedQuests.end(), quest);
        if (it != trackedQuests.end()) {
            trackedQuests.erase(it);
            logger::info("Removed SLT extension quest: {}", quest->GetName());
        }
    }
    
};


}