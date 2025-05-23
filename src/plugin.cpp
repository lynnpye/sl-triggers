SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    bool r = SKSEReg::Init(skse);

    spdlog::info("Plugin starting gold thread");

    return r;
}

OnQuit([]{
    spdlog::info("SKSE plugin shutting down.");
})

OnPostPostLoad([]{
    spdlog::info("starting game play session");
})

OnDeleteGame([]{
    spdlog::info("exiting game play session");
}) 