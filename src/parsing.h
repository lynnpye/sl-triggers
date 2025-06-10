#pragma once


namespace SLT {

// Forward declarations
class IScriptParser;
class INIParser;
class JSONParser;

/**
 * ParseResult - Error codes for parsing operations
 */
enum class ParseResult {
    Success,
    NullFrame,
    EmptyScriptName,
    FileNotFound,
    InvalidFileType,
    ReadError,
    SyntaxError,
    NoExecutableCommands
};

/**
 * Parser - Main entry point for script parsing
 * Handles file type detection and delegates to appropriate parser
 */
class Parser {
public:
    // Delete constructor - this is a static-only utility class
    explicit Parser() = delete;
    
    /**
     * ParseScript - Main parsing entry point
     * @param frame - FrameContext to populate with parsed script data
     * @returns ParseResult indicating success or specific failure
     */
    static ParseResult ParseScript(FrameContext* frame);

private:
    /**
     * CreateParser - Factory method to create appropriate parser based on file extension
     * @param extension - File extension (e.g., ".ini", ".json")
     * @returns unique_ptr to parser, or nullptr if unsupported
     */
    static std::unique_ptr<IScriptParser> CreateParser(std::string_view extension);
    
    /**
     * DetermineScriptPath - Resolves script name to actual file path
     * @param scriptName - Name from FrameContext (may or may not have extension)
     * @returns pair of <resolved_path, detected_extension> or empty path if not found
     */
    static std::pair<fs::path, std::string> DetermineScriptPath(std::string_view scriptName);
};

/**
 * IScriptParser - Base interface for all script parsers
 * Each file type (INI, JSON, etc.) should implement this interface
 */
class IScriptParser {
public:
    virtual ~IScriptParser() = default;
    
    /**
     * Parse - Parse the given file and populate the FrameContext
     * @param filePath - Full path to the script file
     * @param frame - FrameContext to populate with parsed tokens
     * @returns ParseResult indicating success or specific failure
     */
    virtual ParseResult Parse(const fs::path& filePath, FrameContext* frame) = 0;
    
    /**
     * GetScriptType - Return the ScriptType enum value for this parser
     */
    virtual ScriptType GetScriptType() const = 0;

protected:
    /**
     * ReadFileContents - Utility to safely read file contents
     * @param filePath - Path to file
     * @param content - Output string to store file contents
     * @returns ParseResult indicating success or specific failure
     */
    static ParseResult ReadFileContents(const fs::path& filePath, std::string& content);
    
    /**
     * CreateCommandLine - Helper to create CommandLine objects
     * @param lineNumber - 1-based line number from source file
     * @param tokens - Tokenized strings from the line
     * @returns unique_ptr to CommandLine
     */
    static std::unique_ptr<CommandLine> CreateCommandLine(std::size_t lineNumber, 
                                                         const std::vector<std::string>& tokens);
};

/**
 * INIParser - Parser for .ini format scripts
 * Handles SLTScript .ini format: line-by-line, forgiving parser
 * - Each line stands alone
 * - Comments start with ';' and go to end of line
 * - Quoted strings with "" escaping, but must close by end of line
 * - [labels] become [":", "label"] tokens
 */
class INIParser : public IScriptParser {
public:
    ParseResult Parse(const fs::path& filePath, FrameContext* frame) override;
    ScriptType GetScriptType() const override { return ScriptType::INI; }

private:
    /**
     * ParseINILine - Parse a single line of INI content
     * @param line - Raw line content
     * @param lineNumber - 1-based line number from original file
     * @returns CommandLine if parseable, nullptr if should be skipped (empty/comment-only)
     */
    std::unique_ptr<CommandLine> ParseINILine(std::string_view line, std::size_t lineNumber);
    
    /**
     * TokenizeINIValue - Split line into tokens (handles quotes, brackets, escaping)
     * @param value - Trimmed line content (after comment removal)
     * @returns vector of tokens
     */
    std::vector<std::string> TokenizeINIValue(std::string_view value);
};

/**
 * JSONParser - Parser for .json format scripts
 * Handles simple {"cmd": [["command", "arg1"], ...]} format
 */
class JSONParser : public IScriptParser {
public:
    ParseResult Parse(const fs::path& filePath, FrameContext* frame) override;
    ScriptType GetScriptType() const override { return ScriptType::JSON; }

private:
    
    /**
     * ProcessJSONCommand - Convert a single JSON command array to CommandLine
     * @param command - JSON array like ["command", "arg1", "arg2"]
     * @param lineNumber - Sequential line number for this command
     * @returns CommandLine object
     */
    std::unique_ptr<CommandLine> ProcessJSONCommand(const nlohmann::json& command, 
                                                   std::size_t lineNumber);
};

} // namespace SLT