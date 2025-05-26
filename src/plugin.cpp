#include "registrar.h"
#include "papyrus_impl.h"


#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"
#include "util.h"

SKSEPluginInfo(
    .Version = REL::Version{ 2, 0, 0, 0 },
    .Name = "sl-triggers"sv,
    .Author = "Lynn Pye/Hextun"sv,
    .SupportEmail = "lynnpye@github.com"sv,
    .StructCompatibility = SKSE::StructCompatibility::Independent,
    .RuntimeCompatibility = SKSE::VersionIndependence::AddressLibrary,
    .MinimumSKSEVersion = REL::Version{ 0, 0, 0, 0 }
)


SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    bool r = SKSEReg::Init(skse);

    spdlog::info("{} starting", SystemUtil::File::GetPluginName());

    return r;
}

OnQuit([]{
    spdlog::info("{} shutting down", SystemUtil::File::GetPluginName());
})

OnPostLoadGame([]{
    spdlog::info("{} starting session {}", SystemUtil::File::GetPluginName(), plugin::GetSessionId());
})

OnDeleteGame([]{
    spdlog::info("{} ending session {}", SystemUtil::File::GetPluginName(), plugin::GetSessionId());
}) 