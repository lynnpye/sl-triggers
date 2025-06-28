
//=================================================================================================
// SystemUtil::File implementations
//=================================================================================================

void SystemUtil::File::PrintPathProblem(fs::path& folderPath, std::string subfolder, std::vector<std::string> parts) {
    if (subfolder.empty()) {
        logger::error("PrintPathProblem: subfolder is empty or null");
        return;
    }

    fs::path checking = fs::path(subfolder);

    if (fs::exists(checking)) {
        auto pit = parts.begin();
        while (fs::exists(checking) && pit != parts.end()) {
            checking /= *pit;
            pit++;
        }
    }

    if (!fs::exists(checking)) {
        logger::error("Looking for extensions path '{}' but was not able to find '{}'", folderPath.string(), checking.string());
    } else {
        logger::warn("A problem was reported for path starting '{}' with subfolder '{}' and parts '{}', but all folders exist on check",
            folderPath.string(), subfolder, Util::String::Join(parts, ", ")
        );
    }
}

std::string_view SystemUtil::File::GetPluginName() {
    auto decl = SKSE::PluginDeclaration::GetSingleton();
    return decl->GetName();
}

std::vector<std::string> SystemUtil::File::GetConfigs(std::string_view a_folder, std::string_view a_suffix, std::string_view a_extension) {
    std::vector<std::string> configs{};

    for (const auto iterator = fs::directory_iterator(a_folder); const auto& entry : iterator) {
        if (entry.exists()) {
            if (const auto& path = entry.path(); !path.empty() && path.extension() == a_extension) {
                if (const auto& fileName = entry.path().string(); fileName.rfind(a_suffix) != std::string::npos) {
                    configs.push_back(fileName);
                }
            }
        }
    }

    std::ranges::sort(configs);

    return configs;
}

bool SystemUtil::File::IsValidPathComponent(std::string_view input) {
    // Disallow characters illegal in Windows filenames
    static const std::regex validPattern(R"(^[^<>:"/\\|?*\x00-\x1F]+$)");
    return std::regex_match(input.begin(), input.end(), validPattern);
}

//=================================================================================================
// KeyUtil::Interpreter implementations
//=================================================================================================

uint32_t KeyUtil::Interpreter::GamepadMaskToKeycode(uint32_t keyMask) {
    switch (keyMask) {
        case 0x001:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_DPAD_UP);
        case 0x002:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_DPAD_DOWN);
        case 0x004:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_DPAD_LEFT);
        case 0x008:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_DPAD_RIGHT);
        case 0x0010:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_START);
        case 0x0020:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_BACK);
        case 0x0040:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_LEFT_THUMB);
        case 0x0080:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_RIGHT_THUMB);
        case 0x0100:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_LEFT_SHOULDER);
        case 0x0200:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_RIGHT_SHOULDER);
        case 0x1000:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_A);
        case 0x2000:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_B);
        case 0x4000:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_X);
        case 0x8000:	return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_Y);
        case 0x9:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_LT);
        case 0xA:		return static_cast<int>(GAMEPAD_OFFSETS::kGamepadButtonOffset_RT);
        default:		return 282; // Invalid
    }
}

//=================================================================================================
// Util::String implementations
//=================================================================================================

std::vector<std::string> Util::String::Split(std::string_view a_str, std::string_view a_delimiter) {
    auto range = a_str | std::ranges::views::split(a_delimiter) | std::ranges::views::transform([](auto&& r) { return std::string_view(r); });
    return { range.begin(), range.end() };
}


std::string Util::String::truncateAt(const std::string& str, char c) {
    size_t pos = str.find(c);
    if (pos != std::string::npos) {
        return str.substr(0, pos);
    }
    return str;  // char not found, return original string
}

std::string Util::String::ltrim(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(), 
                                [](unsigned char ch) { return std::isspace(ch); });
    return std::string(start, str.end());
}

std::string Util::String::rtrim(std::string_view str) {
    auto end = std::find_if_not(str.rbegin(), str.rend(),
                            [](unsigned char ch) { return std::isspace(ch); }).base();
    return std::string(str.begin(), end);
}

