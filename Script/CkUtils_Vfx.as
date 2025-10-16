namespace utils_vfx
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InHandle,
        FGameplayTag InTag,
        FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto VfxCueExecutor = Subsystem::GetWorldSubsystem(UCk_VfxCueExecutor_Subsystem_UE);
        return VfxCueExecutor.Request_ExecuteCue(InHandle, InTag, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(
        FCk_Handle InHandle,
        FGameplayTag InTag,
        FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto VfxCueExecutor = Subsystem::GetWorldSubsystem(UCk_VfxCueExecutor_Subsystem_UE);
        return VfxCueExecutor.Request_ExecuteCue_Local(InHandle, InTag, InSpawnParams.InstancedStruct);
    }
}