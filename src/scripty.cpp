#include <caprica/common/CapricaConfig.h>
#include <caprica/common/CapricaJobManager.h>
#include <caprica/common/CapricaReportingContext.h>
#include <caprica/common/CapricaStats.h>
#include <caprica/common/FakeScripts.h>
#include <caprica/common/FSUtils.h>
#include <caprica/common/GameID.h>
#include <caprica/common/parser/CapricaUserFlagsParser.h>

#include <caprica/papyrus/PapyrusCompilationContext.h>
#include <caprica/papyrus/PapyrusResolutionContext.h>
#include <caprica/papyrus/PapyrusScript.h>
#include <caprica/papyrus/parser/PapyrusParser.h>

#include <caprica/pex/parser/PexAsmParser.h>
#include <caprica/pex/PexAsmWriter.h>
#include <caprica/pex/PexOptimizer.h>
#include <caprica/pex/PexReader.h>
#include <caprica/pex/PexWriter.h>

#include "caprica_main.h"
#include "scripty.h"
#include "bindings.h"

void callCapricaParser(
    const std::vector<std::filesystem::path>& importFolders,
    const std::filesystem::path& capricaOutputFolder,
    const std::filesystem::path& inputFile,
    caprica::CapricaJobManager* jobManager)
{
    std::vector<std::string> args;
    // argv[0] is conventionally the program name
    args.push_back("caprica");

    // Static args
    args.push_back("--game");
    args.push_back("skyrim");

    // Multiple --import folders
    for (const auto& folder : importFolders) {
        args.push_back("--import");
        args.push_back(folder.string());
    }

    args.push_back("--output");
    args.push_back(capricaOutputFolder.string());

    args.push_back("--parallel-compile");
    args.push_back("--optimize");
    args.push_back("--release");
    args.push_back("--disable-all-warnings");
    args.push_back("--quiet");
    args.push_back("--strict");
    args.push_back("--ignorecwd");
    args.push_back("--flags");
    args.push_back((fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "pscfiles" / "TESV_Papyrus_Flags.flg").string());

    args.push_back("--input-file");
    args.push_back(inputFile.string());

    // Now create char* array
    std::vector<char*> argv;
    for (auto& s : args) {
        argv.push_back(s.data());
    }

    int argc = static_cast<int>(argv.size());
    spdlog::info("about to call caprica parser for command line");

    // Call the function
    bool result = caprica::parseCommandLineArguments(argc, argv.data(), jobManager);

    if (!result) {
        spdlog::info("failed to parse command line");
        return;
    } else {
        spdlog::info("parsed options, continuing");
    }

    caprica::papyrus::PapyrusCompilationContext::RenameImports(jobManager);
    
    spdlog::info("done renaming imports, whatever that is");

    try {
        caprica::papyrus::PapyrusCompilationContext::doCompile(jobManager);
        spdlog::info("compiled?");
    } catch (const std::runtime_error &ex) {
        if (ex.what() != std::string(""))
            spdlog::critical("failed to compile :{}:", ex.what());
        return;
    }

    jobManager->awaitShutdown();
}



void scripty::Test() {
    caprica::CapricaJobManager jobManager{};
    
    std::vector<std::filesystem::path> importFolders;
    importFolders.push_back(fs::path("Data") / "Scripts" / "Source");
    importFolders.push_back(fs::path("Data") / "Source" / "Scripts");
    importFolders.push_back(fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "pscfiles");

    std::filesystem::path capricaOutputFolder = fs::path("Data") / "Scripts";

    std::filesystem::path inputFile = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "pscfiles" / "testacular.psc";

    callCapricaParser(importFolders, capricaOutputFolder, inputFile, &jobManager);
}

namespace {
    PLUGIN_BINDINGS_PAPYRUS_CLASS(scripty) {

        PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(Test) {
            scripty::Test();
        };

    }
}