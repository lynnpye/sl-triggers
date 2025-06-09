#pragma once

#include <variant>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>

namespace SLT {

using namespace RE;
using namespace RE::BSScript;

/**
 * A simple Optional type for Papyrus that mimics std::optional behavior
 * This implementation uses modern CommonLib APIs to avoid complex hooks
 */
class Optional {
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
    bool IsValid() const {
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
};

/**
 * Simple handle management for Optional objects
 * This avoids the complex hooks used in the original FDGE approach
 */
class OptionalManager {
private:
    static inline std::mutex _mutex;
    static inline std::unordered_map<std::uint32_t, std::unique_ptr<Optional>> _optionals;
    static inline std::uint32_t _nextHandle = 1;

public:
    static std::uint32_t CreateOptional(std::unique_ptr<Optional> optional) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::uint32_t handle = _nextHandle++;
        _optionals[handle] = std::move(optional);
        return handle;
    }

    static Optional* GetOptional(std::uint32_t handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _optionals.find(handle);
        return (it != _optionals.end()) ? it->second.get() : nullptr;
    }

    static void DestroyOptional(std::uint32_t handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        _optionals.erase(handle);
    }

    static void Clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _optionals.clear();
        _nextHandle = 1;
    }
};

void OnOptionalSave(SKSE::SerializationInterface* intfc);

void OnOptionalLoad(SKSE::SerializationInterface* intfc);

void OnOptionalRevert(SKSE::SerializationInterface* intfc);

} // namespace SLT