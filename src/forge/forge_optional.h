#pragma once

namespace SLT {

using namespace RE;
using namespace RE::BSScript;

/**
 * A simple Optional type for Papyrus that mimics std::optional behavior
 * This implementation uses modern CommonLib APIs to avoid complex hooks
 */
class Optional : public ForgeObject {
public:
    static constexpr const char* SCRIPT_NAME = "sl_triggersOptional";
    
    enum class Type : std::uint32_t {
        None = 0,
        Bool = 1,
        Int = 2,
        Float = 3,
        String = 4,
        Form = 5,
        ActiveEffect = 6,
        Alias = 7
    };

    using ValueType = std::variant<
        std::monostate,           // None
        bool,                     // Bool
        std::int32_t,            // Int
        float,                   // Float
        std::string,             // String
        RE::TESForm*,            // Form
        RE::ActiveEffect*,       // ActiveEffect
        RE::BGSBaseAlias*        // Alias
    >;

private:
    ValueType _value;

public:
    // Constructors
    Optional() = default;
    explicit Optional(bool value) : _value(value) {}
    explicit Optional(std::int32_t value) : _value(value) {}
    explicit Optional(float value) : _value(value) {}
    explicit Optional(const std::string& value) : _value(value) {}
    explicit Optional(RE::TESForm* value) : _value(value) {}
    explicit Optional(RE::ActiveEffect* value) : _value(value) {}
    explicit Optional(RE::BGSBaseAlias* value) : _value(value) {}

    // Core interface
    bool HasData() const {
        return !std::holds_alternative<std::monostate>(_value);
    }

    Type GetType() const {
        return static_cast<Type>(_value.index());
    }

    void Clear() {
        _value = std::monostate{};
    }

    // Type checking
    bool IsBool() const { return std::holds_alternative<bool>(_value); }
    bool IsInt() const { return std::holds_alternative<std::int32_t>(_value); }
    bool IsFloat() const { return std::holds_alternative<float>(_value); }
    bool IsString() const { return std::holds_alternative<std::string>(_value); }
    bool IsForm() const { return std::holds_alternative<RE::TESForm*>(_value); }
    bool IsActiveEffect() const { return std::holds_alternative<RE::ActiveEffect*>(_value); }
    bool IsAlias() const { return std::holds_alternative<RE::BGSBaseAlias*>(_value); }

    // Value getters
    bool GetBool() const {
        return std::holds_alternative<bool>(_value) ? std::get<bool>(_value) : false;
    }

    std::int32_t GetInt() const {
        return std::holds_alternative<std::int32_t>(_value) ? std::get<std::int32_t>(_value) : 0;
    }

    float GetFloat() const {
        return std::holds_alternative<float>(_value) ? std::get<float>(_value) : 0.0f;
    }

    std::string GetString() const {
        return std::holds_alternative<std::string>(_value) ? std::get<std::string>(_value) : "";
    }

    RE::TESForm* GetForm() const {
        return std::holds_alternative<RE::TESForm*>(_value) ? std::get<RE::TESForm*>(_value) : nullptr;
    }

    RE::ActiveEffect* GetActiveEffect() const {
        return std::holds_alternative<RE::ActiveEffect*>(_value) ? std::get<RE::ActiveEffect*>(_value) : nullptr;
    }

    RE::BGSBaseAlias* GetAlias() const {
        return std::holds_alternative<RE::BGSBaseAlias*>(_value) ? std::get<RE::BGSBaseAlias*>(_value) : nullptr;
    }

    // Value setters
    void SetBool(bool value) { _value = value; }
    void SetInt(std::int32_t value) { _value = value; }
    void SetFloat(float value) { _value = value; }
    void SetString(const std::string& value) { _value = value; }
    void SetForm(RE::TESForm* value) { _value = value; }
    void SetActiveEffect(RE::ActiveEffect* value) { _value = value; }
    void SetAlias(RE::BGSBaseAlias* value) { _value = value; }
    
    bool Serialize(SKSE::SerializationInterface* a_intfc) const;
    bool Deserialize(SKSE::SerializationInterface* a_intfc);

