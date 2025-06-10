#include "parsing.h"
#include "forge/forge.h"

using namespace SLT;

namespace {
    // Helper to check if a line should be skipped (empty or comment-only)
    bool ShouldSkipLine(std::string_view line) {
        // Trim whitespace and check if empty or starts with comment
        auto trimmed = Util::String::trim(line);
        return trimmed.empty() || trimmed[0] == ';';
    }

    std::string TrimString(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, last - first + 1);
    }

    // Modern C++ way: Use std::istringstream with std::getline
    std::vector<std::string> SplitLinesPreservingCount(const std::string& content) {
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        
        // std::getline handles \n, \r\n, and \r automatically
        while (std::getline(stream, line)) {
            lines.push_back(TrimString(line));
        }
        
        // Handle case where file doesn't end with newline
        if (!content.empty() && content.back() != '\n' && content.back() != '\r') {
            // Last line was already handled by the while loop
        }
        
        return lines;
    }
}

#pragma region Parser Implementation

namespace {
    // Label extraction
    std::string ExtractLabelName(const std::vector<std::string>& tokens, ScriptType scriptType) {
        if (tokens.empty()) {
            return "";
        }
        
        if (scriptType == ScriptType::INI) {
            // INI format: [labelname]
            if (tokens.size() == 1) {
                const std::string& token = tokens[0];
                if (token.length() > 2 && token.front() == '[' && token.back() == ']') {
                    return token.substr(1, token.length() - 2);
                }
            }
        } else if (scriptType == ScriptType::JSON) {
            // JSON format: [":", "labelname"] (already normalized by parser)
            if (tokens.size() >= 2 && tokens[0] == ":") {
                return tokens[1];
            }
        }
        
        return "";
    }

    // Build label maps for goto/gosub
    void BuildLabelMaps(FrameContext* frame) {
        if (!frame || frame->scriptTokens.empty()) {
            return;
        }
        
        frame->gotoLabelMap.clear();
        frame->gosubLabelMap.clear();
        
        for (size_t i = 0; i < frame->scriptTokens.size(); ++i) {
            const auto cmdLine = frame->GetCommandLine(i);
            if (!cmdLine || cmdLine->tokens.empty()) {
                continue;
            }
            
            // GOTO labels: [LABELNAME] style
            std::string labelName = ExtractLabelName(cmdLine->tokens, frame->commandType);
            if (!labelName.empty()) {
                frame->gotoLabelMap[labelName] = i;
            }
            
            // GOSUB labels: "beginsub LABELNAME" style  
            if (cmdLine->tokens.size() >= 2 && 
                str::iEquals(cmdLine->tokens[0], "beginsub")) {
                std::string subName = cmdLine->tokens[1];
                if (!subName.empty()) {
                    logger::debug("gosub label added [{}]({})", subName, i);
                    frame->gosubLabelMap[subName] = i; // Point to the beginsub line
                }
            }
        }
    }
}

ParseResult Parser::ParseScript(FrameContext* frame) {
    if (!frame) {
        logger::error("ParseScript: FrameContext cannot be null");
        return ParseResult::NullFrame;
    }
    
    if (frame->scriptName.empty()) {
        logger::error("ParseScript: Script name cannot be empty");
        return ParseResult::EmptyScriptName;
    }
    
    // Determine the actual file path and extension
    auto [scriptPath, extension] = DetermineScriptPath(frame->scriptName);
    
    if (scriptPath.empty()) {
        logger::error("ParseScript: Could not find script file: {}", frame->scriptName);
        return ParseResult::FileNotFound;
    }
    
    // Create appropriate parser
    auto parser = CreateParser(extension);
    if (!parser) {
        logger::error("ParseScript: Unsupported script file type: {}", extension);
        return ParseResult::InvalidFileType;
    }
    
    // Parse the script
    ParseResult result = parser->Parse(scriptPath, frame);
    if (result != ParseResult::Success) {
        logger::error("ParseScript: Failed to parse '{}': error code {}", 
                     frame->scriptName, static_cast<int>(result));
        return result;
    }
    
    // Set the script type
    frame->commandType = parser->GetScriptType();
    
    // Reset execution state
    frame->currentLine = 0;
    frame->isReady = false;
    frame->isReadied = false;

    BuildLabelMaps(frame);
    
    return ParseResult::Success;
}

