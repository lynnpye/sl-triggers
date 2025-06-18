#include "engine.h"
#include "sl_triggers.h"

#pragma push(warning)
#pragma warning(disable:4100)


namespace SLT {

// SLTNativeFunctions implementation remains the same below

#pragma region SLTNativeFunctions definition

// Non-latent Functions
bool SLTNativeFunctions::DeleteTrigger(PAPYRUS_NATIVE_DECL, std::string_view extKeyStr, std::string_view trigKeyStr) {
    if (!SystemUtil::File::IsValidPathComponent(extKeyStr) || !SystemUtil::File::IsValidPathComponent(trigKeyStr)) {
        logger::error("Invalid characters in extensionKey ({}) or triggerKey ({})", extKeyStr, trigKeyStr);
        return false;
    }

    if (extKeyStr.empty() || trigKeyStr.empty()) {
        logger::error("extensionKey and triggerKey may not be empty extensionKey[{}]  triggerKey[{}]", extKeyStr, trigKeyStr);
        return false;
    }

    // Ensure triggerKey ends with ".json"
    if (trigKeyStr.length() < 5 || trigKeyStr.substr(trigKeyStr.length() - 5) != ".json") {
        trigKeyStr = std::string(trigKeyStr) + std::string(".json");
    }

    fs::path filePath = SLT::GetPluginPath() / "extensions" / extKeyStr / trigKeyStr;

    std::error_code ec;

    if (!fs::exists(filePath, ec)) {
        logger::info("Trigger file not found: {}", filePath.string());
        return false;
    }

    if (fs::remove(filePath, ec)) {
        logger::info("Successfully deleted: {}", filePath.string());
        return true;
    } else {
        logger::info("Failed to delete {}: {}", filePath.string(), ec.message());
        return false;
    }
}

RE::TESForm* SLTNativeFunctions::GetForm(PAPYRUS_NATIVE_DECL, std::string_view a_editorID) {
    return FormUtil::Parse::GetForm(a_editorID);
}

std::vector<std::string> SLTNativeFunctions::GetScriptsList(PAPYRUS_NATIVE_DECL) {
    std::vector<std::string> result;

    fs::path scriptsFolderPath = GetPluginPath() / "commands";

    if (fs::exists(scriptsFolderPath)) {
        for (const auto& entry : fs::directory_iterator(scriptsFolderPath)) {
            if (entry.is_regular_file()) {
                auto scriptname = entry.path().filename().string();
                if (scriptname.ends_with(".ini") || scriptname.ends_with(".json")) {
                    result.push_back(scriptname);
                }
            }
        }
    } else {
        logger::error("Scripts folder ({}) doesn't exist. You may need to reinstall the mod.", scriptsFolderPath.string());
    }

    if (result.size() > 1) {
        std::sort(result.begin(), result.end());
    }
    
    return result;
}

SLTSessionId SLTNativeFunctions::GetSessionId(PAPYRUS_NATIVE_DECL) {
    return SLT::GenerateNewSessionId();
}

std::string SLTNativeFunctions::GetTranslatedString(PAPYRUS_NATIVE_DECL, std::string_view input) {
    auto sfmgr = RE::BSScaleformManager::GetSingleton();
    if (!(sfmgr)) {
        return std::string(input);
    }

    auto gfxloader = sfmgr->loader;
    if (!(gfxloader)) {
        return std::string(input);
    }

    auto translator =
        (RE::BSScaleformTranslator*) gfxloader->GetStateBagImpl()->GetStateAddRef(RE::GFxState::StateType::kTranslator);

    if (!(translator)) {
        return std::string(input);
    }

    RE::GFxTranslator::TranslateInfo transinfo;
    RE::GFxWStringBuffer result;

    std::wstring key_utf16 = stl::utf8_to_utf16(input).value_or(L""s);
    transinfo.key = key_utf16.c_str();

    transinfo.result = std::addressof(result);

    translator->Translate(std::addressof(transinfo));

    if (!result.empty()) {
        std::string actualresult = stl::utf16_to_utf8(result).value();
        return actualresult;
    }

    // Fallback: return original string if no translation found
    return std::string(input);
}

std::vector<std::string> SLTNativeFunctions::GetTriggerKeys(PAPYRUS_NATIVE_DECL, std::string_view extensionKey) {
    std::vector<std::string> result;

    fs::path triggerFolderPath = GetPluginPath() / "extensions" / extensionKey;

    if (fs::exists(triggerFolderPath)) {
        for (const auto& entry : fs::directory_iterator(triggerFolderPath)) {
            if (entry.is_regular_file()) {
                if (str::iEquals(entry.path().extension().string(), ".json")) {
                    result.push_back(entry.path().filename().string());
                }
            }
        }
    } else {
        logger::error("Trigger folder ({}) doesn't exist. You may need to reinstall the mod or at least make sure the folder is created.",
            triggerFolderPath.string());
    }
    
    if (result.size() > 1) {
        std::sort(result.begin(), result.end());
    }
    
    return result;
}

bool SLTNativeFunctions::RunOperationOnActor(PAPYRUS_NATIVE_DECL, RE::Actor* cmdTarget, RE::ActiveEffect* cmdPrimary,
    std::vector<std::string> tokens) {
    return OperationRunner::RunOperationOnActor(cmdTarget, cmdPrimary, tokens);
}

void SLTNativeFunctions::SetExtensionEnabled(PAPYRUS_NATIVE_DECL, std::string_view extensionKey, bool enabledState) {
    //SLTExtensionTracker::SetEnabled(extensionKey, enabledState);
    FunctionLibrary* funlib = FunctionLibrary::ByExtensionKey(extensionKey);

    //SLTStackAnalyzer::Walk(stackId);
    if (funlib) {
        funlib->enabled = enabledState;
    } else {
        logger::error("Unable to find function library for extensionKey '{}' to set enabled to '{}'", extensionKey, enabledState);
    }
}

namespace {
bool isNumeric(std::string_view str, float& outValue) {
    const char* begin = str.data();
    const char* end = begin + str.size();

    auto result = std::from_chars(begin, end, outValue);
    return result.ec == std::errc() && result.ptr == end;
}
}

bool SLTNativeFunctions::SmartEquals(PAPYRUS_NATIVE_DECL, std::string_view a, std::string_view b) {
    float aNum = 0.0, bNum = 0.0;
    bool aIsNum = isNumeric(a, aNum);
    bool bIsNum = isNumeric(b, bNum);

    bool outcome = false;
    if (aIsNum && bIsNum) {
        outcome = (std::fabs(aNum - bNum) < FLT_EPSILON);  // safe float comparison
    } else {
        outcome = (a == b);
    }

    return outcome;
}

std::vector<std::string> SLTNativeFunctions::SplitFileContents(PAPYRUS_NATIVE_DECL, std::string_view content_view) {
    std::vector<std::string> lines;
    size_t start = 0;
    size_t i = 0;
    std::string content(content_view.data());
    std::string tmpstr;
    size_t len = content.length();

    while (i < len) {
        if (content[i] == '\r') {
            if (i > start) {
                tmpstr = Util::String::truncateAt(content.substr(start, i - start), ';');
                lines.push_back(tmpstr);
            }
            if (i + 1 < len && content[i + 1] == '\n') {
                i += 2;  // Windows CRLF
            } else {
                i += 1;  // Classic Mac CR
            }
            start = i;
        } else if (content[i] == '\n') {
            if (i > start) {
                tmpstr = Util::String::truncateAt(content.substr(start, i - start), ';');
                lines.push_back(tmpstr);
            }
            i += 1;
            start = i;
        } else {
            i += 1;
        }
    }

    // Add last line if there's any remaining
    if (start < len) {
        tmpstr = Util::String::truncateAt(content.substr(start), ';');
        lines.push_back(tmpstr);
    }

    return lines;
}

bool SLTNativeFunctions::StartScript(PAPYRUS_NATIVE_DECL, RE::Actor* cmdTarget, std::string_view initialScriptName) {
    return ScriptPoolManager::GetSingleton().ApplyScript(cmdTarget, initialScriptName);
}

std::vector<std::string> SLTNativeFunctions::Tokenize(PAPYRUS_NATIVE_DECL, std::string_view input) {
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool inBrackets = false;
    size_t i = 0;

    while (i < input.size()) {
        char c = input[i];

        if (!inQuotes && !inBrackets && c == ';') {
            // Comment detected — ignore rest of line
            break;
        }

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < input.size() && input[i + 1] == '"') {
                    current += '"';  // Escaped quote
                    i += 2;
                } else {
                    inQuotes = false;
                    tokens.push_back(current);
                    current.clear();
                    i++;
                }
            } else {
                current += c;
                i++;
            }
        } else if (inBrackets) {
            if (c == ']') {
                inBrackets = false;
                current = '[' + current + c;
                tokens.push_back(current);
                current.clear();
                i++;
            } else {
                current += c;
                i++;
            }
        } else {
            if (std::isspace(static_cast<unsigned char>(c))) {
                if (!current.empty()) {
                    tokens.push_back(current);
                    current.clear();
                }
                i++;
            } else if (c == '"') {
                inQuotes = true;
                i++;
            } else if (c == '[') {
                inBrackets = true;
                i++;
            } else {
                current += c;
                i++;
            }
        }
    }

    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