    struct Helpers {
        // ============================================================================
        // Type Constants - Global Functions
        // ============================================================================
        typedef std::int32_t OptionalType;
        static OptionalType TYPE_NONE(RE::StaticFunctionTag*) { return 0; }
        static OptionalType TYPE_BOOL(RE::StaticFunctionTag*) { return 1; }
        static OptionalType TYPE_INT(RE::StaticFunctionTag*) { return 2; }
        static OptionalType TYPE_FLOAT(RE::StaticFunctionTag*) { return 3; }
        static OptionalType TYPE_STRING(RE::StaticFunctionTag*) { return 4; }
        static OptionalType TYPE_FORM(RE::StaticFunctionTag*) { return 5; }
        static OptionalType TYPE_ACTIVEEFFECT(RE::StaticFunctionTag*) { return 6; }
        static OptionalType TYPE_ALIAS(RE::StaticFunctionTag*) { return 7; }

        // Factory functions that return handles instead of object pointers
        static ForgeHandle CreateEmpty(RE::StaticFunctionTag*) {
            auto optional = std::make_unique<Optional>();
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateBool(RE::StaticFunctionTag*, bool value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateInt(RE::StaticFunctionTag*, std::int32_t value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateFloat(RE::StaticFunctionTag*, float value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateString(RE::StaticFunctionTag*, std::string value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateForm(RE::StaticFunctionTag*, RE::TESForm* value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateActiveEffect(RE::StaticFunctionTag*, RE::ActiveEffect* value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        static ForgeHandle CreateAlias(RE::StaticFunctionTag*, RE::BGSBaseAlias* value) {
            auto optional = std::make_unique<Optional>(value);
            return ForgeManagerTemplate<Optional>::Create(std::move(optional));
        }

        // Query functions
        static bool HasData(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->HasData() : false;
        }

        static OptionalType GetType(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? static_cast<OptionalType>(optional->GetType()) : 0;
        }

        static bool IsBool(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsBool() : false;
        }

        static bool IsInt(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsInt() : false;
        }

        static bool IsFloat(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsFloat() : false;
        }

        static bool IsString(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsString() : false;
        }

        static bool IsForm(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsForm() : false;
        }

        static bool IsActiveEffect(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsActiveEffect() : false;
        }

        static bool IsAlias(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->IsAlias() : false;
        }

        // Value getters
        static bool GetBool(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetBool() : false;
        }

        static std::int32_t GetInt(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetInt() : 0;
        }

        static float GetFloat(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetFloat() : 0.0f;
        }

        static std::string GetString(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetString() : "";
        }

        static RE::TESForm* GetForm(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetForm() : nullptr;
        }

        static RE::ActiveEffect* GetActiveEffect(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetActiveEffect() : nullptr;
        }

        static RE::BGSBaseAlias* GetAlias(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            return optional ? optional->GetAlias() : nullptr;
        }

        // Value setters
        static void SetBool(RE::StaticFunctionTag*, ForgeHandle handle, bool value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetBool(value);
            }
        }

        static void SetInt(RE::StaticFunctionTag*, ForgeHandle handle, std::int32_t value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetInt(value);
            }
        }

        static void SetFloat(RE::StaticFunctionTag*, ForgeHandle handle, float value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetFloat(value);
            }
        }

        static void SetString(RE::StaticFunctionTag*, ForgeHandle handle, std::string value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetString(value);
            }
        }

        static void SetForm(RE::StaticFunctionTag*, ForgeHandle handle, RE::TESForm* value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetForm(value);
            }
        }

        static void SetActiveEffect(RE::StaticFunctionTag*, ForgeHandle handle, RE::ActiveEffect* value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetActiveEffect(value);
            }
        }

        static void SetAlias(RE::StaticFunctionTag*, ForgeHandle handle, RE::BGSBaseAlias* value) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->SetAlias(value);
            }
        }

        // Utility functions
        static void Clear(RE::StaticFunctionTag*, ForgeHandle handle) {
            auto* optional = ForgeManagerTemplate<Optional>::GetFromHandle(handle);
            if (optional) {
                optional->Clear();
            }
        }

        static void Destroy(RE::StaticFunctionTag*, ForgeHandle handle) {
            ForgeManagerTemplate<Optional>::DestroyFromHandle(handle);
        }
    };

    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        if (!vm) {
            logger::error("Register failed to secure vm");
            return false;
        }