std::unique_ptr<IScriptParser> Parser::CreateParser(std::string_view extension) {
    if (str::iEquals(extension, ".ini")) {
        return std::make_unique<INIParser>();
    } else if (str::iEquals(extension, ".json")) {
        return std::make_unique<JSONParser>();
    }
    
    return nullptr;
}

std::pair<fs::path, std::string> Parser::DetermineScriptPath(std::string_view scriptName) {
    fs::path baseFolder = SLT::GetPluginPath() / "commands";
    fs::path scriptPath(scriptName);
    
    // Check if scriptName already has an extension using std::filesystem
    if (scriptPath.has_extension()) {
        fs::path fullPath = baseFolder / scriptPath;
        if (fs::exists(fullPath)) {
            std::string extension = scriptPath.extension().string();
            return {fullPath, extension};
        }
        return {fs::path{}, std::string{}};  // File not found
    }
    
    // No extension - try .ini first, then .json
    std::vector<std::string> extensions = {".ini", ".json"};
    
    for (const auto& ext : extensions) {
        fs::path fullPath = baseFolder / (std::string(scriptName) + ext);
        if (fs::exists(fullPath)) {
            return {fullPath, ext};
        }
    }
    
    return {fs::path{}, std::string{}};  // No file found with any extension
}

#pragma endregion

#pragma region IScriptParser Base Implementation

ParseResult IScriptParser::ReadFileContents(const fs::path& filePath, std::string& content) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logger::error("ReadFileContents: Cannot open file: {}", filePath.string());
        return ParseResult::ReadError;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    if (file.bad()) {
        logger::error("ReadFileContents: Error reading file: {}", filePath.string());
        return ParseResult::ReadError;
    }
    
    content = buffer.str();
    return ParseResult::Success;
}

std::unique_ptr<CommandLine> IScriptParser::CreateCommandLine(std::size_t lineNumber, 
                                                             const std::vector<std::string>& tokens) {
    return std::make_unique<CommandLine>(lineNumber, tokens);
}

#pragma endregion

#pragma region INIParser Implementation

ParseResult INIParser::Parse(const fs::path& filePath, FrameContext* frame) {
    if (!frame) {
        return ParseResult::NullFrame;
    }
    
    std::string content;
    ParseResult readResult = ReadFileContents(filePath, content);
    if (readResult != ParseResult::Success) {
        return readResult;
    }
    
    // Split by lines but preserve ALL lines (including empty ones)
    std::vector<std::string> allLines = SplitLinesPreservingCount(content);
    
    frame->scriptTokens.clear();
    
    // Process each line with correct line numbering
    for (std::size_t i = 0; i < allLines.size(); ++i) {
        const std::string& line = allLines[i];
        std::size_t actualLineNumber = i + 1; // 1-based, corresponds to actual file line
        
        auto commandLine = ParseINILine(line, actualLineNumber);
        if (commandLine) {
            frame->scriptTokens.push_back(CommandLineManager::Create(std::move(commandLine)));
        }
        // Note: We don't add anything for empty/comment lines, but line numbers stay correct
    }
    
    if (frame->scriptTokens.empty()) {
        logger::warn("INI script contains no executable commands: {}", filePath.string());
        return ParseResult::NoExecutableCommands;
    }
    
    return ParseResult::Success;
}

std::unique_ptr<CommandLine> INIParser::ParseINILine(std::string_view line, std::size_t lineNumber) {
    // Handle comments - find semicolon and strip everything after it
    std::string cleanLine(line);
    auto commentPos = cleanLine.find(';');
    if (commentPos != std::string::npos) {
        cleanLine = cleanLine.substr(0, commentPos);
        cleanLine = Util::String::trim(cleanLine);
    }
    
    if (cleanLine.empty()) {
        return nullptr; // Comment-only or empty line
    }
    
    // Check for label definition: [labelname]
    if (cleanLine.front() == '[' && cleanLine.back() == ']') {
        std::string labelName = cleanLine.substr(1, cleanLine.length() - 2);
        labelName = Util::String::trim(labelName);
        if (!labelName.empty()) {
            std::string normalizedLabel = "[" + labelName + "]";
            return CreateCommandLine(lineNumber, {normalizedLabel});
        }
        return nullptr;
    }
    
    // Regular command line - use your existing tokenizer
    // This reuses SLTNativeFunctions::Tokenize logic
    auto tokens = TokenizeINIValue(cleanLine);
    
    if (tokens.empty()) {
        return nullptr;
    }
    
    return CreateCommandLine(lineNumber, tokens);
}

