#pragma once

namespace SLT {
class RegistrationClass {
public:
    static bool Init(const SKSE::LoadInterface *skse, std::function<void(SKSE::MessagingInterface::Message*)> msgHandler = nullptr);
    static RegistrationClass& GetSingleton();

    void RegisterMessageListener(SKSEMessageType skseMessageType, std::function<void()> cb);
    void RegisterInitter(std::function<void()> cb);
    void RegisterQuitter(std::function<void()> cb);

public:
    class AutoRegister {
    public:
        AutoRegister(SKSEMessageType skseMsgType, std::function<void()> cb);
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

    std::unordered_map<SKSEMessageType, std::vector<std::function<void()>>> _callbacks;
    mutable std::mutex _mutex;
    static std::function<void(SKSE::MessagingInterface::Message*)> _msgHandler;

    std::vector<std::function<void()>> _initters;
    mutable std::mutex _mutexInitters;

    std::vector<std::function<void()>> _quitters;
    mutable std::mutex _mutexQuitters;

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
}

using SKSEReg = SLT::RegistrationClass;
 
#define REGISTRATION_CONCATENATE_DETAIL(x, y) x##y
#define REGISTRATION_CONCATENATE(x, y) REGISTRATION_CONCATENATE_DETAIL(x, y)
#define REGISTRATION_UNIQUE_NAME(prefix) REGISTRATION_CONCATENATE(prefix, __LINE__)

#define OnAfterSKSEInit(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoInitter REGISTRATION_UNIQUE_NAME(_OnAfterSKSEInit_)(lambda); \
}

#define OnQuit(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoQuitter REGISTRATION_UNIQUE_NAME(_OnQuit_)(lambda); \
}

#define OnPostLoad(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostLoad_)(::SKSE::MessagingInterface::kPostLoad, lambda); \
}

#define OnPostPostLoad(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostPostLoad_)(::SKSE::MessagingInterface::kPostPostLoad, lambda); \
}

#define OnPreLoadGame(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPreLoadGame_)(::SKSE::MessagingInterface::kPreLoadGame, lambda); \
}

#define OnPostLoadGame(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnPostLoadGame_)(::SKSE::MessagingInterface::kPostLoadGame, lambda); \
}

#define OnSaveGame(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnSaveGame_)(::SKSE::MessagingInterface::kSaveGame, lambda); \
}

#define OnDeleteGame(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnDeleteGame_)(::SKSE::MessagingInterface::kDeleteGame, lambda); \
}

#define OnInputLoaded(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnInputLoaded_)(::SKSE::MessagingInterface::kInputLoaded, lambda); \
}

#define OnNewGame(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnNewGame_)(::SKSE::MessagingInterface::kNewGame, lambda); \
}

#define OnDataLoaded(lambda) \
namespace { \
    static SLT::RegistrationClass::AutoRegister REGISTRATION_UNIQUE_NAME(_OnDataLoaded_)(::SKSE::MessagingInterface::kDataLoaded, lambda); \
}