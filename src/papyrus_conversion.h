#pragma once

namespace SLT {

/**
 * PapyrusConverter - Template utilities for converting between SKSE forms and Papyrus objects
 */
class PapyrusConverter {
public:
    /**
     * Convert an SKSE form to a Papyrus script object
     * @tparam T - The SKSE form type (must have T::FORMTYPE)
     * @param form - The SKSE form to convert
     * @param vm - Virtual machine instance (optional, will get singleton if null)
     * @returns BSTSmartPointer to the Papyrus object, or nullptr on failure
     */
    template<typename T>
    static RE::BSTSmartPointer<RE::BSScript::Object> ToPapyrusObject(T* form, 
                                                                     RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        static_assert(std::is_base_of_v<RE::TESForm, T>, "T must derive from TESForm");
        static_assert(requires { T::FORMTYPE; }, "T must have FORMTYPE constant");
        
        if (!form) {
            return nullptr;
        }
        
        if (!vm) {
            vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                logger::error("Failed to get VM singleton");
                return nullptr;
            }
        }
        
        auto* handlePolicy = vm->GetObjectHandlePolicy();
        auto* bindPolicy = vm->GetObjectBindPolicy();
        
        if (!handlePolicy || !bindPolicy) {
            logger::error("Failed to get VM policies");
            return nullptr;
        }
        
        // Get handle for the form
        auto vmHandle = handlePolicy->GetHandleForObject(T::FORMTYPE, form);
        if (!vmHandle) {
            logger::warn("Failed to get VM handle for form 0x{:X}", form->GetFormID());
            return nullptr;
        }
        
        // Bind to Papyrus object
        RE::BSTSmartPointer<RE::BSScript::Object> papyrusObject;
        bindPolicy->BindObject(papyrusObject, vmHandle);
        
        if (!papyrusObject) {
            logger::warn("Failed to bind form 0x{:X} to Papyrus object", form->GetFormID());
        }
        
        return papyrusObject;
    }
    
    /**
     * Convert a Papyrus script object back to an SKSE form
     * @tparam T - The expected SKSE form type
     * @param papyrusObject - The Papyrus object to convert
     * @param vm - Virtual machine instance (optional, will get singleton if null)
     * @returns Pointer to the SKSE form, or nullptr on failure
     */
    template<typename T>
    static T* FromPapyrusObject(RE::BSTSmartPointer<RE::BSScript::Object> papyrusObject,
                               RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        static_assert(std::is_base_of_v<RE::TESForm, T>, "T must derive from TESForm");
        static_assert(requires { T::FORMTYPE; }, "T must have FORMTYPE constant");
        
        if (!papyrusObject) {
            return nullptr;
        }
        
        if (!vm) {
            vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                logger::error("Failed to get VM singleton");
                return nullptr;
            }
        }
        
        auto* handlePolicy = vm->GetObjectHandlePolicy();
        if (!handlePolicy) {
            logger::error("Failed to get handle policy");
            return nullptr;
        }
        
        // Get the VM handle from the object
        auto vmHandle = papyrusObject->GetHandle();
        if (!vmHandle) {
            logger::warn("Papyrus object has no VM handle");
            return nullptr;
        }
        
        // Check if the handle is available and get the underlying form
        if (!handlePolicy->IsHandleObjectAvailable(vmHandle)) {
            logger::warn("VM handle is not available");
            return nullptr;
        }
        
