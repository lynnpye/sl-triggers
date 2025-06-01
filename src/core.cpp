#include "core.h"

namespace SLT {
const std::string_view BASE_QUEST = "sl_triggersExtension";
const std::string_view BASE_AME = "sl_triggersCmd";

fs::path GetPluginPath() {
    fs::path pluginPath = fs::path("Data") / "SKSE" / "Plugins" / "sl_triggers";
    return pluginPath;
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

}