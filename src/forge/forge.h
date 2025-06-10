#pragma once

namespace SLT {

#pragma region SerializationHelper

class SerializationHelper {
public:
    template<typename T>
    static bool WriteData(SKSE::SerializationInterface* a_intfc, const T& data) {
        return a_intfc->WriteRecordData(&data, sizeof(T));
    }

    template<typename T>
    static bool ReadData(SKSE::SerializationInterface* a_intfc, T& data) {
        return a_intfc->ReadRecordData(&data, sizeof(T));
    }

    static bool WriteString(SKSE::SerializationInterface* a_intfc, const std::string& str) {
        std::size_t length = str.length();
        if (!WriteData(a_intfc, length)) return false;
        if (length > 0) {
            return a_intfc->WriteRecordData(str.c_str(), length);
        }
        return true;
    }

    static bool ReadString(SKSE::SerializationInterface* a_intfc, std::string& str) {
        std::size_t length;
        if (!ReadData(a_intfc, length)) return false;
        
        if (length > 0) {
            str.resize(length);
            return a_intfc->ReadRecordData(str.data(), length);
        } else {
            str.clear();
            return true;
        }
    }

    static bool WriteStringMap(SKSE::SerializationInterface* a_intfc, 
                              const std::map<std::string, std::string>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteString(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadStringMap(SKSE::SerializationInterface* a_intfc, 
                             std::map<std::string, std::string>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key, value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadString(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteSizeTMap(SKSE::SerializationInterface* a_intfc,
                              const std::map<std::string, std::size_t>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadSizeTMap(SKSE::SerializationInterface* a_intfc,
                             std::map<std::string, std::size_t>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key;
            std::size_t value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadData(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteSizeTVector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::size_t>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& value : vec) {
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadSizeTVector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::size_t>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::size_t value;
            if (!ReadData(a_intfc, value)) return false;
            vec.push_back(value);
        }
        return true;
    }

    static bool WriteUint32Map(SKSE::SerializationInterface* a_intfc,
                              const std::map<std::string, std::uint32_t>& map) {
        std::size_t mapSize = map.size();
        if (!WriteData(a_intfc, mapSize)) return false;
        
        for (const auto& [key, value] : map) {
            if (!WriteString(a_intfc, key)) return false;
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadUint32Map(SKSE::SerializationInterface* a_intfc,
                             std::map<std::string, std::uint32_t>& map) {
        std::size_t mapSize;
        if (!ReadData(a_intfc, mapSize)) return false;
        
        map.clear();
        for (std::size_t i = 0; i < mapSize; ++i) {
            std::string key;
            std::uint32_t value;
            if (!ReadString(a_intfc, key)) return false;
            if (!ReadData(a_intfc, value)) return false;
            map[key] = value;
        }
        return true;
    }

    static bool WriteUint32Vector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::uint32_t>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& value : vec) {
            if (!WriteData(a_intfc, value)) return false;
        }
        return true;
    }

    static bool ReadUint32Vector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::uint32_t>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::uint32_t value;
            if (!ReadData(a_intfc, value)) return false;
            vec.push_back(value);
        }
        return true;
    }
    
    static bool WriteStringVector(SKSE::SerializationInterface* a_intfc,
                                 const std::vector<std::string>& vec) {
        std::size_t vecSize = vec.size();
        if (!WriteData(a_intfc, vecSize)) return false;
        
        for (const auto& str : vec) {
            if (!WriteString(a_intfc, str)) return false;
        }
        return true;
    }

    static bool ReadStringVector(SKSE::SerializationInterface* a_intfc,
                                std::vector<std::string>& vec) {
        std::size_t vecSize;
        if (!ReadData(a_intfc, vecSize)) return false;
        
        vec.clear();
        vec.reserve(vecSize);
        for (std::size_t i = 0; i < vecSize; ++i) {
            std::string str;
            if (!ReadString(a_intfc, str)) return false;
            vec.push_back(std::move(str));
        }
        return true;
    }
};
#pragma endregion

#pragma region ForgeObject
/**
 * ForgeObject - potential base object for handle-based persisted Papyrus/SKSE transferrable objects
 */
class ForgeObject {
private:
    ForgeHandle _handle = 0;

public:
    ForgeObject() = default;
    virtual ~ForgeObject() = default;
    
    ForgeHandle GetHandle() const { return _handle; }
    void SetHandle(ForgeHandle handle) { _handle = handle; }
    
    virtual bool Serialize(SKSE::SerializationInterface* a_intfc) const {
        return SerializationHelper::WriteData(a_intfc, _handle);
    }
    
    virtual bool Deserialize(SKSE::SerializationInterface* a_intfc) {
        return SerializationHelper::ReadData(a_intfc, _handle);
    }
    
    // Optional Papyrus registration - override if needed
    static bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) { return true; }

protected:
    // Common serialization helpers available to derived classes
    template<typename T>
    bool WriteData(SKSE::SerializationInterface* a_intfc, const T& data) const {
        return SerializationHelper::WriteData(a_intfc, data);
    }
    
    template<typename T>
    bool ReadData(SKSE::SerializationInterface* a_intfc, T& data) const {
        return SerializationHelper::ReadData(a_intfc, data);
    }
    
    bool WriteString(SKSE::SerializationInterface* a_intfc, const std::string& str) const {
        return SerializationHelper::WriteString(a_intfc, str);
    }
    
    bool ReadString(SKSE::SerializationInterface* a_intfc, std::string& str) const {
        return SerializationHelper::ReadString(a_intfc, str);
    }
};
#pragma endregion


#pragma region ForgeManager
/**
 * Orchestrator for all handle-based managers - maintains existing API
 */
class ForgeManager {
private:

    static std::unordered_map<RE::FormID, ForgeHandle> formIdTargetMap;
    static std::map<std::string, std::string> globalVars;

public:
    ForgeManager() = delete;

    static bool Register(RE::BSScript::IVirtualMachine* vm);
    static void OnSave(SKSE::SerializationInterface* intfc);
    static void OnLoad(SKSE::SerializationInterface* intfc);
    static void OnRevert(SKSE::SerializationInterface* intfc);

public:
    
    static ForgeHandle nextForgeHandle;
    
    static ForgeHandle CreateNextForgeHandle() {
        if (nextForgeHandle == std::numeric_limits<ForgeHandle>::max() || nextForgeHandle < 1) {
            nextForgeHandle = 1;
        }
        return nextForgeHandle++;
    }
};
#pragma endregion

#pragma region ForgeManagerTemplate
/**
 * Template-based manager for individual handle-based types
 */
template<typename T>
class ForgeManagerTemplate {
private:
    static inline std::mutex _mutex;
    static inline std::unordered_map<ForgeHandle, std::unique_ptr<T>> _forgeObjects;

public:
    static ForgeHandle Create(std::unique_ptr<T> forgeObject) {
        std::lock_guard<std::mutex> lock(_mutex);
        ForgeHandle handle = ForgeManager::CreateNextForgeHandle();
        forgeObject->SetHandle(handle);
        _forgeObjects[handle] = std::move(forgeObject);
        return handle;
    }

    static T* GetFromHandle(ForgeHandle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _forgeObjects.find(handle);
        return (it != _forgeObjects.end()) ? it->second.get() : nullptr;
    }

    static void DestroyFromHandle(ForgeHandle handle) {
        std::lock_guard<std::mutex> lock(_mutex);
        _forgeObjects.erase(handle);
    }
    
    static bool Register(RE::BSScript::IVirtualMachine* vm) {
        if constexpr (requires { T::RegisterPapyrusFunctions(vm); }) {
            return T::RegisterPapyrusFunctions(vm);
        }
        return true;
    }

    static void OnSave(SKSE::SerializationInterface* intfc) {
        using SH = SerializationHelper;

        logger::info("OnSave: Starting {} serialization", typeid(T).name());
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        std::size_t objectCount = _forgeObjects.size();
        if (!SH::WriteData(intfc, objectCount)) {
            logger::error("OnSave: Failed to write object count for {}", typeid(T).name());
            return;
        }
        
        for (const auto& [handle, object] : _forgeObjects) {
            if (!SH::WriteData(intfc, handle)) {
                logger::error("OnSave: Failed to write handle {} for {}", handle, typeid(T).name());
                return;
            }
            
            if (!object->Serialize(intfc)) {
                logger::error("OnSave: Failed to serialize object with handle {} for {}", handle, typeid(T).name());
                return;
            }
        }
    }

    static void OnLoad(SKSE::SerializationInterface* intfc) {
        using SH = SerializationHelper;

        logger::info("OnLoad: Starting {} deserialization", typeid(T).name());
        
        std::lock_guard<std::mutex> lock(_mutex);
        
        _forgeObjects.clear();
        
        std::size_t objectCount;
        if (!SH::ReadData(intfc, objectCount)) {
            logger::error("OnLoad: Failed to read object count for {}", typeid(T).name());
            return;
        }
        
        for (std::size_t i = 0; i < objectCount; ++i) {
            ForgeHandle handle;
            if (!SH::ReadData(intfc, handle)) {
                logger::error("OnLoad: Failed to read handle for object {} in {}", i, typeid(T).name());
                return;
            }
            
            auto object = std::make_unique<T>();
            if (!object->Deserialize(intfc)) {
                logger::error("OnLoad: Failed to deserialize object with handle {} for {}", handle, typeid(T).name());
                return;
            }
            
            _forgeObjects[handle] = std::move(object);
        }
    }

    static void OnRevert(SKSE::SerializationInterface* intfc) {
        logger::info("OnRevert: Clearing all {} objects on revert", typeid(T).name());
        std::lock_guard<std::mutex> lock(_mutex);
        _forgeObjects.clear();
    }
};
#pragma endregion
}

#include "forge_optional.h"
#include "forge_commandline.h"
#include "forge_framecontext.h"
#include "forge_threadcontext.h"
#include "forge_targetcontext.h"