#pragma once

#include "util.h"

namespace SLT {

#pragma region Random type declarations
enum ScriptType {
    INVALID = 0,

    INI,
    JSON,


    END
};

typedef std::uint32_t SKSEMessageType;
typedef std::int32_t SLTSessionId;
typedef std::int32_t ForgeHandle;

extern const std::string_view BASE_QUEST;
extern const std::string_view BASE_AME;
#pragma endregion

#pragma region Random function declarations
fs::path GetPluginPath();
fs::path GetScriptfilePath(std::string_view scriptfilename);
SLTSessionId GenerateNewSessionId(bool force = false);
SLTSessionId GetSessionId();
#pragma endregion

#pragma region LoggerGuard
// RAII Logger Guard - logs on construction and destruction
class LoggerGuard {
private:
    std::string function_name;
    std::string exit_message;
    
public:
    LoggerGuard(const std::string& func_name, 
                const std::string& entry_msg = "Entering", 
                const std::string& exit_msg = "Exiting") 
        : function_name(func_name), exit_message(exit_msg) {
        logger::debug("\n\t\t>>>>>{} {}\n", function_name, entry_msg);
    }
    
    ~LoggerGuard() {
        logger::debug("\n\t\t<<<<<{} {}\n", function_name, exit_message);
    }
    
    // Prevent copying to avoid double logging
    LoggerGuard(const LoggerGuard&) = delete;
    LoggerGuard& operator=(const LoggerGuard&) = delete;
};

// Macro for convenience (optional)
#define LOG_FUNCTION_SCOPE(func_name) LoggerGuard _log_guard(func_name)
#define LOG_FUNCTION_SCOPE_CUSTOM(func_name, entry, exit) LoggerGuard _log_guard(func_name, entry, exit)
#pragma endregion

#pragma region SmartComparator
class SmartComparator {
private:
    template<typename T>
    static bool IsTruthy(const T& value) {
        if constexpr (std::is_same_v<T, std::monostate>) {
            return false;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return value != 0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            return !value.empty() && value != "0" && !Util::String::isFalse(value);
        } else {
            return true; // Unknown types default to truthy
        }
    }
    
    template<typename T, typename U>
    static bool CompareValues(const T& lhs, const U& rhs) {
        bool lhsTruthy = IsTruthy(lhs);
        bool rhsTruthy = IsTruthy(rhs);
        
        if (lhsTruthy == rhsTruthy && !lhsTruthy) {
            return true;
        }
        
        if (lhsTruthy != rhsTruthy) {
            return false;
        }

        if constexpr (std::is_arithmetic_v<T> && std::is_arithmetic_v<U>) {
            if constexpr (std::is_same_v<T, bool> || std::is_same_v<U, bool>) {
                return lhsTruthy && rhsTruthy;
            } else {
                return CompareNumeric(lhs, rhs);
            }
        } else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<U, std::string>) {
            return CompareWithString(lhs, rhs);
        } else {
            return lhsTruthy && rhsTruthy;
        }
    }
    
    static bool CompareValues(const std::monostate&, const std::monostate&) { return true; }
    static bool CompareValues(bool lhs, bool rhs) { return lhs == rhs; }
    static bool CompareValues(std::int32_t lhs, std::int32_t rhs) { return lhs == rhs; }
    static bool CompareValues(float lhs, float rhs) { 
        return std::fabs(lhs - rhs) < FLT_EPSILON; 
    }
    static bool CompareValues(const std::string& lhs, const std::string& rhs) { 
        // Try to parse both as numbers first
        float lhsFloat, rhsFloat;
        bool lhsIsFloat = false, rhsIsFloat = false;
        
        // Try parsing as floats
        auto [ptr1, ec1] = std::from_chars(lhs.data(), lhs.data() + lhs.size(), lhsFloat);
        if (ec1 == std::errc{} && ptr1 == lhs.data() + lhs.size()) {
            lhsIsFloat = true;
        }
        
        auto [ptr2, ec2] = std::from_chars(rhs.data(), rhs.data() + rhs.size(), rhsFloat);
        if (ec2 == std::errc{} && ptr2 == rhs.data() + rhs.size()) {
            rhsIsFloat = true;
        }
        
        // If both parsed as numbers, compare numerically
        if (lhsIsFloat && rhsIsFloat) {
            return std::fabs(lhsFloat - rhsFloat) < FLT_EPSILON;
        }
        
        // If only one is a number, try integer parsing for the other
        if (lhsIsFloat || rhsIsFloat) {
            std::int32_t lhsInt, rhsInt;
            bool lhsIsInt = false, rhsIsInt = false;
            
            if (!lhsIsFloat) {
                auto [ptr, ec] = std::from_chars(lhs.data(), lhs.data() + lhs.size(), lhsInt);
                if (ec == std::errc{} && ptr == lhs.data() + lhs.size()) {
                    lhsIsInt = true;
                    lhsFloat = static_cast<float>(lhsInt);
                    lhsIsFloat = true;
                }
            }
            
            if (!rhsIsFloat) {
                auto [ptr, ec] = std::from_chars(rhs.data(), rhs.data() + rhs.size(), rhsInt);
                if (ec == std::errc{} && ptr == rhs.data() + rhs.size()) {
                    rhsIsInt = true;
                    rhsFloat = static_cast<float>(rhsInt);
                    rhsIsFloat = true;
                }
            }
            
            // If both are now numeric, compare
            if (lhsIsFloat && rhsIsFloat) {
                return std::fabs(lhsFloat - rhsFloat) < FLT_EPSILON;
            }
        }
        
        // Fall back to string comparison
        return str::iEquals(lhs, rhs); 
    }
    
