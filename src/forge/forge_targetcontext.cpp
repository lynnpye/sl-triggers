

namespace SLT {

#pragma region TargetContext

std::string TargetContext::SetTargetVar(std::string_view name, std::string_view value) {
    if (name.empty()) {
        logger::warn("SetTargetVar: Variable name cannot be empty");
        return "";
    }
    
    std::string response = "";
    try {
        std::unique_lock<std::shared_mutex> lock(varlock);
        auto result = this->targetVars.emplace(name, value);
        if (!result.second) {
            result.first->second = value; // Update existing
        }
        response = result.first->second;
    } catch (...) {
        throw;
    }
    return response;
}

std::string TargetContext::GetTargetVar(std::string_view name) const {
    if (name.empty()) {
        return "";
    }
    
    std::string response = "";
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->targetVars.find(std::string(name));
        response = (it != this->targetVars.end()) ? it->second : std::string{};
    } catch (...) {
        throw;
    }
    return response;
}

bool TargetContext::HasTargetVar(std::string_view name) const {
    if (name.empty()) {
        return false;
    }
    
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->targetVars.find(std::string(name));
        return (it != this->targetVars.end()) ? true : false;
    } catch (...) {
        throw;
    }
    return false;
}


bool TargetContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    if (!ForgeObject::Serialize(a_intfc)) return false;
    
    using SH = SerializationHelper;
    
    // Write target FormID
    if (!SH::WriteData(a_intfc, tesTargetFormID)) return false;
    
    // Write threads
    std::size_t threadCount = threads.size();
    if (!SH::WriteData(a_intfc, threadCount)) return false;
    
    for (const auto threadHandle : threads) {
        if (!SH::WriteData(a_intfc, threadHandle)) return false;
    }
    
    // Write target variables
    if (!SH::WriteStringMap(a_intfc, targetVars)) return false;
    
    return true;
}

bool TargetContext::Deserialize(SKSE::SerializationInterface* a_intfc) {
    if (!ForgeObject::Serialize(a_intfc)) return false;
    
    using SH = SerializationHelper;
    
    // Read target FormID
    std::int32_t savedFormID;
    if (!SH::ReadData(a_intfc, savedFormID)) return false;
    
    // Resolve FormID (handles form ID changes between saves)
    if (!a_intfc->ResolveFormID(savedFormID, tesTargetFormID)) {
        // Form no longer exists - this context is invalid
        return false;
    }
    
    // Look up the actual form
    tesTarget = RE::TESForm::LookupByID(tesTargetFormID);
    if (!tesTarget) {
        return false;
    }
    
    // Read threads
    std::size_t threadCount;
    if (!SH::ReadData(a_intfc, threadCount)) return false;
    
    threads.clear();
    threads.reserve(threadCount);
    
    ForgeHandle threadHandle;
    for (std::size_t i = 0; i < threadCount; ++i) {
        if (!SH::ReadData(a_intfc, threadHandle)) return false;
        threads.push_back(threadHandle);
    }
    
    // Read target variables
    if (!SH::ReadStringMap(a_intfc, targetVars)) return false;
    
    return true;
}



#pragma endregion


}