std::string Util::String::trim(std::string_view str) {
    auto start = std::find_if_not(str.begin(), str.end(),
                                [](unsigned char ch) { return std::isspace(ch); });
    if (start == str.end()) {
        return std::string{}; // All whitespace
    }
    
    auto end = std::find_if_not(str.rbegin(), str.rend(),
                            [](unsigned char ch) { return std::isspace(ch); }).base();
    return std::string(start, end);
}

void Util::String::ltrim_inplace(std::string& str) {
    str.erase(str.begin(), 
            std::find_if_not(str.begin(), str.end(),
                            [](unsigned char ch) { return std::isspace(ch); }));
}

void Util::String::rtrim_inplace(std::string& str) {
    str.erase(std::find_if_not(str.rbegin(), str.rend(),
                            [](unsigned char ch) { return std::isspace(ch); }).base(),
            str.end());
}

void Util::String::trim_inplace(std::string& str) {
    ltrim_inplace(str);
    rtrim_inplace(str);
}

std::optional<std::int32_t> Util::String::StringToIntWithImplicitHexConversion(std::string_view _hexStr) {
    std::string hexStr = trim(_hexStr);
    if (hexStr.empty()) {
        return 0;
    }
    const char* start = hexStr.data();
    const char* end = hexStr.data() + hexStr.size();
    int base = 10;
    
    if (hexStr.size() >= 2 && str::iEquals(hexStr.substr(0, 2), "0x")) {
        start += 2;
        base = 16;
    }
    
    std::int32_t result;
    auto [ptr, ec] = std::from_chars(start, end, result, base);
    
    if (ec == std::errc{} && ptr == end) {
        return result;
    }
    return std::nullopt;
}

std::optional<std::uint32_t> Util::String::StringToUnsignedIntWithImplicitHexConversion(std::string_view _hexStr) {
    std::string hexStr = trim(_hexStr);
    if (hexStr.empty()) {
        return 0;
    }
    const char* start = hexStr.data();
    const char* end = hexStr.data() + hexStr.size();
    int base = 10;
    
    if (hexStr.size() >= 2 && str::iEquals(hexStr.substr(0, 2), "0x")) {
        start += 2;
        base = 16;
    }
    
    std::uint32_t result;
    auto [ptr, ec] = std::from_chars(start, end, result, base);
    
    if (ec == std::errc{} && ptr == end) {
        return result;
    }
    return std::nullopt;
}

bool Util::String::iContains(std::string_view a_str1, std::string_view a_str2) {
    if (a_str2.length() > a_str1.length()) {
        return false;
    }

    const auto subrange = std::ranges::search(a_str1, a_str2, [](unsigned char ch1, unsigned char ch2) {
        return std::toupper(ch1) == std::toupper(ch2);
    });

    return !subrange.empty();
}

bool Util::String::iEquals(std::string_view a_str1, std::string_view a_str2) {
    return std::ranges::equal(a_str1, a_str2, [](unsigned char ch1, unsigned char ch2) {
        return std::toupper(ch1) == std::toupper(ch2);
    });
}

bool Util::String::isTrue(std::string_view a_str) {
    return iEquals("true", a_str);
}

bool Util::String::isFalse(std::string_view a_str) {
    return iEquals("false", a_str);
}

bool Util::String::toBool(std::string_view a_str) {
    return iEquals("true", a_str);
}

std::string Util::String::Join(const std::vector<std::string>& a_vec, std::string_view a_delimiter) {
    return std::accumulate(a_vec.begin(), a_vec.end(), std::string{},
        [a_delimiter](const auto& str1, const auto& str2) {
            return str1.empty() ? str2 : str1 + a_delimiter.data() + str2;
        });
}

std::vector<float> Util::String::ToFloatVector(const std::vector<std::string> stringVector) {
    std::vector<float> floatNumbers; 
    for(auto str : stringVector) {
        float num = atof(str.c_str());
        floatNumbers.push_back(num);
    }
    return floatNumbers;
}

std::string Util::String::ToLower(std::string_view a_str) {
    std::string result(a_str);
    std::ranges::transform(result, result.begin(), [](unsigned char ch) { return static_cast<unsigned char>(std::tolower(ch)); });
    return result;
}