std::vector<std::string> INIParser::TokenizeINIValue(std::string_view value) {
    // This is your existing tokenization logic from SLTNativeFunctions::Tokenize
    // Moved here to avoid circular dependencies
    
    std::vector<std::string> tokens;
    std::string current;
    bool inQuotes = false;
    bool inBrackets = false;
    size_t i = 0;

    while (i < value.size()) {
        char c = value[i];

        if (!inQuotes && !inBrackets && c == ';') {
            // Comment detected — ignore rest of line
            break;
        }

        if (inQuotes) {
            if (c == '"') {
                if (i + 1 < value.size() && value[i + 1] == '"') {
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

#pragma endregion

#pragma region JSONParser Implementation

ParseResult JSONParser::Parse(const fs::path& filePath, FrameContext* frame) {
    if (!frame) {
        return ParseResult::NullFrame;
    }
    
    std::string content;
    ParseResult readResult = ReadFileContents(filePath, content);
    if (readResult != ParseResult::Success) {
        return readResult;
    }
    
    nlohmann::json jsonData;
    try {
        jsonData = nlohmann::json::parse(content);
    } catch (const nlohmann::json::parse_error& e) {
        logger::error("JSON parse error in {}: {}", filePath.string(), e.what());
        return ParseResult::SyntaxError;
    } catch (const std::exception& e) {
        logger::error("JSON parsing failed for {}: {}", filePath.string(), e.what());
        return ParseResult::SyntaxError;
    }
    
    try {
        frame->scriptTokens.clear();// = ParseJSONCommands(jsonData);
        
    
        // Expected format: {"cmd": [["command", "arg1", "arg2"], ["command2", "arg1"], ...]}
        if (!jsonData.is_object()) {
            logger::error("JSON root must be an object");
        }
        else if (!jsonData.contains("cmd")) {
            logger::error("JSON must contain 'cmd' array");
        }
        else {
            const auto& cmdArray = jsonData["cmd"];
            if (!cmdArray.is_array()) {
                logger::error("'cmd' must be an array");
            }
            else {
                std::size_t lineNumber = 1;
                for (const auto& command : cmdArray) {
                    auto cmdLine = ProcessJSONCommand(command, lineNumber++);
                    if (cmdLine) {
                        frame->scriptTokens.push_back(CommandLineManager::Create(std::move(cmdLine)));
                    }
                }
            }
        }
    } catch (const std::exception& e) {
        logger::error("JSON command processing failed for {}: {}", filePath.string(), e.what());
        return ParseResult::SyntaxError;
    }
    
    if (frame->scriptTokens.empty()) {
        logger::warn("JSON script contains no executable commands: {}", filePath.string());
        return ParseResult::NoExecutableCommands;
    }
    
    return ParseResult::Success;
}

std::unique_ptr<CommandLine> JSONParser::ProcessJSONCommand(const nlohmann::json& command, 
                                                           std::size_t lineNumber) {
    // Expected format: ["command", "arg1", "arg2", ...]
    if (!command.is_array()) {
        return nullptr; // Skip invalid commands
    }
    
    if (command.empty()) {
        return nullptr; // Skip empty arrays
    }
    
    std::vector<std::string> tokens;
    
    // Convert each JSON element to string
    for (const auto& element : command) {
        if (element.is_string()) {
            tokens.push_back(element.get<std::string>());
        } else if (element.is_number_integer()) {
            tokens.push_back(std::to_string(element.get<int>()));
        } else if (element.is_number_float()) {
            tokens.push_back(std::to_string(element.get<double>()));
        } else if (element.is_boolean()) {
            tokens.push_back(element.get<bool>() ? "true" : "false");
        } else if (element.is_null()) {
            tokens.push_back("");
        } else {
            // For any other type, serialize to string
            tokens.push_back(element.dump());
        }
    }
    
    // NORMALIZE ALL LABELS TO [LABELNAME] FORMAT
    // JSON: [":", "hereIam"] becomes ["[hereIam]"]
    if (tokens.size() >= 2 && tokens[0] == ":") {
        std::string labelName = tokens[1];
        std::string normalizedLabel = "[" + labelName + "]";
        return CreateCommandLine(lineNumber, {normalizedLabel});
    }
    
    // Regular command - pass through as-is
    return CreateCommandLine(lineNumber, tokens);
}

#pragma endregion