        // Register type constants
        vm->RegisterFunction("TYPE_NONE", SCRIPT_NAME, Helpers::TYPE_NONE);
        vm->RegisterFunction("TYPE_BOOL", SCRIPT_NAME, Helpers::TYPE_BOOL);
        vm->RegisterFunction("TYPE_INT", SCRIPT_NAME, Helpers::TYPE_INT);
        vm->RegisterFunction("TYPE_FLOAT", SCRIPT_NAME, Helpers::TYPE_FLOAT);
        vm->RegisterFunction("TYPE_STRING", SCRIPT_NAME, Helpers::TYPE_STRING);
        vm->RegisterFunction("TYPE_FORM", SCRIPT_NAME, Helpers::TYPE_FORM);
        vm->RegisterFunction("TYPE_ACTIVEEFFECT", SCRIPT_NAME, Helpers::TYPE_ACTIVEEFFECT);
        vm->RegisterFunction("TYPE_ALIAS", SCRIPT_NAME, Helpers::TYPE_ALIAS);

        // Register factory functions
        vm->RegisterFunction("CreateEmpty", SCRIPT_NAME, Helpers::CreateEmpty);
        vm->RegisterFunction("CreateBool", SCRIPT_NAME, Helpers::CreateBool);
        vm->RegisterFunction("CreateInt", SCRIPT_NAME, Helpers::CreateInt);
        vm->RegisterFunction("CreateFloat", SCRIPT_NAME, Helpers::CreateFloat);
        vm->RegisterFunction("CreateString", SCRIPT_NAME, Helpers::CreateString);
        vm->RegisterFunction("CreateForm", SCRIPT_NAME, Helpers::CreateForm);
        vm->RegisterFunction("CreateActiveEffect", SCRIPT_NAME, Helpers::CreateActiveEffect);
        vm->RegisterFunction("CreateAlias", SCRIPT_NAME, Helpers::CreateAlias);

        // Register query functions
        vm->RegisterFunction("HasData", SCRIPT_NAME, Helpers::HasData);
        vm->RegisterFunction("GetType", SCRIPT_NAME, Helpers::GetType);
        vm->RegisterFunction("IsBool", SCRIPT_NAME, Helpers::IsBool);
        vm->RegisterFunction("IsInt", SCRIPT_NAME, Helpers::IsInt);
        vm->RegisterFunction("IsFloat", SCRIPT_NAME, Helpers::IsFloat);
        vm->RegisterFunction("IsString", SCRIPT_NAME, Helpers::IsString);
        vm->RegisterFunction("IsForm", SCRIPT_NAME, Helpers::IsForm);
        vm->RegisterFunction("IsActiveEffect", SCRIPT_NAME, Helpers::IsActiveEffect);
        vm->RegisterFunction("IsAlias", SCRIPT_NAME, Helpers::IsAlias);

        // Register value getters
        vm->RegisterFunction("GetBool", SCRIPT_NAME, Helpers::GetBool);
        vm->RegisterFunction("GetInt", SCRIPT_NAME, Helpers::GetInt);
        vm->RegisterFunction("GetFloat", SCRIPT_NAME, Helpers::GetFloat);
        vm->RegisterFunction("GetString", SCRIPT_NAME, Helpers::GetString);
        vm->RegisterFunction("GetForm", SCRIPT_NAME, Helpers::GetForm);
        vm->RegisterFunction("GetActiveEffect", SCRIPT_NAME, Helpers::GetActiveEffect);
        vm->RegisterFunction("GetAlias", SCRIPT_NAME, Helpers::GetAlias);

        // Register value setters
        vm->RegisterFunction("SetBool", SCRIPT_NAME, Helpers::SetBool);
        vm->RegisterFunction("SetInt", SCRIPT_NAME, Helpers::SetInt);
        vm->RegisterFunction("SetFloat", SCRIPT_NAME, Helpers::SetFloat);
        vm->RegisterFunction("SetString", SCRIPT_NAME, Helpers::SetString);
        vm->RegisterFunction("SetForm", SCRIPT_NAME, Helpers::SetForm);
        vm->RegisterFunction("SetActiveEffect", SCRIPT_NAME, Helpers::SetActiveEffect);
        vm->RegisterFunction("SetAlias", SCRIPT_NAME, Helpers::SetAlias);

        // Register utility functions
        vm->RegisterFunction("Clear", SCRIPT_NAME, Helpers::Clear);
        vm->RegisterFunction("Destroy", SCRIPT_NAME, Helpers::Destroy);
        
        return true;
    }
};

using OptionalManager = ForgeManagerTemplate<Optional>;

} // namespace SLT