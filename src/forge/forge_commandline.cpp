#include "forge.h"

namespace SLT {

#pragma region CommandLine
bool CommandLine::Serialize(SKSE::SerializationInterface* a_intfc) const {
    if (!ForgeObject::Serialize(a_intfc)) return false;

    using SH = SerializationHelper;

    if (!SH::WriteData(a_intfc, lineNumber)) return false;

    std::size_t tokenCount = tokens.size();
    if (!SH::WriteData(a_intfc, tokenCount)) return false;

    for (auto& str : tokens) {
        if (!SH::WriteString(a_intfc, str)) return false;
    }

    return true;
}

bool CommandLine::Deserialize(SKSE::SerializationInterface* a_intfc) {
    if (!ForgeObject::Deserialize(a_intfc)) return false;
    
    using SH = SerializationHelper;

    if (!SH::ReadData(a_intfc, lineNumber)) return false;

    std::size_t tokenCount;
    if (!SH::ReadData(a_intfc, tokenCount)) return false;

    tokens.clear();
    tokens.reserve(tokenCount);
    for (std::size_t i = 0; i < tokenCount; ++i) {
        std::string str;
        if (!SH::ReadString(a_intfc, str)) return false;
        tokens.push_back(std::move(str));
    }

    return true;
}

#pragma endregion

}