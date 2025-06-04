
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

    return r;
}

OnQuit([]{
    logger::info("{} shutting down", SystemUtil::File::GetPluginName());
})

OnPostLoad([]{
    logger::info("{} starting", SystemUtil::File::GetPluginName());
})
