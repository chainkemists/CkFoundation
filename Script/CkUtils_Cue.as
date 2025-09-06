namespace utils_cue_executor
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InLifetimeOwner, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        Subsystem::GetWorldSubsystem(UCk_EcsWorld_Subsystem_UE);
        Subsystem::GetWorldSubsystem(UCk_AudioCueExecutor_Subsystem_UE);

        auto LifetimeOwner = InLifetimeOwner;
        auto CueExecutor = Subsystem::GetWorldSubsystem(UCk_AudioCueExecutor_Subsystem_UE);

        if (ck::Ensure(ck::IsValid(CueExecutor), "No CueExecutor subsystem found") == false)
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Local(InLifetimeOwner, InCueName, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(FCk_Handle InLifetimeOwner, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto LifetimeOwner = InLifetimeOwner;
        auto CueExecutor = Subsystem::GetWorldSubsystem(UCk_AudioCueExecutor_Subsystem_UE);

        if (ck::Ensure(ck::IsValid(CueExecutor), "No CueExecutor subsystem found") == false)
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue(InLifetimeOwner, InCueName, InSpawnParams.InstancedStruct);
    }
}