std::vector<std::string> SLTNativeFunctions::Tokenizev2(PAPYRUS_NATIVE_DECL, std::string_view input) {
    std::vector<std::string> tokens;
    size_t pos = 0;
    size_t len = input.length();
    
    while (pos < len) {
        // Skip whitespace
        while (pos < len && std::isspace(input[pos])) {
            pos++;
        }
        
        if (pos >= len) break;
        
        // Check for comment - everything from ';' to end of line is ignored
        if (input[pos] == ';') {
            break; // Stop processing, ignore rest of line
        }
        
        // Check for $" (dollar-double-quoted interpolation)
        if (pos + 1 < len && input[pos] == '$' && input[pos + 1] == '"') {
            size_t start = pos;
            pos += 2; // Skip $"
            
            // Find closing quote, handling escaped quotes ""
            while (pos < len) {
                if (input[pos] == '"') {
                    // Check for escaped quote ""
                    if (pos + 1 < len && input[pos + 1] == '"') {
                        pos += 2; // Skip escaped quote pair
                    } else {
                        pos++; // Include the closing quote
                        break; // Found unescaped closing quote
                    }
                } else {
                    pos++;
                }
            }
            
            // Add token with $" prefix, including trailing quote
            tokens.push_back(std::string(input.substr(start, pos - start)));
        }
        // Check for " (double-quoted literal)
        else if (input[pos] == '"') {
            size_t start = pos;
            pos++; // Skip opening quote
            
            // Find closing quote, handling escaped quotes ""
            while (pos < len) {
                if (input[pos] == '"') {
                    // Check for escaped quote ""
                    if (pos + 1 < len && input[pos + 1] == '"') {
                        pos += 2; // Skip escaped quote pair
                    } else {
                        pos++; // Include the closing quote
                        break; // Found unescaped closing quote
                    }
                } else {
                    pos++;
                }
            }
            
            // Add token with leading and trailing quotes
            tokens.push_back(std::string(input.substr(start, pos - start)));
        }
        // Bare token - collect until whitespace
        else {
            size_t start = pos;
            
            while (pos < len && !std::isspace(input[pos])) {
                pos++;
            }
            
            tokens.push_back(std::string(input.substr(start, pos - start)));
        }
    }
    
    return tokens;
}

