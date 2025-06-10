#pragma once


namespace SLT {

/**
 * CommandLine - container for an execution-relevant line of SLTScript; may contain additional contextual information useful for output and debugging for the end-user
 * std::size_t lineNumber : line number in the original user SLTScript (i.e. inclusive of empty lines, a command may be on line 5 but be the first command, e.g. lineNumber would be 5)
 * std::vector<std::string> tokens : raw, unresolved string tokens (parsed but not necessarily lexed)
 */
class CommandLine : public ForgeObject {
public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersCommandLine";

    CommandLine() = default;

    explicit CommandLine(std::size_t _lineNumber, std::vector<std::string> _tokens)
        : lineNumber(_lineNumber), tokens(_tokens) {}

    std::size_t lineNumber;
    std::vector<std::string> tokens;

    std::size_t GetLineNumber() const {
        return lineNumber;
    }

    void SetLineNumber(std::size_t _lineNumber) {
        lineNumber = _lineNumber;
    }

    std::vector<std::string> GetTokens() const {
        return tokens;
    }

    void SetTokens(std::vector<std::string> _tokens) {
        tokens = _tokens;
    }
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    struct Helpers {
        static ForgeHandle CreateEmpty(RE::StaticFunctionTag*) {
            auto obj = std::make_unique<CommandLine>();
            return ForgeManagerTemplate<CommandLine>::Create(std::move(obj));
        }

        static ForgeHandle CreateNew(RE::StaticFunctionTag*, std::int32_t lineNumber, std::vector<std::string> tokens) {
            auto obj = std::make_unique<CommandLine>(lineNumber, tokens);
            return ForgeManagerTemplate<CommandLine>::Create(std::move(obj));
        }

        static std::int32_t GetLineNumber(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<CommandLine>::GetFromHandle(handle);
            return obj ? obj->GetLineNumber() : 0;
        }

        static void SetLineNumber(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t lineNumber) {
            auto* obj = ForgeManagerTemplate<CommandLine>::GetFromHandle(handle);
            if (obj) obj->SetLineNumber(lineNumber);
        }

        static std::vector<std::string> GetTokens(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* obj = ForgeManagerTemplate<CommandLine>::GetFromHandle(handle);
            return obj ? obj->GetTokens() : std::vector<std::string>{};
        }

        static void SetTokens(RE::StaticFunctionTag*, ForgeHandle handle, std::vector<std::string> tokens) {
            auto* obj = ForgeManagerTemplate<CommandLine>::GetFromHandle(handle);
            if (obj) obj->SetTokens(tokens);
        }

        static void Destroy(RE::StaticFunctionTag*, ForgeHandle handle) {
            ForgeManagerTemplate<CommandLine>::DestroyFromHandle(handle);
        }
    };

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        vm->RegisterFunction("CreateEmpty", SCRIPT_NAME, Helpers::CreateEmpty);
        vm->RegisterFunction("CreateNew", SCRIPT_NAME, Helpers::CreateNew);
        vm->RegisterFunction("GetLineNumber", SCRIPT_NAME, Helpers::GetLineNumber);
        vm->RegisterFunction("SetLineNumber", SCRIPT_NAME, Helpers::SetLineNumber);
        vm->RegisterFunction("GetTokens", SCRIPT_NAME, Helpers::GetTokens);
        vm->RegisterFunction("SetTokens", SCRIPT_NAME, Helpers::SetTokens);
        vm->RegisterFunction("Destroy", SCRIPT_NAME, Helpers::Destroy);

        return true;
    }
};

using CommandLineManager = ForgeManagerTemplate<CommandLine>;

}