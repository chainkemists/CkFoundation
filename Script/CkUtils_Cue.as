namespace utils_cue
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem, FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::Ensure(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found") == false)
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Local(InCueOwnerEntity, InCueName, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem, FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::Ensure(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found") == false)
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue(InCueOwnerEntity, InCueName, InSpawnParams.InstancedStruct);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_generic
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Local(UCk_GenericCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue(UCk_GenericCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_audio
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Local(UCk_AudioCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue(UCk_AudioCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_objective
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Local(UCk_ObjectiveCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue(UCk_ObjectiveCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }
}

//--------------------------------------------------------------------------------------------------------------------------
