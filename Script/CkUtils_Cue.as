namespace utils_cue
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Transient(
        TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Transient(InCueName, InSpawnParams.InstancedStruct, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Transient_Local(TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Transient_Local(InCueName, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem, FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Local(InCueOwnerEntity, InCueName, InSpawnParams.InstancedStruct);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem,
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue(InCueOwnerEntity, InCueName, InSpawnParams.InstancedStruct, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_generic
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Transient(
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        return utils_cue::Request_ExecuteCue_Transient(UCk_GenericCueExecutor_Subsystem_UE, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Transient_Local(FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Transient_Local(UCk_GenericCueExecutor_Subsystem_UE, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Local(UCk_GenericCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        return utils_cue::Request_ExecuteCue(UCk_GenericCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
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
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        return utils_cue::Request_ExecuteCue(UCk_AudioCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
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
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        return utils_cue::Request_ExecuteCue(UCk_ObjectiveCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_vfx
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Local(FCk_Handle InCueOwnerEntity, FGameplayTag InCueName, FAngelscriptAnyStructParameter InSpawnParams)
    {
        return utils_cue::Request_ExecuteCue_Local(UCk_VfxCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::MulticastToClients,
        ECk_Cue_ExecutionPolicy InExecutionPolicy = ECk_Cue_ExecutionPolicy::Replicated)
    {
        return utils_cue::Request_ExecuteCue(UCk_VfxCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy, InExecutionPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------