std::string Util::String::ToUpper(std::string_view a_str) {
    std::string result(a_str);
    std::ranges::transform(result, result.begin(), [](unsigned char ch) { return static_cast<unsigned char>(std::toupper(ch)); });
    return result;
}

float Util::String::TryToFloat(std::string_view strval) {
    float value;
    auto [ptr, ec] = std::from_chars(strval.data(), strval.data() + strval.size(), value);
    if (ec == std::errc{} && ptr == strval.data() + strval.size()) {
        return value;
    }
    return 0.0f;
}

//=================================================================================================
// MathUtil::Angle implementations
//=================================================================================================

NiPoint3 MathUtil::Angle::ToRadianVector(float x, float y, float z) {
    RE::NiPoint3 rotationVector{ 0.f, 0.f, 0.f };

    rotationVector.x = DegreeToRadian(x); 
    rotationVector.y = DegreeToRadian(y); 
    rotationVector.z = DegreeToRadian(z); 
    return rotationVector; 
}

float MathUtil::Angle::NormalAbsoluteAngle(float a_angle) {
    while (a_angle < 0)
        a_angle += TWO_PI;
    while (a_angle > TWO_PI)
        a_angle -= TWO_PI;
    return a_angle;

    // return fmod(a_angle, TWO_PI) >= 0 ? a_angle : (a_angle + TWO_PI);
}

float MathUtil::Angle::NormalRelativeAngle(float a_angle) {
    while (a_angle > PI)
        a_angle -= TWO_PI;
    while (a_angle < -PI)
        a_angle += TWO_PI;
    return a_angle;

    // return fmod(a_angle, TWO_PI) >= 0 ? (a_angle < PI) ? a_angle : a_angle - TWO_PI : (a_angle >= -PI) ? a_angle : a_angle + TWO_PI;
}

float MathUtil::Angle::GetAngle(RE::NiPoint2 &a, RE::NiPoint2 &b) {
    return atan2(a.Cross(b), a.Dot(b));
}

void MathUtil::Angle::GetAngle(const RE::NiPoint3 &a_from, const RE::NiPoint3 &a_to, AngleZX &angle) {
    const auto x = a_to.x - a_from.x;
    const auto y = a_to.y - a_from.y;
    const auto z = a_to.z - a_from.z;
    const auto xy = sqrt(x * x + y * y);

    angle.z = atan2(x, y);
    angle.x = atan2(-z, xy);
    angle.distance = sqrt(xy * xy + z * z);
}

RE::NiPoint3 MathUtil::Angle::RotateVector(const RE::NiPoint3& a_vec, const RE::NiQuaternion& a_quat) {
    //http://people.csail.mit.edu/bkph/articles/Quaternions.pdf
    const RE::NiPoint3 Q{ a_quat.x, a_quat.y, a_quat.z };
    const RE::NiPoint3 T = Q.Cross(a_vec) * 2.f;
    return a_vec + (T * a_quat.w) + Q.Cross(T);
}

RE::NiPoint3 MathUtil::Angle::GetForwardVector(const RE::NiQuaternion& a_quat) {
    return RotateVector({ 0.f, 1.f, 0.f }, a_quat);
}

