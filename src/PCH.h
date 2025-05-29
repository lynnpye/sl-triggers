#pragma once

#include <string>
using namespace std::literals;

#include <filesystem>
namespace fs = std::filesystem;

#include <cstdint>
#include <functional>
#include <mutex>
#include <ranges>
#include <unordered_map>
#include <vector>

typedef std::uint32_t SKSEMessageType;
typedef std::int32_t SLTSessionId;
typedef std::int32_t ThreadContextHandle;

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
namespace logger = SKSE::log;

#include "skse_events.h"
#include "bindings.h"
#include "util.h"
