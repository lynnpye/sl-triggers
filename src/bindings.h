#pragma once

namespace SLT {
    namespace binding {
        namespace detail {
            class PapyrusStaticFunctionBinder {
            public:
                [[nodiscard]] static RE::BSTSmartPointer<RE::BSScript::Stack> GetStack(RE::VMStackID stackID) noexcept;

            protected:
                inline PapyrusStaticFunctionBinder() noexcept = default;

                class Function {
                public:
                    inline explicit Function(std::string_view className, std::string_view functionName,
                                                    RE::BSScript::IVirtualMachine* vm) noexcept
                            : _className(className), _functionName(functionName), _vm(vm) {
                    }

                    template <class Return, class... Args>
                    void operator>>(Return(*function)(RE::BSScript::Internal::VirtualMachine*,
                            const RE::VMStackID, Args...)) {
                        _vm->RegisterFunction(
                                _functionName, _className, reinterpret_cast<Return(*)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, Args...)>(
                                        function));
                    }

                private:
                    std::string_view _className;
                    std::string_view _functionName;
                    RE::BSScript::IVirtualMachine* _vm;

                    friend class PapyrusStaticFunctionBinder;
                };
            };
        }
    }
}

#define PLUGIN_BINDINGS_CONCAT(x, y) x ## y

#define PLUGIN_BINDINGS_PAPYRUS_CLASS(name) namespace { \
    struct PLUGIN_BINDINGS_CONCAT(PapyrusStaticFunctionBinder, name) : public ::SLT::binding::detail::PapyrusStaticFunctionBinder { \
        inline PLUGIN_BINDINGS_CONCAT(PapyrusStaticFunctionBinder, name)() {                                                   \
            ::SKSE::GetPapyrusInterface()->Register(+[](::RE::BSScript::Internal::VirtualMachine* vm) {         \
                Initialize(#name, vm); \
                return true;           \
            });                        \
        }                              \
                                       \
        private:                       \
            static inline void Initialize(std::string_view ClassName, ::RE::BSScript::Internal::VirtualMachine* VirtualMachine); \
    };                                 \
    OnAfterSKSEInit([]{                     \
        PLUGIN_BINDINGS_CONCAT(PapyrusStaticFunctionBinder, name) PLUGIN_BINDINGS_CONCAT(__papyrusClass, name);                         \
    });                                  \
}                                      \
void PLUGIN_BINDINGS_CONCAT(PapyrusStaticFunctionBinder, name)::Initialize([[maybe_unused]] const std::string_view ClassName,  \
    [[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* VirtualMachine)

#define PLUGIN_BINDINGS_PAPYRUS_NONSTATIC_FUNCTION(name, ...) Function(ClassName, #name, VirtualMachine) >> +[]([[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* VirtualMachine, [[maybe_unused]] const ::RE::VMStackID StackID, __VA_ARGS__)

#define PLUGIN_BINDINGS_PAPYRUS_STATIC_FUNCTION(name, ...) Function(ClassName, #name, VirtualMachine) >> +[]([[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* VirtualMachine, [[maybe_unused]] const ::RE::VMStackID StackID, ::RE::StaticFunctionTag* __VA_OPT__(,) __VA_ARGS__)

#define PLUGIN_CURRENT_STACK GetStack(StackID)