NiMatrix3 MathUtil::Angle::QuaternionToMatrix(const NiQuaternion& a_quat) {
    float sqw = a_quat.w * a_quat.w;
    float sqx = a_quat.x * a_quat.x;
    float sqy = a_quat.y * a_quat.y;
    float sqz = a_quat.z * a_quat.z;

    NiMatrix3 ret;

    // invs (inverse square length) is only required if quaternion is not already normalised
    float invs = 1.f / (sqx + sqy + sqz + sqw);
    ret.entry[0][0] = (sqx - sqy - sqz + sqw) * invs;  // since sqw + sqx + sqy + sqz =1/invs*invs
    ret.entry[1][1] = (-sqx + sqy - sqz + sqw) * invs;
    ret.entry[2][2] = (-sqx - sqy + sqz + sqw) * invs;

    float tmp1 = a_quat.x * a_quat.y;
    float tmp2 = a_quat.z * a_quat.w;
    ret.entry[1][0] = 2.f * (tmp1 + tmp2) * invs;
    ret.entry[0][1] = 2.f * (tmp1 - tmp2) * invs;

    tmp1 = a_quat.x * a_quat.z;
    tmp2 = a_quat.y * a_quat.w;
    ret.entry[2][0] = 2.f * (tmp1 - tmp2) * invs;
    ret.entry[0][2] = 2.f * (tmp1 + tmp2) * invs;
    tmp1 = a_quat.y * a_quat.z;
    tmp2 = a_quat.x * a_quat.w;
    ret.entry[2][1] = 2.f * (tmp1 + tmp2) * invs;
    ret.entry[1][2] = 2.f * (tmp1 - tmp2) * invs;

    return ret;
}

//=================================================================================================
// MathUtil::Interp implementations
//=================================================================================================

float MathUtil::Interp::InterpTo(float a_current, float a_target, float a_deltaTime, float a_interpSpeed) {
    if (a_interpSpeed <= 0.f) {
        return a_target;
    }

    const float distance = a_target - a_current;

    if (distance * distance < FLT_EPSILON) {
        return a_target;
    }

    const float delta = distance * Clamp(a_deltaTime * a_interpSpeed, 0.f, 1.f);

    return a_current + delta;
}

//=================================================================================================
// ObjectUtil::Transform implementations
//=================================================================================================

void ObjectUtil::Transform::TranslateTo(RE::BSScript::IVirtualMachine *vm, RE::VMStackID stackID, RE::TESObjectREFR *object, float afX, float afY, float afZ, float afAngleX, float afAngleY, float afAngleZ, float afSpeed, float afMaxRotationSpeed) {
    using func_t = decltype(TranslateTo);
    REL::Relocation<func_t> func{RELOCATION_ID(55706, 56237)};
    func(vm, stackID, object, afX, afY, afZ, afAngleX, afAngleY, afAngleZ, afSpeed, afMaxRotationSpeed);
}

float ObjectUtil::Transform::InterpAngleTo(float a_current, float a_target, float a_deltaTime, float a_interpSpeed) {
    if (a_interpSpeed <= 0.f) {
        return a_target;
    }

    const float distance = MathUtil::Angle::NormalRelativeAngle(a_target - a_current);

    if (distance * distance < FLT_EPSILON) {
        return a_target;
    }

    const float delta = distance * Clamp(a_deltaTime * a_interpSpeed, 0.f, 1.f);

    return a_current + delta;
}

float ObjectUtil::Transform::Clamp(float value, float min, float max) {
    return value < min ? min : value < max ? value : max;
}

//=================================================================================================
// AnimUtil::Idle implementations
//=================================================================================================

bool AnimUtil::Idle::Play(RE::TESIdleForm* idle, RE::Actor* actor, RE::DEFAULT_OBJECT action, RE::Actor* target) {
    if (actor && actor->GetActorRuntimeData().currentProcess) {
        typedef bool (*func_t)(RE::AIProcess *, RE::Actor *, RE::DEFAULT_OBJECT, RE::TESIdleForm *, bool, bool, RE::Actor *);
        REL::Relocation<func_t> func{RELOCATION_ID(38290, 39256)};
        return func(actor->GetActorRuntimeData().currentProcess, actor, action, idle, true, true, target);
    }
    return false;
}

//=================================================================================================
// FormUtil::Parse implementations
//=================================================================================================

RE::TESForm *FormUtil::Parse::GetFormFromMod(uint32_t formid, std::string modname) {
    if (!modname.length() || !formid)
        return nullptr;
    RE::TESDataHandler *dh = RE::TESDataHandler::GetSingleton();
    return dh->LookupForm(formid, modname);
}

RE::TESForm *FormUtil::Parse::GetFormFromMod(std::string modname, std::string formIDString) {
    if (formIDString.length() == 0) return nullptr; 

    uint32_t formID = std::stoi(formIDString, 0, 16); 
    return GetFormFromMod(formID, modname); 
} 

