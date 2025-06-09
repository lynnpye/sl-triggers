#include "forge.h"

namespace SLT {

// ============================================================================
// Type Constants - Global Functions
// ============================================================================

std::uint32_t TYPE_NONE(RE::StaticFunctionTag*) { return 0; }
std::uint32_t TYPE_BOOL(RE::StaticFunctionTag*) { return 1; }
std::uint32_t TYPE_INT(RE::StaticFunctionTag*) { return 2; }
std::uint32_t TYPE_FLOAT(RE::StaticFunctionTag*) { return 3; }
std::uint32_t TYPE_STRING(RE::StaticFunctionTag*) { return 4; }
std::uint32_t TYPE_FORM(RE::StaticFunctionTag*) { return 5; }
std::uint32_t TYPE_ACTIVEEFFECT(RE::StaticFunctionTag*) { return 6; }
std::uint32_t TYPE_ALIAS(RE::StaticFunctionTag*) { return 7; }

// ============================================================================
// Papyrus Function Implementations using Handle-based approach
// ============================================================================

// Factory functions that return handles instead of object pointers
std::uint32_t CreateEmpty(RE::StaticFunctionTag*) {
    auto optional = std::make_unique<Optional>();
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateBool(RE::StaticFunctionTag*, bool value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateInt(RE::StaticFunctionTag*, std::int32_t value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateFloat(RE::StaticFunctionTag*, float value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateString(RE::StaticFunctionTag*, std::string value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateForm(RE::StaticFunctionTag*, RE::TESForm* value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateActiveEffect(RE::StaticFunctionTag*, RE::ActiveEffect* value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

std::uint32_t CreateAlias(RE::StaticFunctionTag*, RE::BGSBaseAlias* value) {
    auto optional = std::make_unique<Optional>(value);
    return OptionalManager::CreateOptional(std::move(optional));
}

// Query functions
bool HasData(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->HasData() : false;
}

std::uint32_t GetType(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? static_cast<std::uint32_t>(optional->GetType()) : 0;
}

bool IsBool(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsBool() : false;
}

bool IsInt(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsInt() : false;
}

bool IsFloat(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsFloat() : false;
}

bool IsString(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsString() : false;
}

bool IsForm(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsForm() : false;
}

bool IsActiveEffect(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsActiveEffect() : false;
}

bool IsAlias(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->IsAlias() : false;
}

// Value getters
bool GetBool(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetBool() : false;
}

std::int32_t GetInt(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetInt() : 0;
}

float GetFloat(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetFloat() : 0.0f;
}

std::string GetString(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetString() : "";
}

RE::TESForm* GetForm(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetForm() : nullptr;
}

RE::ActiveEffect* GetActiveEffect(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetActiveEffect() : nullptr;
}

RE::BGSBaseAlias* GetAlias(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    return optional ? optional->GetAlias() : nullptr;
}

// Value setters
void SetBool(RE::StaticFunctionTag*, std::uint32_t handle, bool value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetBool(value);
    }
}

void SetInt(RE::StaticFunctionTag*, std::uint32_t handle, std::int32_t value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetInt(value);
    }
}

void SetFloat(RE::StaticFunctionTag*, std::uint32_t handle, float value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetFloat(value);
    }
}

void SetString(RE::StaticFunctionTag*, std::uint32_t handle, std::string value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetString(value);
    }
}

void SetForm(RE::StaticFunctionTag*, std::uint32_t handle, RE::TESForm* value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetForm(value);
    }
}

void SetActiveEffect(RE::StaticFunctionTag*, std::uint32_t handle, RE::ActiveEffect* value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetActiveEffect(value);
    }
}

void SetAlias(RE::StaticFunctionTag*, std::uint32_t handle, RE::BGSBaseAlias* value) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->SetAlias(value);
    }
}

// Utility functions
void Clear(RE::StaticFunctionTag*, std::uint32_t handle) {
    auto* optional = OptionalManager::GetOptional(handle);
    if (optional) {
        optional->Clear();
    }
}

void Destroy(RE::StaticFunctionTag*, std::uint32_t handle) {
    OptionalManager::DestroyOptional(handle);
}

// ============================================================================
// Papyrus Registration
// ============================================================================

