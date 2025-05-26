#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"


class RegistrationClass {
public:
    static bool Init(const SKSE::LoadInterface *skse, std::function<void(SKSE::MessagingInterface::Message*)> msgHandler = nullptr);
    static RegistrationClass& GetSingleton();

    void RegisterMessageListener(uint32_t skseMessageType, std::function<void()> cb);
    void RegisterInitter(std::function<void()> cb);
    void RegisterQuitter(std::function<void()> cb);

public:
    class AutoRegister {
    public:
        AutoRegister(uint32_t skseMsg, std::function<void()> cb);
    };

    class AutoInitter {
    public:
        AutoInitter(std::function<void()> cb);
    };

    class AutoQuitter {
    public:
        AutoQuitter(std::function<void()> cb);
    };

private:
    RegistrationClass() = default;

    std::unordered_map<uint32_t, std::vector<std::function<void()>>> _callbacks;
    std::mutex _mutex;
    static std::function<void(SKSE::MessagingInterface::Message*)> _msgHandler;

    std::vector<std::function<void()>> _initters;
    std::mutex _mutexInitters;

    std::vector<std::function<void()>> _quitters;
    std::mutex _mutexQuitters;

private:
    struct QuitGameHook {
        static void hook();
        static void install();

        static std::string logName;
        static REL::Relocation<decltype(hook)> orig;
        static REL::RelocationID srcFunc;
        static uint64_t srcFuncOffset;
    };
    
};

using SKSEReg = RegistrationClass;
 
#define REGISTRATION_CONCATENATE_DETAIL(x, y) x##y
#define REGISTRATION_CONCATENATE(x, y) REGISTRATION_CONCATENATE_DETAIL(x, y)
#define REGISTRATION_UNIQUE_NAME(prefix) REGISTRATION_CONCATENATE(prefix, __LINE__)

#define OnAfterSKSEInit(lambda) \
namespace { \
    static ::RegistrationClass::AutoInitter REGISTRATION_UNIQUE_NAME(_OnAfterSKSEInit_)(lambda); \
}

#define OnQuit(lambda) \
namespace { \
    static ::RegistrationClass::AutoQuitter REGISTRATION_UNIQUE_NAME(_OnQuit_)(lambda); \
}

#define OnPostLoad(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostLoad_)(::SKSE::MessagingInterface::kPostLoad, lambda); \
}

#define OnPostPostLoad(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostPostLoad_)(::SKSE::MessagingInterface::kPostPostLoad, lambda); \
}

#define OnPreLoadGame(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPreLoadGame_)(::SKSE::MessagingInterface::kPreLoadGame, lambda); \
}

#define OnPostLoadGame(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostLoadGame_)(::SKSE::MessagingInterface::kPostLoadGame, lambda); \
}

#define OnSaveGame(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnSaveGame_)(::SKSE::MessagingInterface::kSaveGame, lambda); \
}

#define OnDeleteGame(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnDeleteGame_)(::SKSE::MessagingInterface::kDeleteGame, lambda); \
}

#define OnInputLoaded(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnInputLoaded_)(::SKSE::MessagingInterface::kInputLoaded, lambda); \
}

#define OnNewGame(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnNewGame_)(::SKSE::MessagingInterface::kNewGame, lambda); \
}

#define OnDataLoaded(lambda) \
namespace { \
    static ::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnDataLoaded_)(::SKSE::MessagingInterface::kDataLoaded, lambda); \
}