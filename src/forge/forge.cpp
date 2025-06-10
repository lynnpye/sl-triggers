
namespace SLT {

ForgeHandle ForgeManager::nextForgeHandle = 1;

bool ForgeManager::Register(RE::BSScript::IVirtualMachine* vm) {
    return
        GlobalContext::Register(vm)
        && ForgeManagerTemplate<CommandLine>::Register(vm)
        && ForgeManagerTemplate<FrameContext>::Register(vm)
        && ForgeManagerTemplate<Optional>::Register(vm)
        && ForgeManagerTemplate<TargetContext>::Register(vm)
        && ForgeManagerTemplate<ThreadContext>::Register(vm)
        ;
}

void ForgeManager::OnSave(SKSE::SerializationInterface* intfc) {
    GlobalContext::OnSave(intfc);
    ForgeManagerTemplate<CommandLine>::OnSave(intfc);
    ForgeManagerTemplate<FrameContext>::OnSave(intfc);
    ForgeManagerTemplate<Optional>::OnSave(intfc);
    ForgeManagerTemplate<TargetContext>::OnSave(intfc);
    ForgeManagerTemplate<ThreadContext>::OnSave(intfc);
}

void ForgeManager::OnLoad(SKSE::SerializationInterface* intfc) {
    GlobalContext::OnLoad(intfc);
    ForgeManagerTemplate<CommandLine>::OnLoad(intfc);
    ForgeManagerTemplate<FrameContext>::OnLoad(intfc);
    ForgeManagerTemplate<Optional>::OnLoad(intfc);
    ForgeManagerTemplate<TargetContext>::OnLoad(intfc);
    ForgeManagerTemplate<ThreadContext>::OnLoad(intfc);
}

void ForgeManager::OnRevert(SKSE::SerializationInterface* intfc) {
    GlobalContext::OnRevert(intfc);
    ForgeManagerTemplate<CommandLine>::OnRevert(intfc);
    ForgeManagerTemplate<FrameContext>::OnRevert(intfc);
    ForgeManagerTemplate<Optional>::OnRevert(intfc);
    ForgeManagerTemplate<TargetContext>::OnRevert(intfc);
    ForgeManagerTemplate<ThreadContext>::OnRevert(intfc);
}

}