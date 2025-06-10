#include "forge/forge.h"
#include "engine.h"
#include "sl_triggers.h"

namespace SLT {
    static GlobalContext* g_GlobalContext = nullptr;

// Registration helper macro
#define REGISTER_PAPYRUS_PROVIDER(ProviderClass, ClassName) \
{ \
    ::SKSE::GetPapyrusInterface()->Register([](::RE::BSScript::Internal::VirtualMachine* vm) { \
        static ProviderClass provider; \
        provider.RegisterFunctions(vm, ClassName); \
        return true; \
    }); \
};

    void GameEventHandler::onLoad() {
        g_GlobalContext = &GlobalContext::GetSingleton();
        fs::path debugmsg_log = GetPluginPath() / "debugmsg.log";
        std::ofstream file(debugmsg_log, std::ios::trunc);

        // Register the provider
        REGISTER_PAPYRUS_PROVIDER(SLTPapyrusFunctionProvider, "sl_triggers");
        REGISTER_PAPYRUS_PROVIDER(SLTInternalPapyrusFunctionProvider, "sl_triggers_internal");
    }

    void GameEventHandler::onPostLoad() {
        //logger::info("onPostLoad()");
    }

    void GameEventHandler::onPostPostLoad() {
        //logger::info("onPostPostLoad()");
    }

    void GameEventHandler::onInputLoaded() {
        //logger::info("onInputLoaded()");
    }

    void GameEventHandler::onDataLoaded() {
        FunctionLibrary::PrecacheLibraries();
        ScriptPoolManager::GetSingleton().InitializePool();
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (!ForgeManager::Register(vm)) {
            logger::error("Failed to successfully register Forge");
        }
        
        // Register serialization callbacks
        auto* serialization = SKSE::GetSerializationInterface();
        if (serialization) {
            serialization->SetSaveCallback([](SKSE::SerializationInterface* intfc) {
                if (intfc->OpenRecord('SLTR', 1)) {
                    ForgeManager::OnSave(intfc);
                }
            });
            
            serialization->SetLoadCallback([](SKSE::SerializationInterface* intfc) {
                std::uint32_t type, version, length;
                while (intfc->GetNextRecordInfo(type, version, length)) {
                    if (type == 'SLTR' && version == 1) {
                        ForgeManager::OnLoad(intfc);
                    }
                }
            });
            
            serialization->SetRevertCallback([](SKSE::SerializationInterface* intfc) {
                // Clear all contexts on new game
                ForgeManager::OnRevert(intfc);
            });
        }
    }

    void GameEventHandler::onNewGame() {
        SLT::GenerateNewSessionId(true);
        logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::GetSessionId());
    }

    void GameEventHandler::onPreLoadGame() {
        //logger::info("onPreLoadGame()");
    }

    void GameEventHandler::onPostLoadGame() {
        SLT::GenerateNewSessionId(true);
        logger::info("{} starting session {}", SystemUtil::File::GetPluginName(), SLT::GetSessionId());
    }

    void GameEventHandler::onSaveGame() {
        //logger::info("onSaveGame()");
    }

    void GameEventHandler::onDeleteGame() {
        //logger::info("onDeleteGame()");
    }
}  // namespace plugin


