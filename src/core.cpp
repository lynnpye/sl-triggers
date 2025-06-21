
namespace SLT {

#pragma region Core basics
const std::string_view BASE_QUEST = "sl_triggersExtension";
const std::string_view BASE_AME = "sl_triggersCmd";

fs::path GetPluginPath() {
    fs::path pluginPath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers";
    return pluginPath;
}

fs::path GetScriptfilePath(std::string_view scriptfilename) {
    std::string strfn(scriptfilename);
    fs::path scriptfilepath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers" / "commands" / strfn;
    return scriptfilepath;
}
    
SLTSessionId sessionId;
bool sessionIdGenerated = false;

SLTSessionId GenerateNewSessionId(bool force) {
    if (!sessionIdGenerated || force) {
        static std::random_device rd;
        static std::mt19937 engine(rd());
        static std::uniform_int_distribution<std::int32_t> dist(std::numeric_limits<std::int32_t>::min(),
                                                                std::numeric_limits<std::int32_t>::max());
        sessionId = dist(engine);
    }
    return sessionId;
}

SLTSessionId GetSessionId() {
    return sessionId;
}
#pragma endregion

#pragma region ScriptPoolManager

bool ScriptPoolManager::ApplyScript(RE::Actor* target, std::string_view scriptName) {
    if (!target) {
        logger::error("Invalid caster or target for script application");
        return false;
    }
    
    try {
        // Find an available MGEF
        auto availableMGEF = FindAvailableMGEF(target);
        if (!availableMGEF) {
            logger::error("No available magic effects to apply script: {}", scriptName);
            return false;
        }
        
        // Find the corresponding spell
        auto spell = FindSpellForMGEF(availableMGEF);
        if (!spell) {
            logger::error("Could not find spell for MGEF when applying script: {}", scriptName);
            return false;
        }
        
        // Cast the spell
        auto* magicCaster = target->GetMagicCaster(RE::MagicSystem::CastingSource::kInstant);
        if (magicCaster) {
            magicCaster->CastSpellImmediate(spell, false, target, 1.0f, false, 0.0f, target);
            return true;
        } else {
            logger::error("Failed to get magic caster for script application");
        }
    } catch (...) {
        logger::error("Unknown/unexpected exception in ApplyScript");
    }

    return false;
}
#pragma endregion
}