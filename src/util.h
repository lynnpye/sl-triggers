#pragma once

#define PI 3.1415926535897932f
#define TWOTHIRDS_PI 2.0943951023931955f
#define TWO_PI 6.2831853071795865f
#define PI2 1.5707963267948966f
#define PI3 1.0471975511965977f
#define PI4 0.7853981633974483f
#define PI8 0.3926990816987242f

using namespace RE;

//=================================================================================================
// FORWARD DECLARATIONS
//=================================================================================================
namespace SystemUtil { struct File; }
namespace KeyUtil { struct Interpreter; }
namespace Util { struct String; }
namespace MathUtil { struct Angle; struct Interp; }
namespace ObjectUtil { struct Transform; }
namespace AnimUtil { struct Idle; }
namespace FormUtil { struct Parse; struct Quest; }
namespace NifUtil { struct Node; struct Armature; struct Collision; }

//=================================================================================================
// CLASS DECLARATIONS
//=================================================================================================

namespace PointerUtil //yoinked po3's code
{
template <class T, class U>
inline auto adjust_pointer(U* a_ptr, std::ptrdiff_t a_adjust) noexcept
{
	auto addr = a_ptr ? reinterpret_cast<std::uintptr_t>(a_ptr) + a_adjust : 0;
	if constexpr (std::is_const_v<U> && std::is_volatile_v<U>) {
		return reinterpret_cast<std::add_cv_t<T>*>(addr);
	} else if constexpr (std::is_const_v<U>) {
		return reinterpret_cast<std::add_const_t<T>*>(addr);
	} else if constexpr (std::is_volatile_v<U>) {
		return reinterpret_cast<std::add_volatile_t<T>*>(addr);
	} else {
		return reinterpret_cast<T*>(addr);
	}
}
}

namespace SystemUtil
{
    struct File 
    {
        static void PrintPathProblem(fs::path& folderPath, std::string subfolder, std::vector<std::string> parts);
        static std::string_view GetPluginName();
        static std::vector<std::string> GetConfigs(std::string_view a_folder, std::string_view a_suffix, std::string_view a_extension = ".ini"sv);
        static bool IsValidPathComponent(std::string_view input);
    };
}

namespace KeyUtil 
{
    enum class MACRO_LIMITS {
        kMaxMacros = 282
    };

    enum class KBM_OFFSETS {
		// first 256 for keyboard, then 8 mouse buttons, then mouse wheel up, wheel down, then 16 gamepad buttons
		kMacro_KeyboardOffset = 0,		// not actually used, just for self-documentation
		kMacro_NumKeyboardKeys = 256,

		kMacro_MouseButtonOffset = kMacro_NumKeyboardKeys,	// 256
		kMacro_NumMouseButtons = 8,

		kMacro_MouseWheelOffset = kMacro_MouseButtonOffset + kMacro_NumMouseButtons,	// 264
		kMacro_MouseWheelDirections = 2,

		kMacro_GamepadOffset = kMacro_MouseWheelOffset + kMacro_MouseWheelDirections,	// 266
		kMacro_NumGamepadButtons = 16,

			// 282
	};

	enum class GAMEPAD_OFFSETS {
		kGamepadButtonOffset_DPAD_UP = static_cast<int>(KBM_OFFSETS::kMacro_GamepadOffset),	// 266
		kGamepadButtonOffset_DPAD_DOWN,
		kGamepadButtonOffset_DPAD_LEFT,
		kGamepadButtonOffset_DPAD_RIGHT,
		kGamepadButtonOffset_START,
		kGamepadButtonOffset_BACK,
		kGamepadButtonOffset_LEFT_THUMB,
		kGamepadButtonOffset_RIGHT_THUMB,
		kGamepadButtonOffset_LEFT_SHOULDER,
		kGamepadButtonOffset_RIGHT_SHOULDER,
		kGamepadButtonOffset_A,
		kGamepadButtonOffset_B,
		kGamepadButtonOffset_X,
		kGamepadButtonOffset_Y,
		kGamepadButtonOffset_LT,
		kGamepadButtonOffset_RT	// 281
	};

    struct Interpreter
    {
        static uint32_t GamepadMaskToKeycode(uint32_t keyMask);
    };
}

namespace Util
{
    struct String
    {
		static std::vector<std::string> Split(std::string_view a_str, std::string_view a_delimiter);
        static std::string truncateAt(const std::string& str, char c);
        static std::string ltrim(std::string_view str);
        static std::string rtrim(std::string_view str);
        static std::string trim(std::string_view str);
        static void ltrim_inplace(std::string& str);
        static void rtrim_inplace(std::string& str);
        static void trim_inplace(std::string& str);
        static std::optional<std::int32_t> StringToIntWithImplicitHexConversion(std::string_view _hexStr);
        static std::optional<std::uint32_t> StringToUnsignedIntWithImplicitHexConversion(std::string_view _hexStr);
        static bool iContains(std::string_view a_str1, std::string_view a_str2);
		static bool iEquals(std::string_view a_str1, std::string_view a_str2);
        static bool isTrue(std::string_view a_str);
        static bool isFalse(std::string_view a_str);
        static bool toBool(std::string_view a_str);
		static std::string Join(const std::vector<std::string>& a_vec, std::string_view a_delimiter);
        static std::vector<float> ToFloatVector(const std::vector<std::string> stringVector);
        static std::string ToLower(std::string_view a_str);
		static std::string ToUpper(std::string_view a_str);
        static float TryToFloat(std::string_view strval);

