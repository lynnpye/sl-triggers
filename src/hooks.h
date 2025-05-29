#pragma once

namespace SLT {
struct Hooking {
    template <class T, size_t size = 5>
    static void writeCall() {
        SKSE::AllocTrampoline(64);
        auto& trampoline = SKSE::GetTrampoline();
        uintptr_t addrCall = T::srcFunc.address() + T::srcFuncOffset;
        T::orig = trampoline.write_call<size>(addrCall, T::hook);
        if constexpr (requires { T::logName; }) {
            if constexpr (requires { T::srcFunc.id(); }) {
                SKSE::log::info("{} hook installed at address 0x{:X} (id {} + 0x{:X})", T::logName, T::srcFunc.offset(),
                                T::srcFunc.id(), T::srcFuncOffset);
            } else {
                SKSE::log::info("{} hook installed at address 0x{:X} + 0x{:X}", T::logName, T::srcFunc.offset(), T::srcFuncOffset);
            }
        }
    }

    template <class T, size_t size = 5>
    static void writeCall(uintptr_t srcFuncAddress, int64_t srcFuncOffset) {
        SKSE::AllocTrampoline(64);
        auto& trampoline = SKSE::GetTrampoline();
        uintptr_t addrCall = srcFuncAddress + srcFuncOffset;
        T::orig = trampoline.write_call<size>(addrCall, T::hook);
        if constexpr (requires { T::logName; }) {
            if constexpr (requires { T::srcFunc.id(); }) {
                SKSE::log::info("{} hook installed at address 0x{:X} (id {} + 0x{:X})", T::logName, T::srcFunc.offset(),
                                T::srcFunc.id(), T::srcFuncOffset);
            } else {
                SKSE::log::info("{} hook installed at address 0x{:X} + 0x{:X}", T::logName, srcFuncAddress, T::srcFuncOffset);
            }
        }
    }
};
}