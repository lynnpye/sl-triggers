#pragma once

#include <string>
using namespace std::literals;

#include <filesystem>
namespace fs = std::filesystem;

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <coroutine>
#include <cstdint>
#include <format>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <ranges>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
namespace logger = SKSE::log;

#include "core.h"
#include "util.h"
#include "skse_events.h"
#include "bindings.h"
