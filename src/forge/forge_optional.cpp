
namespace SLT {


#pragma region Optional
bool Optional::Serialize(SKSE::SerializationInterface* a_intfc) const {
    if (!ForgeObject::Serialize(a_intfc)) return false;
    
    using SH = SerializationHelper;

    // Write the variant index to know which type is active
    std::size_t variantIndex = _value.index();
    if (!SH::WriteData(a_intfc, variantIndex)) return false;
    
    // Write the actual value based on type
    switch (variantIndex) {
        case 0: // std::monostate - nothing to write
            break;
            
        case 1: // bool
            if (!SH::WriteData(a_intfc, std::get<bool>(_value))) return false;
            break;
            
        case 2: // std::int32_t
            if (!SH::WriteData(a_intfc, std::get<std::int32_t>(_value))) return false;
            break;
            
        case 3: // float
            if (!SH::WriteData(a_intfc, std::get<float>(_value))) return false;
            break;
            
        case 4: // std::string
            if (!SH::WriteString(a_intfc, std::get<std::string>(_value))) return false;
            break;
            
        case 5: // RE::TESForm*
        {
            auto* form = std::get<RE::TESForm*>(_value);
            RE::FormID formID = form ? form->GetFormID() : 0;
            if (!SH::WriteData(a_intfc, formID)) return false;
            break;
        }
        
        case 6: // RE::ActiveEffect*
        {
            // ActiveEffect doesn't have a stable FormID, so we can't serialize it
            // Write 0 to indicate null on load
            RE::FormID nullFormID = 0;
            if (!SH::WriteData(a_intfc, nullFormID)) return false;
            break;
        }
        
        case 7: // RE::BGSBaseAlias*
        {
            // Aliases are complex to serialize properly, write 0 for now
            RE::FormID nullFormID = 0;
            if (!SH::WriteData(a_intfc, nullFormID)) return false;
            break;
        }
        
        default:
            logger::error("Optional::Serialize: Unknown variant index {}", variantIndex);
            return false;
    }
    
    return true;
}

bool Optional::Deserialize(SKSE::SerializationInterface* a_intfc) {
    if (!ForgeObject::Deserialize(a_intfc)) return false;
    
    using SH = SerializationHelper;

    // Read the variant index
    std::size_t variantIndex;
    if (!SH::ReadData(a_intfc, variantIndex)) return false;
    
    // Read the value based on type
    switch (variantIndex) {
        case 0: // std::monostate
            _value = std::monostate{};
            break;
            
        case 1: // bool
        {
            bool value;
            if (!SH::ReadData(a_intfc, value)) return false;
            _value = value;
            break;
        }
        
        case 2: // std::int32_t
        {
            std::int32_t value;
            if (!SH::ReadData(a_intfc, value)) return false;
            _value = value;
            break;
        }
        
        case 3: // float
        {
            float value;
            if (!SH::ReadData(a_intfc, value)) return false;
            _value = value;
            break;
        }
        
        case 4: // std::string
        {
            std::string value;
            if (!SH::ReadString(a_intfc, value)) return false;
            _value = value;
            break;
        }
        
        case 5: // RE::TESForm*
        {
            RE::FormID savedFormID;
            if (!SH::ReadData(a_intfc, savedFormID)) return false;
            
            if (savedFormID == 0) {
                _value = static_cast<RE::TESForm*>(nullptr);
            } else {
                RE::FormID resolvedFormID;
                if (a_intfc->ResolveFormID(savedFormID, resolvedFormID)) {
                    auto* form = RE::TESForm::LookupByID(resolvedFormID);
                    _value = form;
                } else {
                    // Form no longer exists
                    _value = static_cast<RE::TESForm*>(nullptr);
                }
            }
            break;
        }
        
        case 6: // RE::ActiveEffect*
        {
            RE::FormID nullFormID;
            if (!SH::ReadData(a_intfc, nullFormID)) return false;
            // Always null since we can't serialize ActiveEffect properly
            _value = static_cast<RE::ActiveEffect*>(nullptr);
            break;
        }
        
        case 7: // RE::BGSBaseAlias*
        {
            RE::FormID nullFormID;
            if (!SH::ReadData(a_intfc, nullFormID)) return false;
            // Always null since we can't serialize BGSBaseAlias properly
            _value = static_cast<RE::BGSBaseAlias*>(nullptr);
            break;
        }
        
        default:
            logger::error("Optional::Deserialize: Unknown variant index {}", variantIndex);
            return false;
    }
    
    return true;
}

#pragma endregion


}