#include "forge.h"

#include "../parsing.h"

namespace SLT {

bool FrameContext::ParseScript() {
    bool result = ParseResult::Success == Parser::ParseScript(this);
    if (!result) {
        LogError(std::format("Failed to parse script '{}': error code {}", scriptName, static_cast<int>(result)));
        return false;
    }
    return currentLine < scriptTokens.size();
}

ThreadContext* FrameContext::pthread() const {
    if (!thread && threadContextHandle && !threadFetchAttempted.exchange(true)) {
        thread = ThreadContextManager::GetFromHandle(threadContextHandle);
    }
    return thread;
}

CommandLine* FrameContext::GetCommandLine(std::size_t _at) const {
    if (_at < 0 || _at >= scriptTokens.size())
        return nullptr;

    auto cmdLineHandle = scriptTokens.at(_at);
    if (cmdLineHandle)
        return CommandLineManager::GetFromHandle(cmdLineHandle);
    
    return nullptr;
}

CommandLine* FrameContext::GetCommandLine() const {
    return GetCommandLine(currentLine);
}

std::size_t FrameContext::GetCurrentActualLineNumber() const {
    if (auto* cmdLine = GetCommandLine(currentLine)) {
        return cmdLine->lineNumber;
    }
    return static_cast<std::size_t>(-1);
}

std::string FrameContext::SetLocalVar(std::string_view name, std::string_view value) {
    if (name.empty()) {
        LogWarn("SetLocalVar: Variable name cannot be empty");
        return "";
    }
    
    std::string response = "";
    try {
        std::unique_lock<std::shared_mutex> lock(varlock);
        auto result = this->localVars.emplace(name, value);
        if (!result.second) {
            result.first->second = value; // Update existing
        }
        response = result.first->second;
    } catch (...) {
        throw;
    }
    return response;
}

std::string FrameContext::GetLocalVar(std::string_view name) const {
    if (name.empty()) {
        return "";
    }
    
    std::string response = "";
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->localVars.find(std::string(name));
        response = (it != this->localVars.end()) ? it->second : std::string{};
    } catch (...) {
        throw;
    }
    return response;
}

bool FrameContext::HasLocalVar(std::string_view name) const {
    if (name.empty()) {
        return false;
    }
    
    try {
        std::shared_lock<std::shared_mutex> lock(varlock);
        auto it = this->localVars.find(std::string(name));
        return (it != this->localVars.end()) ? true : false;
    } catch (...) {
        throw;
    }
    return false;
}

void FrameContext::LogInfo(const std::string& message) const {
    std::string prefixed = std::format("[{}:{}] - {}", scriptName, GetCurrentActualLineNumber(), message);
    logger::info("{}", prefixed);
}

void FrameContext::LogWarn(const std::string& message) const {
    std::string prefixed = std::format("[{}:{}] - {}", scriptName, GetCurrentActualLineNumber(), message);
    logger::warn("{}", prefixed);
}

void FrameContext::LogError(const std::string& message) const {
    std::string prefixed = std::format("[{}:{}] - {}", scriptName, GetCurrentActualLineNumber(), message);
    logger::error("{}", prefixed);
}

void FrameContext::LogDebug(const std::string& message) const {
    std::string prefixed = std::format("[{}:{}] - {}", scriptName, GetCurrentActualLineNumber(), message);
    logger::debug("{}", prefixed);
}

bool FrameContext::Serialize(SKSE::SerializationInterface* a_intfc) const {
    if (!ForgeObject::Serialize(a_intfc)) return false;
    
    using SH = SerializationHelper;
    
    // Write basic data
    if (!SH::WriteData(a_intfc, threadContextHandle)) return false;
    if (!SH::WriteString(a_intfc, scriptName)) return false;
    if (!SH::WriteData(a_intfc, currentLine)) return false;
    if (!SH::WriteData(a_intfc, commandType)) return false;
    if (!SH::WriteString(a_intfc, mostRecentResult)) return false;
    if (!SH::WriteData(a_intfc, lastKey)) return false;
    if (!SH::WriteData(a_intfc, iterActorFormID)) return false;
    
    // Write tokenized script
    std::size_t scriptLength = scriptTokens.size();
    if (!SH::WriteData(a_intfc, scriptLength)) return false;

    for (ForgeHandle cmdLineHandle : scriptTokens) {
        if (!SH::WriteData(a_intfc, cmdLineHandle)) return false;
    }
    
    // Write call arguments
    if (!SH::WriteStringVector(a_intfc, callArgs)) return false;
    
    // Write label maps
    if (!SH::WriteSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::WriteSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Write return stack
    if (!SH::WriteSizeTVector(a_intfc, returnStack)) return false;
    
    // Write local variables
    if (!SH::WriteStringMap(a_intfc, localVars)) return false;

    if (!SH::WriteData(a_intfc, isReady)) return false;
    if (!SH::WriteData(a_intfc, isReadied)) return false;
    
    return true;
}

bool FrameContext::Deserialize(SKSE::SerializationInterface* a_intfc) {
    if (!ForgeObject::Deserialize(a_intfc)) return false;
    
    using SH = SerializationHelper;
    
    // Read basic data
    if (!SH::ReadData(a_intfc, threadContextHandle)) return false;
    if (!SH::ReadString(a_intfc, scriptName)) return false;
    if (!SH::ReadData(a_intfc, currentLine)) return false;
    if (!SH::ReadData(a_intfc, commandType)) return false;
    if (!SH::ReadString(a_intfc, mostRecentResult)) return false;
    if (!SH::ReadData(a_intfc, lastKey)) return false;
    if (!SH::ReadData(a_intfc, iterActorFormID)) return false;
    
    // Resolve form ID for iterActor
    if (iterActorFormID != 0) {
        std::uint32_t resolvedFormID;
        if (a_intfc->ResolveFormID(iterActorFormID, resolvedFormID)) {
            iterActorFormID = resolvedFormID;
            iterActor = RE::TESForm::LookupByID<RE::Actor>(resolvedFormID);
        } else {
            // Form no longer exists
            iterActorFormID = 0;
            iterActor = nullptr;
        }
    } else {
        iterActor = nullptr;
    }
    
    std::size_t scriptLength;
    if (!SH::ReadData(a_intfc, scriptLength)) return false;

    scriptTokens.clear();
    scriptTokens.reserve(scriptLength);

    for (std::size_t i = 0; i < scriptLength; ++i) {
        ForgeHandle handle;
        if (!SH::ReadData(a_intfc, handle)) return false;
        scriptTokens.push_back(handle);
    }
    
    // Read call arguments
    if (!SH::ReadStringVector(a_intfc, callArgs)) return false;
    
    // Read label maps
    if (!SH::ReadSizeTMap(a_intfc, gotoLabelMap)) return false;
    if (!SH::ReadSizeTMap(a_intfc, gosubLabelMap)) return false;
    
    // Read return stack
    if (!SH::ReadSizeTVector(a_intfc, returnStack)) return false;
    
    // Read local variables
    if (!SH::ReadStringMap(a_intfc, localVars)) return false;

    if (!SH::ReadData(a_intfc, isReady)) return false;
    if (!SH::ReadData(a_intfc, isReadied)) return false;
    
    // Reset transient fields
    customResolveFormResult = nullptr;
    popAfterStepReturn = false;
    thread = nullptr; // Will be restored by ThreadContext
    threadFetchAttempted.store(false);
    
    return true;
}

} // namespace SLT