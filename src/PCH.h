#pragma once

#include <string>
using namespace std::literals;

#include <filesystem>
namespace fs = std::filesystem;

#include <cstdint>
#include <format>
#include <functional>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
namespace logger = SKSE::log;

#include "core.h"
#include "skse_events.h"
#include "bindings.h"