        // Get the form object
        auto* formObject = handlePolicy->GetObjectForHandle(T::FORMTYPE, vmHandle);
        return static_cast<T*>(formObject);
    }
    
    /**
     * Generic form-to-Papyrus conversion using FormType detection
     * Useful when you don't know the exact type at compile time
     */
    static RE::BSTSmartPointer<RE::BSScript::Object> ToPapyrusObject(RE::TESForm* form,
                                                                     RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        if (!form) {
            return nullptr;
        }
        
        if (!vm) {
            vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                return nullptr;
            }
        }
        
        auto* handlePolicy = vm->GetObjectHandlePolicy();
        auto* bindPolicy = vm->GetObjectBindPolicy();
        
        if (!handlePolicy || !bindPolicy) {
            return nullptr;
        }
        
        // Use the form's actual type
        RE::FormType formType = form->GetFormType();
        auto vmHandle = handlePolicy->GetHandleForObject(formType, form);
        
        if (!vmHandle) {
            return nullptr;
        }
        
        RE::BSTSmartPointer<RE::BSScript::Object> papyrusObject;
        bindPolicy->BindObject(papyrusObject, vmHandle);
        
        return papyrusObject;
    }
    
    /**
     * Generic Papyrus-to-form conversion
     * Returns the underlying TESForm* without type casting
     */
    static RE::TESForm* FromPapyrusObject(RE::BSTSmartPointer<RE::BSScript::Object> papyrusObject,
                                         RE::FormType expectedType = RE::FormType::None,
                                         RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        if (!papyrusObject) {
            return nullptr;
        }
        
        if (!vm) {
            vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            if (!vm) {
                return nullptr;
            }
        }
        
        auto* handlePolicy = vm->GetObjectHandlePolicy();
        if (!handlePolicy) {
            return nullptr;
        }
        
        auto vmHandle = papyrusObject->GetHandle();
        if (!vmHandle || !handlePolicy->IsHandleObjectAvailable(vmHandle)) {
            return nullptr;
        }
        
        // If no specific type expected, try common form types
        if (expectedType == RE::FormType::None) {
            // Try to determine the type from the object's type info
            auto* typeInfo = papyrusObject->GetTypeInfo();
            if (typeInfo) {
                std::string typeName = typeInfo->GetName();
                
                // Map common Papyrus type names to FormTypes
                if (typeName == "Quest") expectedType = RE::FormType::Quest;
                else if (typeName == "Actor") expectedType = RE::FormType::ActorCharacter;
                else if (typeName == "ObjectReference") expectedType = RE::FormType::Reference;
                else if (typeName == "Spell") expectedType = RE::FormType::Spell;
                else if (typeName == "MagicEffect") expectedType = RE::FormType::MagicEffect;
                // Add more mappings as needed
            }
        }
        
        if (expectedType == RE::FormType::None) {
            logger::warn("Could not determine form type for Papyrus object");
            return nullptr;
        }
        
        return handlePolicy->GetObjectForHandle(expectedType, vmHandle);
    }
    
    /**
     * Convenience functions for common types
     */
    static RE::BSTSmartPointer<RE::BSScript::Object> QuestToPapyrus(RE::TESQuest* quest, 
                                                                   RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return ToPapyrusObject(quest, vm);
    }
    
    static RE::TESQuest* PapyrusToQuest(RE::BSTSmartPointer<RE::BSScript::Object> obj,
                                       RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return FromPapyrusObject<RE::TESQuest>(obj, vm);
    }
    
    static RE::BSTSmartPointer<RE::BSScript::Object> ActorToPapyrus(RE::Actor* actor,
                                                                   RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return ToPapyrusObject(actor, vm);
    }
    
    static RE::Actor* PapyrusToActor(RE::BSTSmartPointer<RE::BSScript::Object> obj,
                                    RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return FromPapyrusObject<RE::Actor>(obj, vm);
    }
    
    static RE::BSTSmartPointer<RE::BSScript::Object> SpellToPapyrus(RE::SpellItem* spell,
                                                                   RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return ToPapyrusObject(spell, vm);
    }
    
    static RE::SpellItem* PapyrusToSpell(RE::BSTSmartPointer<RE::BSScript::Object> obj,
                                        RE::BSScript::Internal::VirtualMachine* vm = nullptr) {
        return FromPapyrusObject<RE::SpellItem>(obj, vm);
    }
};

// Convenience macros for even cleaner usage
#define SKSE_TO_PAPYRUS(form) SLT::PapyrusConverter::ToPapyrusObject(form)
#define PAPYRUS_TO_SKSE(obj, type) SLT::PapyrusConverter::FromPapyrusObject<type>(obj)

} // namespace SLT