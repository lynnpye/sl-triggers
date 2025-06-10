namespace SLT {

#pragma region ThreadContext

TargetContext* ThreadContext::ptarget() const {
    if (!_target && targetContextHandle && !targetFetchAttempted) {
        targetFetchAttempted = true;
        _target = nullptr; //TargetContextManager::GetFromHandle(targetContextHandle);
    }
    return _target;
}

std::string ThreadContext::SetThreadVar(std::string_view name, std::string_view value) {
    if (name.empty()) {
        logger::warn("SetThreadVar: Variable name cannot be empty");
        return "";
    }
    
    std::string response = "";
    try {
        std::unique_lock<std::shared_mutex> lock(varlock);
        auto result = this->threadVars.emplace(name, value);
        if (!result.second) {
            result.first->second = value; // Update existing
        }
        response = result.first->second;
    } catch (...) {
        throw;
    }
    return response;
}

std::string ThreadContext::GetThreadVar(std::string_view name) const {
    if (name.empty()) {
        return "";
    }
    
    std::string response = "";
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->threadVars.find(std::string(name));
        response = (it != this->threadVars.end()) ? it->second : std::string{};
    } catch (...) {
        throw;
    }
    return response;
}

bool ThreadContext::HasThreadVar(std::string_view name) const {
    if (name.empty()) {
        return false;
    }
    
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->threadVars.find(std::string(name));
        return (it != this->threadVars.end()) ? true : false;
    } catch (...) {
        throw;
    }
    return false;
}

FrameContext* ThreadContext::GetCurrentFrame() const {
    FrameContext* result = nullptr;

    if (callStack.size() > 0) {
        result = FrameContextManager::GetFromHandle(callStack.back());
    }

    return result;
}

FrameContext* ThreadContext::PushFrameContext(std::string_view initialScriptName) {
    if (initialScriptName.empty())
        return nullptr;
    
    try {
        callStack.push_back(FrameContextManager::Create(std::move(std::make_unique<FrameContext>(GetHandle(), initialScriptName))));
        return GetCurrentFrame();
    } catch (...) {
        return nullptr;
    }
}

bool ThreadContext::PopFrameContext() {
    if (callStack.size() < 1)
        return false;

    try {
        if (callStack.size() > 0)
            callStack.pop_back();

        if (callStack.size() < 1)
            return false;
        
        auto* previousFrame = GetCurrentFrame();
        while (previousFrame && (previousFrame->currentLine >= previousFrame->scriptTokens.size())) {
            callStack.pop_back();
            if (callStack.size() < 1)
                return false;
            previousFrame = GetCurrentFrame();
        }
        
        return (previousFrame && (previousFrame->currentLine < previousFrame->scriptTokens.size()));
    } catch (...) {
        return false;
    }
}

bool ThreadContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    if (!ForgeObject::Serialize(a_intfc)) return false;
    
    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, wasClaimed)) return false;
    if (!SH::WriteString(a_intfc, initialScriptName)) return false;
    
    // Write call stack
    std::size_t stackSize = callStack.size();
    if (!SH::WriteData(a_intfc, stackSize)) return false;
    
    for (const auto frameHandle : callStack) {
        if (!SH::WriteData(a_intfc, frameHandle)) return false;
    }
    
    // Write thread variables
    if (!SH::WriteStringMap(a_intfc, threadVars)) return false;
    
    return true;
}

bool ThreadContext::Deserialize(SKSE::SerializationInterface* a_intfc) {
    if (!ForgeObject::Deserialize(a_intfc)) return false;
    
    using SH = SerializationHelper;

    isClaimed = false;
    if (!SH::ReadData(a_intfc, wasClaimed)) return false;
    if (!SH::ReadString(a_intfc, initialScriptName)) return false;
    
    // Read call stack
    std::size_t stackSize;
    if (!SH::ReadData(a_intfc, stackSize)) return false;
    
    callStack.clear();
    callStack.reserve(stackSize);
    
    ForgeHandle frameHandle;
    for (std::size_t i = 0; i < stackSize; ++i) {
        if (!SH::ReadData(a_intfc, frameHandle)) return false;
        callStack.push_back(frameHandle);
    }
    
    _target = nullptr;
    targetFetchAttempted = false;
    
    // Read thread variables
    if (!SH::ReadStringMap(a_intfc, threadVars)) return false;
    
    return true;
}


#pragma endregion


}