namespace {
bool IsValidVariableName(const std::string& name) {
    if (name.empty()) return false;
    
    for (char c : name) {
        if (!std::isalnum(c) && c != '_' && c != '.') {
            return false;
        }
    }
    
    // Don't allow names starting or ending with dots
    return name.front() != '.' && name.back() != '.';
}
}

std::vector<std::string> SLTNativeFunctions::TokenizeForVariableSubstitution(PAPYRUS_NATIVE_DECL, std::string_view input) {
    std::vector<std::string> result;
    
    if (input.empty()) {
        return result;
    }
    
    size_t pos = 0;
    std::string currentLiteral;
    
    const std::set<std::string> validScopes = {"local", "thread", "target", "global"};
    
    while (pos < input.length()) {
        size_t openBrace = input.find('{', pos);
        
        if (openBrace == std::string::npos) {
            // No more braces, add remaining text as literal
            currentLiteral += input.substr(pos);
            break;
        }
        
        // Add text before the brace as literal
        currentLiteral += input.substr(pos, openBrace - pos);
        
        // Check for escaped opening brace {{
        if (openBrace + 1 < input.length() && input[openBrace + 1] == '{') {
            currentLiteral += "{";  // Add single literal brace
            pos = openBrace + 2;    // Skip both braces
            continue;
        }
        
        // Find matching closing brace
        size_t closeBrace = input.find('}', openBrace + 1);
        if (closeBrace == std::string::npos) {
            // No matching closing brace, treat as literal
            currentLiteral += input.substr(openBrace);
            break;
        }
        
        // Check for escaped closing brace }}
        if (closeBrace + 1 < input.length() && input[closeBrace + 1] == '}') {
            // This is an escaped closing brace, not end of variable
            currentLiteral += input.substr(openBrace, closeBrace - openBrace + 2);
            currentLiteral.back() = '}';  // Replace second } with single }
            pos = closeBrace + 2;
            continue;
        }
        
        // Extract variable name between braces
        std::string varName = std::string(input.substr(openBrace + 1, closeBrace - openBrace - 1));
        
        // Trim whitespace from variable name
        varName.erase(0, varName.find_first_not_of(" \t"));
        varName.erase(varName.find_last_not_of(" \t") + 1);
        
        if (!varName.empty() && IsValidVariableName(varName)) {
            // Check if variable has a scope and if it's valid
            size_t dotPos = varName.find('.');
            if (dotPos != std::string::npos) {
                std::string scope = varName.substr(0, dotPos);
                if (validScopes.find(scope) == validScopes.end()) {
                    // Invalid scope, remove scope and dot
                    varName = varName.substr(dotPos + 1);
                }
            }
            
            // Add current literal if not empty
            if (!currentLiteral.empty()) {
                result.push_back(currentLiteral);
                currentLiteral.clear();
            }
            
            // Add variable name bare (with $ prefix)
            result.push_back("$" + varName);
        } else {
            // Invalid or empty variable name, treat braces as literal
            currentLiteral += input.substr(openBrace, closeBrace - openBrace + 1);
        }
        
        pos = closeBrace + 1;
    }
    
    // Add final literal if not empty
    if (!currentLiteral.empty()) {
        result.push_back(currentLiteral);
    }
    
    return result;
}

std::string SLTNativeFunctions::Trim(PAPYRUS_NATIVE_DECL, std::string_view str) {
    return Util::String::trim(str);
}


#pragma endregion

};

#pragma pop()