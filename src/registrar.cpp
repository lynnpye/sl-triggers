#include <spdlog/sinks/basic_file_sink.h>

std::function<void(SKSE::MessagingInterface::Message*)> RegistrationClass::_msgHandler = nullptr;

void RegistrationClass::QuitGameHook::hook() {
    {
        auto& reg = RegistrationClass::GetSingleton();
        std::lock_guard<std::mutex> lock(reg._mutexQuitters);
        for (auto& cb : reg._quitters) {
            cb();
        }
    }
    orig();
}

void RegistrationClass::QuitGameHook::install() {
    Hooking::writeCall<RegistrationClass::QuitGameHook>();
}

std::string RegistrationClass::QuitGameHook::logName = "QuitGame";
REL::Relocation<decltype(RegistrationClass::QuitGameHook::hook)> RegistrationClass::QuitGameHook::orig;
REL::RelocationID RegistrationClass::QuitGameHook::srcFunc = REL::RelocationID{35545, 36544};
uint64_t RegistrationClass::QuitGameHook::srcFuncOffset = REL::Relocate(0x35, 0x1AE);

bool RegistrationClass::Init(const SKSE::LoadInterface *skse, std::function<void(SKSE::MessagingInterface::Message*)> msgHandler) {
    if (msgHandler) {
        _msgHandler = std::move(msgHandler);
    }

    auto logsFolder = SKSE::log::log_directory();
    if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
    auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
    auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
    auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
    auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
    spdlog::set_default_logger(std::move(loggerPtr));
    spdlog::set_level(spdlog::level::trace);
    spdlog::flush_on(spdlog::level::trace);

    SKSE::Init(skse);

    ::SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* msg) {
        auto& reg = RegistrationClass::GetSingleton();
        std::vector<std::function<void()>> toCall;
        {
            std::lock_guard<std::mutex> lock(reg._mutex);
            auto it = reg._callbacks.find(msg->type);
            if (it != reg._callbacks.end())
                toCall = it->second; // Safe copy
        }
        for (auto& cb : toCall)
            cb();

        if (RegistrationClass::_msgHandler) {
            RegistrationClass::_msgHandler(msg);
        }
    });
 
    {
        auto& reg = RegistrationClass::GetSingleton();
        std::lock_guard<std::mutex> lock(reg._mutexInitters);
        for (auto& cb : reg._initters) {
            cb();
        }
    }

    return true;
}

RegistrationClass& RegistrationClass::GetSingleton() {
    static RegistrationClass instance;
    return instance;
}

void RegistrationClass::RegisterMessageListener(uint32_t skseMessageType, std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks[skseMessageType].push_back(std::move(cb));
}

RegistrationClass::AutoRegister::AutoRegister(uint32_t skseMsg, std::function<void()> cb) {
    RegistrationClass::GetSingleton().RegisterMessageListener(skseMsg, cb);
}

void RegistrationClass::RegisterInitter(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutexInitters);
    _initters.push_back(std::move(cb));
}

RegistrationClass::AutoInitter::AutoInitter(std::function<void()> cb) {
    RegistrationClass::GetSingleton().RegisterInitter(cb);
}

void RegistrationClass::RegisterQuitter(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutexQuitters);
    _quitters.push_back(std::move(cb));
}

RegistrationClass::AutoQuitter::AutoQuitter(std::function<void()> cb) {
    RegistrationClass::GetSingleton().RegisterQuitter(cb);
}