    template<typename T, typename U>
    static bool CompareNumeric(T lhs, U rhs) {
        if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
            return std::fabs(static_cast<float>(lhs) - static_cast<float>(rhs)) < FLT_EPSILON;
        } else {
            return lhs == rhs;
        }
    }
    
    template<typename T>
    static bool CompareWithString(const T& value, const std::string& str) {
        return CompareWithString(str, value);
    }
    
    template<typename T>
    static bool CompareWithString(const std::string& str, const T& value) {
        if constexpr (std::is_same_v<T, bool>) {
            auto strLower = str;
            std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
            
            // Handle falsy values
            return Util::String::toBool(str) == value;
        } else if constexpr (std::is_arithmetic_v<T>) {
            if constexpr (std::is_integral_v<T>) {
                std::int32_t parsedInt;
                auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), parsedInt);
                if (ec == std::errc{} && ptr == str.data() + str.size()) {
                    return parsedInt == static_cast<std::int32_t>(value);
                }
            }
            if constexpr (std::is_floating_point_v<T>) {
                float parsedFloat;
                auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), parsedFloat);
                if (ec == std::errc{} && ptr == str.data() + str.size()) {
                    return std::fabs(parsedFloat - static_cast<float>(value)) < FLT_EPSILON;
                }
            }
            return false;
        } else if constexpr (std::is_same_v<T, RE::TESForm*>) {
            if (!value) return !IsTruthy(str);
            
            // Try EditorID comparison if available
            auto editorId = value->GetFormEditorID();
            if (editorId && str == editorId) return true;
            
            // Try FormID comparison
            RE::FormID n_formId = value->GetFormID();
            auto str_formId = Util::String::StringToIntWithImplicitHexConversion(str);
            if (str_formId.has_value()) {
                return n_formId == str_formId.value();
            }
            
            return false;
        } else {
            return false;
        }
    }
};
#pragma endregion

#pragma region ScriptPoolManager

class ScriptPoolManager {
public:
    static ScriptPoolManager& GetSingleton() {
        static ScriptPoolManager singleton;
        return singleton;
    }

    void InitializePool() {
        spellPool.clear();
        mgefPool.clear();

        int highWaterMark = 0;
        std::string lastSpellId;
        
        for (int i = 1; i <= 99; ++i) {
            std::string spellId = "slt_cmds" + std::to_string(i).insert(0, 2 - std::to_string(i).length(), '0');
            if (auto spell = RE::TESForm::LookupByEditorID<RE::SpellItem>(spellId)) {
                spellPool.push_back(spell);
                highWaterMark = i;
                lastSpellId = spellId;
            } else {
                break;
            }
        }
        
        for (int i = 1; i <= highWaterMark; ++i) {
            std::string mgefId = "slt_cmd" + std::to_string(i).insert(0, 2 - std::to_string(i).length(), '0');
            if (auto mgef = RE::TESForm::LookupByEditorID<RE::EffectSetting>(mgefId)) {
                mgefPool.push_back(mgef);
            } else {
                highWaterMark = i;
                logger::warn("Failed to find magic effect: [{}]({})\n\
                    Fewer MGEF records than SPEL records. Using reduced high water mark {} and reducing SPEL pool to match", i, mgefId, highWaterMark);
                spellPool.resize(highWaterMark);
                break;
            }
        }
        
        logger::info("Script pool initialized: {} spells, {} magic effects", 
                    spellPool.size(), mgefPool.size());
    }

    RE::EffectSetting* FindAvailableMGEF(RE::Actor* target) {
        if (!target) return nullptr;
        
        auto magicTarget = target->AsMagicTarget();
        if (!magicTarget) return nullptr;
        
        for (auto mgef : mgefPool) {
            if (!magicTarget->HasMagicEffect(mgef)) {
                return mgef;
            }
        }
        
        logger::warn("No available magic effects in pool for target {}", 
                    target->GetDisplayFullName());
        return nullptr;
    }
    
    RE::SpellItem* FindSpellForMGEF(RE::EffectSetting* mgef) {
        if (!mgef) return nullptr;
        
        auto it = std::find(mgefPool.begin(), mgefPool.end(), mgef);
        if (it != mgefPool.end()) {
            size_t index = std::distance(mgefPool.begin(), it);
            if (index < spellPool.size()) {
                return spellPool[index];
            }
        }
        
        return nullptr;
    }
    
    bool ApplyScript(RE::Actor* target, std::string_view scriptName);

private:
    std::vector<RE::SpellItem*> spellPool;
    std::vector<RE::EffectSetting*> mgefPool;
    
    ScriptPoolManager() = default;
    ScriptPoolManager(const ScriptPoolManager&) = delete;
    ScriptPoolManager& operator=(const ScriptPoolManager&) = delete;
};
#pragma endregion
}