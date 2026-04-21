namespace utils_cue
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue_Transient(
        TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue_Transient(InCueName, InSpawnParams.InstancedStruct, InReliabilityPolicy, InMulticastPolicy);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        TSubclassOf<UCk_CueExecutor_Subsystem_Base_UE> InCueSubsystem,
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        auto CueExecutor = Subsystem::GetWorldSubsystem(InCueSubsystem);

        if (ck::EnsureIfNot(ck::IsValid(CueExecutor), f"No CueExecutor subsystem [{InCueSubsystem}] found"))
        { return FCk_Handle_PendingEntityScript(); }

        return CueExecutor.Request_ExecuteCue(InCueOwnerEntity, InCueName, InSpawnParams.InstancedStruct, InReliabilityPolicy, InMulticastPolicy);
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
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        return utils_cue::Request_ExecuteCue_Transient(UCk_GenericCueExecutor_Subsystem_UE, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy);
    }

    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        return utils_cue::Request_ExecuteCue(UCk_GenericCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_audio
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        return utils_cue::Request_ExecuteCue(UCk_AudioCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_objective
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        return utils_cue::Request_ExecuteCue(UCk_ObjectiveCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------

namespace utils_cue_vfx
{
    FCk_Handle_PendingEntityScript
    Request_ExecuteCue(
        FCk_Handle InCueOwnerEntity,
        FGameplayTag InCueName,
        FAngelscriptAnyStructParameter InSpawnParams,
        ECk_Cue_ReliabilityPolicy InReliabilityPolicy = ECk_Cue_ReliabilityPolicy::Unreliable,
        ECk_Cue_MulticastPolicy InMulticastPolicy = ECk_Cue_MulticastPolicy::ServerAndAllClients)
    {
        return utils_cue::Request_ExecuteCue(UCk_VfxCueExecutor_Subsystem_UE, InCueOwnerEntity, InCueName, InSpawnParams, InReliabilityPolicy, InMulticastPolicy);
    }
}

//--------------------------------------------------------------------------------------------------------------------------
