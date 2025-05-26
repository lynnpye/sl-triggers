#pragma once

#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <common/CapricaJobManager.h>
#include <common/CapricaConfig.h>
#include <common/GameID.h>
#include <common/FSUtils.h>
#include <common/parser/CapricaPPJParser.h>
#include <common/parser/PapyrusProject.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <papyrus/PapyrusCompilationContext.h>
#include <string>
#include <utility>
namespace conf = caprica::conf;
namespace po = boost::program_options;
namespace filesystem = std::filesystem;
#pragma once

#include <boost/program_options.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <utility>
#include <memory>
#include <map>

// Forward declarations for Caprica types.
namespace caprica {

bool addFilesFromDirectory(
    const IInputFile& input,
    const std::filesystem::path& baseOutputDir,
    CapricaJobManager* jobManager,
    papyrus::PapyrusCompilationNode::NodeType nodeType,
    const std::string& startingNS = ""
);

void parseUserFlags(std::string&& flagsPath);

bool handleImports(
    const std::vector<ImportDir>& f,
    CapricaJobManager* jobManager
);

bool addSingleFile(
    const IInputFile& input,
    const std::filesystem::path& baseOutputDir,
    CapricaJobManager* jobManager,
    papyrus::PapyrusCompilationNode::NodeType nodeType
);

std::pair<std::string, std::string> parseOddArguments(const std::string& str);

void SaveDefaultConfig(
    const boost::program_options::options_description& descOptions,
    const std::string& configFilePath_,
    const boost::program_options::variables_map& vm
);

std::string findFlags(
    const std::filesystem::path& flagsPath,
    const std::filesystem::path& progamBasePath,
    const std::filesystem::path& baseOutputDir
);

void setPPJBool(BoolSetting value, bool& setting);

bool setGame(std::string gameType);

bool parseCommandLineArguments(
    int argc,
    char* argv[],
    CapricaJobManager* jobManager
);

} // namespace caprica
