#pragma once

// Parameter macros for common Papyrus function signatures
#define PAPYRUS_STATIC_ARGS [[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* vm, [[maybe_unused]] const ::RE::VMStackID stackId, [[maybe_unused]] RE::StaticFunctionTag*

#define PAPYRUS_INSTANCE_ARGS [[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* vm, [[maybe_unused]] const ::RE::VMStackID stackId

#define PAPYRUS_NATIVE_DECL [[maybe_unused]] ::RE::BSScript::Internal::VirtualMachine* vm, [[maybe_unused]] const ::RE::VMStackID stackId

#define PAPYRUS_FN_PARMS vm, stackId

namespace SLT {
    namespace binding {
        // Base class for function providers
        class BasePapyrusFunctionProvider {
        public:
            virtual ~BasePapyrusFunctionProvider() = default;
            virtual void RegisterFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) = 0;
        };

        // Template-based registrar for clean registration
        template<typename T>
        class PapyrusRegistrar {
            RE::BSScript::Internal::VirtualMachine* vm_;
            std::string className_;
            
        public:
            PapyrusRegistrar(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) 
                : vm_(vm), className_(className) {}
            
            // Static function registration
            template<typename Return, typename... Args>
            void RegisterStatic(std::string_view name, Return(*func)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Args...), bool callableFromTasklets = false) {
                auto wrapper = reinterpret_cast<Return(*)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Args...)>(func);
                vm_->RegisterFunction(name, className_, wrapper, callableFromTasklets);
            }
            
            // Static latent function registration
            template<typename Return, typename... Args>
            void RegisterStaticLatent(std::string_view name, RE::BSScript::LatentStatus(*func)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, RE::StaticFunctionTag*, Args...), bool callableFromTasklets = false) {
                vm_->RegisterLatentFunction<Return>(name, className_, func, callableFromTasklets);
            }
            
            // Instance function registration (for future use)
            template<typename Return, typename Instance, typename... Args>
            void RegisterInstance(std::string_view name, Return(*func)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, Instance*, Args...), bool callableFromTasklets = false) {
                auto wrapper = reinterpret_cast<Return(*)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, Instance*, Args...)>(func);
                vm_->RegisterFunction(name, className_, wrapper, callableFromTasklets);
            }
            
            // Instance latent function registration (for future use)
            template<typename Return, typename Instance, typename... Args>
            void RegisterInstanceLatent(std::string_view name, Return(*func)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, Instance*, Args...), bool callableFromTasklets = false) {
                auto wrapper = reinterpret_cast<Return(*)(RE::BSScript::Internal::VirtualMachine*, RE::VMStackID, Instance*, Args...)>(func);
                vm_->RegisterLatentFunction(name, className_, wrapper, callableFromTasklets);
            }
        };

        // Provider base class with template magic
        template<typename Derived>
        class PapyrusFunctionProvider : public BasePapyrusFunctionProvider {
        public:
            void RegisterFunctions(RE::BSScript::Internal::VirtualMachine* vm, std::string_view className) override {
                static_cast<Derived*>(this)->RegisterAllFunctions(vm, className);
            }
        };

        // Utility function to get stack (keeping from original implementation)
        [[nodiscard]] static RE::BSTSmartPointer<RE::BSScript::Stack> GetStack(RE::VMStackID stackId) noexcept {
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                return nullptr;
            }
            
            RE::BSScript::Stack* rawStack = nullptr;
            if (!vm->GetStackByID(stackId, &rawStack) || !rawStack) {
                return nullptr;
            }
            
            return RE::BSTSmartPointer<RE::BSScript::Stack>(rawStack);
        }
    }
}