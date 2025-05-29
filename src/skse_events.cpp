#include "hooks.h"

#include <spdlog/sinks/basic_file_sink.h>

std::function<void(SKSE::MessagingInterface::Message*)> SLT::RegistrationClass::_msgHandler = nullptr;

void SLT::RegistrationClass::QuitGameHook::hook() {
    {
        auto& reg = SLT::RegistrationClass::GetSingleton();
        std::lock_guard<std::mutex> lock(reg._mutexQuitters);
        for (auto& cb : reg._quitters) {
            cb();
        }
    }
    orig();
}

void SLT::RegistrationClass::QuitGameHook::install() {
    Hooking::writeCall<SLT::RegistrationClass::QuitGameHook>();
}

std::string SLT::RegistrationClass::QuitGameHook::logName = "QuitGame";
REL::Relocation<decltype(SLT::RegistrationClass::QuitGameHook::hook)> SLT::RegistrationClass::QuitGameHook::orig;
REL::RelocationID SLT::RegistrationClass::QuitGameHook::srcFunc = REL::RelocationID{35545, 36544};
uint64_t SLT::RegistrationClass::QuitGameHook::srcFuncOffset = REL::Relocate(0x35, 0x1AE);

bool SLT::RegistrationClass::Init(const SKSE::LoadInterface *skse, std::function<void(SKSE::MessagingInterface::Message*)> msgHandler) {
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
        auto& reg = SLT::RegistrationClass::GetSingleton();
        std::vector<std::function<void()>> toCall;
        {
            std::lock_guard<std::mutex> lock(reg._mutex);
            auto it = reg._callbacks.find(msg->type);
            if (it != reg._callbacks.end())
                toCall = it->second; // Safe copy
        }
        for (auto& cb : toCall)
            cb();

        if (SLT::RegistrationClass::_msgHandler) {
            SLT::RegistrationClass::_msgHandler(msg);
        }
    });
 
    {
        auto& reg = SLT::RegistrationClass::GetSingleton();
        std::lock_guard<std::mutex> lock(reg._mutexInitters);
        for (auto& cb : reg._initters) {
            cb();
        }
    }

    return true;
}

SLT::RegistrationClass& SLT::RegistrationClass::GetSingleton() {
    static SLT::RegistrationClass instance;
    return instance;
}

void SLT::RegistrationClass::RegisterMessageListener(SKSEMessageType skseMessageType, std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks[skseMessageType].push_back(std::move(cb));
}

SLT::RegistrationClass::AutoRegister::AutoRegister(SKSEMessageType skseMsg, std::function<void()> cb) {
    SLT::RegistrationClass::GetSingleton().RegisterMessageListener(skseMsg, cb);
}

void SLT::RegistrationClass::RegisterInitter(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutexInitters);
    _initters.push_back(std::move(cb));
}

SLT::RegistrationClass::AutoInitter::AutoInitter(std::function<void()> cb) {
    SLT::RegistrationClass::GetSingleton().RegisterInitter(cb);
}

void SLT::RegistrationClass::RegisterQuitter(std::function<void()> cb) {
    std::lock_guard<std::mutex> lock(_mutexQuitters);
    _quitters.push_back(std::move(cb));
}

SLT::RegistrationClass::AutoQuitter::AutoQuitter(std::function<void()> cb) {
    SLT::RegistrationClass::GetSingleton().RegisterQuitter(cb);
}