RE::TESForm *FormUtil::Parse::GetFormFromConfigString(std::string str, std::string_view delimiter) {
    std::vector<std::string> splitData = Util::String::Split(str, delimiter); 
    if (splitData.size() < 2) return nullptr;  
    return GetFormFromMod(splitData[0], splitData[1]);
}

RE::TESForm *FormUtil::Parse::GetFormFromConfigString(std::string str) {
    return GetFormFromConfigString(str, ":"sv); 
}

RE::FormID FormUtil::Parse::GetFormIDFromMod(uint32_t relativeFormID, std::string modName) {
    auto *dataHandler = TESDataHandler::GetSingleton();

    if (!dataHandler)
        return -1;

    return dataHandler->LookupFormID(relativeFormID, modName);
}

RE::FormID FormUtil::Parse::GetFormIDFromMod(std::string relativeFormIDString, std::string modName) {
    if (relativeFormIDString.length() == 0) return -1; 

    uint32_t relativeFormID = std::stoi(relativeFormIDString,  0, 16); 
    return GetFormIDFromMod(relativeFormID, modName); 
}

RE::FormID FormUtil::Parse::GetFormIDFromConfigString(std::string str, std::string_view delimiter) {
    std::vector<std::string> splitData = Util::String::Split(str, delimiter); 
    if (splitData.size() < 2) return -1; 
    return GetFormIDFromMod(splitData[0], splitData[1]);
}

RE::FormID FormUtil::Parse::GetFormIDFromConfigString(std::string str) {
    return GetFormIDFromConfigString(str, ":"sv); 
}

RE::TESForm* FormUtil::Parse::GetForm(std::string_view data) {
    RE::TESForm* retVal = nullptr;
    
    if (data.empty()) {
        return nullptr;
    }
    
    std::vector<std::string> params = Util::String::Split(data, ":");
    
    if (params.size() == 2) {
        const std::string& modfile = params[0];
        const std::string& sid = params[1];
        
        if (!modfile.empty() && !sid.empty()) {
            auto idOpt = Util::String::StringToUnsignedIntWithImplicitHexConversion(sid);
            
            if (idOpt.has_value()) {
                RE::FormID id = static_cast<RE::FormID>(idOpt.value());
                
                auto* dataHandler = RE::TESDataHandler::GetSingleton();
                if (dataHandler) {
                    retVal = dataHandler->LookupForm(id, modfile);
                }
            }
        }
    } 
    else {
        params = Util::String::Split(data, "|");
        if (params.size() == 2) {
            const std::string& sid = params[0];
            const std::string& modfile = params[1];
            
            if (!modfile.empty() && !sid.empty()) {
                auto idOpt = Util::String::StringToUnsignedIntWithImplicitHexConversion(sid);
                
                if (idOpt.has_value()) {
                    RE::FormID id = static_cast<RE::FormID>(idOpt.value());
                    
                    auto* dataHandler = RE::TESDataHandler::GetSingleton();
                    if (dataHandler) {
                        retVal = dataHandler->LookupForm(id, modfile);
                    }
                }
            }
        } 
        else if (params.size() == 1) {
            auto idOpt = Util::String::StringToUnsignedIntWithImplicitHexConversion(data);
            
            if (idOpt.has_value() && idOpt.value() != 0) {
                RE::FormID id = static_cast<RE::FormID>(idOpt.value());
                retVal = RE::TESForm::LookupByID(id);
            } else {
                retVal = RE::TESForm::LookupByEditorID(data);
            }
        }
    }
    
    if (!retVal) {
        logger::error("Form not found ({})", data);
    }
    
    return retVal;
}

//=================================================================================================
// FormUtil::Quest implementations
//=================================================================================================

BGSBaseAlias *FormUtil::Quest::FindAliasByName(std::string_view name, TESQuest *owningQuest) {
    RE::BSWriteLockGuard AliasLock{owningQuest->aliasAccessLock};
    for (auto *alias : owningQuest->aliases) {
        std::string aliasName = alias->aliasName.c_str();
        if (aliasName == name)
            return alias;
    }
    return nullptr;
}