        template<typename T>
        static std::string ToHex(T value) {
            return std::format("0x{:X}", value);
        }
    };
}

typedef Util::String str;

namespace MathUtil
{
    [[nodiscard]] inline float Clamp(float value, float min, float max)
    {
        return value < min ? min : value < max ? value
                                               : max;
    }
    [[nodiscard]] inline RE::NiPoint3 GetNiPoint3(RE::hkVector4 a_hkVector4)
    {
        float quad[4];
        _mm_store_ps(quad, a_hkVector4.quad);
        return RE::NiPoint3{quad[0], quad[1], quad[2]};
    }
    [[nodiscard]] inline bool ApproximatelyEqual(float A, float B)
    {
        return ((A - B) < FLT_EPSILON) && ((B - A) < FLT_EPSILON);
    }

    struct Angle 
    {
        struct AngleZX
        {
            double z;
            double x;
            double distance;
        };

        [[nodiscard]] constexpr static float DegreeToRadian(float a_angle)
        {
            return a_angle * 0.017453292f;
        }

        [[nodiscard]] constexpr static float RadianToDegree(float a_radian)
        {
            return a_radian * 57.295779513f;
        }

        static NiPoint3 ToRadianVector(float x, float y, float z);
        static float NormalAbsoluteAngle(float a_angle);
        static float NormalRelativeAngle(float a_angle);
        static float GetAngle(RE::NiPoint2 &a, RE::NiPoint2 &b);
        static void GetAngle(const RE::NiPoint3 &a_from, const RE::NiPoint3 &a_to, AngleZX &angle);
        static RE::NiPoint3 RotateVector(const RE::NiPoint3& a_vec, const RE::NiQuaternion& a_quat);
        static RE::NiPoint3 GetForwardVector(const RE::NiQuaternion& a_quat);
        static NiMatrix3 QuaternionToMatrix(const NiQuaternion& a_quat);
    }; 

    struct Interp
    {
        [[nodiscard]] static float InterpTo(float a_current, float a_target, float a_deltaTime, float a_interpSpeed);
    };
}

namespace ObjectUtil
{ 
    struct Transform
    {
        static void TranslateTo(RE::BSScript::IVirtualMachine *vm, RE::VMStackID stackID, RE::TESObjectREFR *object, float afX, float afY, float afZ, float afAngleX, float afAngleY, float afAngleZ, float afSpeed, float afMaxRotationSpeed);
        static float InterpAngleTo(float a_current, float a_target, float a_deltaTime, float a_interpSpeed);
        static float Clamp(float value, float min, float max);
    };
}

namespace AnimUtil
{
    struct Idle
    {
        static bool Play(RE::TESIdleForm* idle, RE::Actor* actor, RE::DEFAULT_OBJECT action, RE::Actor* target);
    };
}

namespace FormUtil
{
    struct Parse
    {
        static RE::TESForm *GetFormFromMod(uint32_t formid, std::string modname);
        static RE::TESForm *GetFormFromMod(std::string modname, std::string formIDString);
        static RE::TESForm *GetFormFromConfigString(std::string str, std::string_view delimiter);
        static RE::TESForm *GetFormFromConfigString(std::string str);
        static RE::FormID GetFormIDFromMod(uint32_t relativeFormID, std::string modName);
        static RE::FormID GetFormIDFromMod(std::string relativeFormIDString, std::string modName);
        static RE::FormID GetFormIDFromConfigString(std::string str, std::string_view delimiter);
        static RE::FormID GetFormIDFromConfigString(std::string str);
        static RE::TESForm* GetForm(std::string_view data);
    };

    struct Quest 
    {
        static BGSBaseAlias *FindAliasByName(std::string_view name, TESQuest *owningQuest);
    };
}

namespace NifUtil
{
    struct Node
	{
        static NiAVObject* Clone(NiAVObject* original);
		static RE::NiAVObject* GetNiObject(RE::NiNode* a_root, const RE::BSFixedString& a_name);
		static void AttachToNode(RE::NiAVObject* a_object, RE::NiNode* a_node);
        static std::vector<BSGeometry*> GetAllGeometries(RE::NiAVObject* root);
	};

    struct Armature
    {
        static RE::NiNode* GetActorNode(RE::Actor* actor, std::string nodeName);
        static void AttachToNode(RE::NiAVObject* obj, RE::Actor* actor, std::string nodeName);
    };

    struct Collision
    {
        static bool ToggleMeshCollision(RE::NiAVObject* root, RE::bhkWorld* world, bool collisionState);
        static bool RemoveMeshCollision(RE::NiAVObject* root, RE::bhkWorld* world, bool collisionState);
    };
}