bool RegisterOptional(RE::BSScript::IVirtualMachine* vm) {
    if (!vm) {
        logger::error("RegisterOptional failed to secure vm");
        return false;
    }

    // Register type constants
    vm->RegisterFunction("TYPE_NONE", Optional::SCRIPT_NAME, TYPE_NONE);
    vm->RegisterFunction("TYPE_BOOL", Optional::SCRIPT_NAME, TYPE_BOOL);
    vm->RegisterFunction("TYPE_INT", Optional::SCRIPT_NAME, TYPE_INT);
    vm->RegisterFunction("TYPE_FLOAT", Optional::SCRIPT_NAME, TYPE_FLOAT);
    vm->RegisterFunction("TYPE_STRING", Optional::SCRIPT_NAME, TYPE_STRING);
    vm->RegisterFunction("TYPE_FORM", Optional::SCRIPT_NAME, TYPE_FORM);
    vm->RegisterFunction("TYPE_ACTIVEEFFECT", Optional::SCRIPT_NAME, TYPE_ACTIVEEFFECT);
    vm->RegisterFunction("TYPE_ALIAS", Optional::SCRIPT_NAME, TYPE_ALIAS);

    // Register factory functions
    vm->RegisterFunction("CreateEmpty", Optional::SCRIPT_NAME, CreateEmpty);
    vm->RegisterFunction("CreateBool", Optional::SCRIPT_NAME, CreateBool);
    vm->RegisterFunction("CreateInt", Optional::SCRIPT_NAME, CreateInt);
    vm->RegisterFunction("CreateFloat", Optional::SCRIPT_NAME, CreateFloat);
    vm->RegisterFunction("CreateString", Optional::SCRIPT_NAME, CreateString);
    vm->RegisterFunction("CreateForm", Optional::SCRIPT_NAME, CreateForm);
    vm->RegisterFunction("CreateActiveEffect", Optional::SCRIPT_NAME, CreateActiveEffect);
    vm->RegisterFunction("CreateAlias", Optional::SCRIPT_NAME, CreateAlias);

    // Register query functions
    vm->RegisterFunction("HasData", Optional::SCRIPT_NAME, HasData);
    vm->RegisterFunction("GetType", Optional::SCRIPT_NAME, GetType);
    vm->RegisterFunction("IsBool", Optional::SCRIPT_NAME, IsBool);
    vm->RegisterFunction("IsInt", Optional::SCRIPT_NAME, IsInt);
    vm->RegisterFunction("IsFloat", Optional::SCRIPT_NAME, IsFloat);
    vm->RegisterFunction("IsString", Optional::SCRIPT_NAME, IsString);
    vm->RegisterFunction("IsForm", Optional::SCRIPT_NAME, IsForm);
    vm->RegisterFunction("IsActiveEffect", Optional::SCRIPT_NAME, IsActiveEffect);
    vm->RegisterFunction("IsAlias", Optional::SCRIPT_NAME, IsAlias);

    // Register value getters
    vm->RegisterFunction("GetBool", Optional::SCRIPT_NAME, GetBool);
    vm->RegisterFunction("GetInt", Optional::SCRIPT_NAME, GetInt);
    vm->RegisterFunction("GetFloat", Optional::SCRIPT_NAME, GetFloat);
    vm->RegisterFunction("GetString", Optional::SCRIPT_NAME, GetString);
    vm->RegisterFunction("GetForm", Optional::SCRIPT_NAME, GetForm);
    vm->RegisterFunction("GetActiveEffect", Optional::SCRIPT_NAME, GetActiveEffect);
    vm->RegisterFunction("GetAlias", Optional::SCRIPT_NAME, GetAlias);

    // Register value setters
    vm->RegisterFunction("SetBool", Optional::SCRIPT_NAME, SetBool);
    vm->RegisterFunction("SetInt", Optional::SCRIPT_NAME, SetInt);
    vm->RegisterFunction("SetFloat", Optional::SCRIPT_NAME, SetFloat);
    vm->RegisterFunction("SetString", Optional::SCRIPT_NAME, SetString);
    vm->RegisterFunction("SetForm", Optional::SCRIPT_NAME, SetForm);
    vm->RegisterFunction("SetActiveEffect", Optional::SCRIPT_NAME, SetActiveEffect);
    vm->RegisterFunction("SetAlias", Optional::SCRIPT_NAME, SetAlias);

    // Register utility functions
    vm->RegisterFunction("Clear", Optional::SCRIPT_NAME, Clear);
    vm->RegisterFunction("Destroy", Optional::SCRIPT_NAME, Destroy);

    return true;
}

// ============================================================================
// SKSE Event Handlers
// ============================================================================

void OnOptionalSave(SKSE::SerializationInterface* intfc) {
    // For simplicity, we don't persist optionals across saves
    // You could implement this if needed
}

void OnOptionalLoad(SKSE::SerializationInterface* intfc) {
    // Clear all optionals on load
    OptionalManager::Clear();
}

void OnOptionalRevert(SKSE::SerializationInterface* intfc) {
    // Clear all optionals on revert
    OptionalManager::Clear();
}


} // namespace SLT