//=================================================================================================
// NifUtil::Node implementations
//=================================================================================================

NiAVObject* NifUtil::Node::Clone(NiAVObject* original) {
    typedef NiAVObject* (*func_t)(NiAVObject* avObj);
    REL::Relocation<func_t> func{ RELOCATION_ID(68835, 70187) };
    return func(original);
}

RE::NiAVObject* NifUtil::Node::GetNiObject(RE::NiNode* a_root, const RE::BSFixedString& a_name) {
    return a_root->GetObjectByName(a_name);
}

void NifUtil::Node::AttachToNode(RE::NiAVObject* a_object, RE::NiNode* a_node) {
    if (a_object->parent != a_node) {
        a_node->AttachChild(a_object, true);
    }
}

std::vector<BSGeometry*> NifUtil::Node::GetAllGeometries(RE::NiAVObject* root) {
    std::vector<BSGeometry*> geometries; 
    RE::BSVisit::TraverseScenegraphGeometries(root, [&](BSGeometry* geom)-> RE::BSVisit::BSVisitControl {
        geometries.emplace_back(geom); 
        return RE::BSVisit::BSVisitControl::kContinue;
    }); 
    return geometries;
}

//=================================================================================================
// NifUtil::Armature implementations
//=================================================================================================

RE::NiNode* NifUtil::Armature::GetActorNode(RE::Actor* actor, std::string nodeName) {
    auto root = actor->Get3D();
    if (!root) return nullptr;

    auto bone = root->GetObjectByName(nodeName);
    if (!bone) return nullptr;

    auto node = bone->AsNode();
    if (!node) return nullptr;

    return node;
}

void NifUtil::Armature::AttachToNode(RE::NiAVObject* obj, RE::Actor* actor, std::string nodeName) {
    auto* node = GetActorNode(actor, nodeName);
    if (node) {
        node->AttachChild(obj, true);
        SKSE::log::info("Object Attached");
    }
}

//=================================================================================================
// NifUtil::Collision implementations
//=================================================================================================

bool NifUtil::Collision::ToggleMeshCollision(RE::NiAVObject* root, RE::bhkWorld* world, bool collisionState) {
    constexpr auto no_collision_flag = static_cast<std::uint32_t>(RE::CFilter::Flag::kNoCollision);
    if (root && world) {
        RE::BSWriteLockGuard locker(world->worldLock);

        RE::BSVisit::TraverseScenegraphCollision(root, [&](RE::bhkNiCollisionObject* a_col) -> RE::BSVisit::BSVisitControl {
            if (auto hkpBody = a_col->body ? static_cast<RE::hkpWorldObject*>(a_col->body->referencedObject.get()) : nullptr; hkpBody) {
                auto& filter = hkpBody->collidable.broadPhaseHandle.collisionFilterInfo;
                if (!collisionState) {
                    filter |= no_collision_flag;
                } else {
                    filter &= ~no_collision_flag;
                }
            }
            return RE::BSVisit::BSVisitControl::kContinue;
        });
    } else {
        return false;
    }
    return true;
}

bool NifUtil::Collision::RemoveMeshCollision(RE::NiAVObject* root, RE::bhkWorld* world, bool collisionState) {
    constexpr auto no_collision_flag = static_cast<std::uint32_t>(RE::CFilter::Flag::kNoCollision);
    if (root && world) {
        RE::BSWriteLockGuard locker(world->worldLock);

        RE::BSVisit::TraverseScenegraphCollision(root, [&](RE::bhkNiCollisionObject* a_col) -> RE::BSVisit::BSVisitControl {
            if (auto hkpBody = a_col->body ? static_cast<RE::hkpWorldObject*>(a_col->body->referencedObject.get()) : nullptr; hkpBody) {
                auto& filter = hkpBody->collidable.broadPhaseHandle.collisionFilterInfo;
                if (!collisionState) {
                    filter |= no_collision_flag;
                } else {
                    filter &= ~no_collision_flag;
                }
            }
            return RE::BSVisit::BSVisitControl::kContinue;
        });
    } else {
        return false;
    }
    return true;
}