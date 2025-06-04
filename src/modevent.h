// Finally! The ACTUAL simple way to send Papyrus events

namespace SLT {
namespace ModEvent {

// Send event to all registered handlers (replaces ModEvent.Create + Push + Send)
template<typename... Args>
bool SendToAll(std::string_view eventName, Args... args) {
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) return false;
    
    // Strip references and cv-qualifiers to make MakeFunctionArguments happy
    auto* functionArgs = RE::MakeFunctionArguments(std::decay_t<Args>(args)...);
    
    vm->SendEventAll(RE::BSFixedString(eventName), functionArgs);
    
    return true;
}

// Send event to specific object
template<typename... Args>
bool SendToObject(RE::VMHandle handle, std::string_view eventName, Args... args) {
    auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
    if (!vm) return false;
    
    // Strip references and cv-qualifiers to make MakeFunctionArguments happy
    auto* functionArgs = RE::MakeFunctionArguments(std::decay_t<Args>(args)...);
    
    vm->SendEvent(handle, RE::BSFixedString(eventName), functionArgs);
    
    return true;
}

// Convenience functions for common patterns
bool Send(std::string_view eventName) {
    return SendToAll(eventName);
}

bool SendWithForm(std::string_view eventName, RE::TESForm* form) {
    return SendToAll(eventName, form);
}

bool SendWithString(std::string_view eventName, std::string_view str) {
    return SendToAll(eventName, RE::BSFixedString(str));
}

// Standard ModEvent pattern: OnModEvent(string eventName, string strArg, float numArg, Form sender)
bool SendModEvent(std::string_view eventName, std::string_view strArg = "", float numArg = 0.0f, RE::TESForm* sender = nullptr) {
    // The "OnModEvent" handler expects: eventName, strArg, numArg, sender
    return SendToAll("OnModEvent", RE::BSFixedString(eventName), RE::BSFixedString(strArg), numArg, sender);
}

} // namespace ModEvent
} // namespace SLT

// Usage examples:
/*

// Simple custom event (scripts register for "MyEvent")
SLT::ModEvent::Send("MyEvent");

// Custom event with form (scripts register for "PlayerAction") 
SLT::ModEvent::SendWithForm("PlayerAction", RE::PlayerCharacter::GetSingleton());

// Standard ModEvent pattern (scripts register for "OnModEvent")
SLT::ModEvent::SendModEvent("MyEventName", "string data", 42.0f, targetActor);

// Direct template usage for any argument types
SLT::ModEvent::SendToAll("ComplexEvent", someActor, "message", 123, 45.6f, true);

// Send to specific object instead of all
auto handle = GetSomeObjectHandle();
SLT::ModEvent::SendToObject(handle, "PrivateEvent", "data");

*/