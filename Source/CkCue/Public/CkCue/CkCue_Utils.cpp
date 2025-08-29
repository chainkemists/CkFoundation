#include "CkCue_Utils.h"

#include "CkCueSubsystem_Base.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Cue_UE, FCk_Handle_Cue, TSubclassOf<UCk_CueBase_EntityScript>);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Cue_UE::
    Request_Execute(
        const FCk_Handle& InOwnerEntity,
        const FGameplayTag& InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute Cue [{}]"), InCueName)
    { return {}; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InOwnerEntity);
    const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
        TEXT("CueReplicator Subsystem was INVALID when trying to execute Cue [{}]"), InCueName)
    { return {}; }

    return CueReplicatorSubsystem->Request_ExecuteCue(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    UCk_Utils_Cue_UE::
    Request_Execute_Local(
        const FCk_Handle& InOwnerEntity,
        const FGameplayTag& InCueName,
        FInstancedStruct InSpawnParams)
    -> FCk_Handle_PendingEntityScript
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwnerEntity),
        TEXT("OwnerEntity is invalid when trying to execute local Cue [{}]"), InCueName)
    { return {}; }

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InOwnerEntity);
    const auto CueReplicatorSubsystem = World->GetSubsystem<UCk_CueExecutor_Subsystem_Base_UE>();

    CK_ENSURE_IF_NOT(ck::IsValid(CueReplicatorSubsystem),
        TEXT("CueReplicator Subsystem was INVALID when trying to execute local Cue [{}]"), InCueName)
    { return {}; }

    return CueReplicatorSubsystem->Request_ExecuteCue_Local(InOwnerEntity, InCueName, InSpawnParams);
}

auto
    UCk_Utils_Cue_UE::
    Get_Name(
        const FCk_Handle_Cue& InCueEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InCueEntity);
}

auto
    UCk_Utils_Cue_UE::
    Get_CueClass(
        const FCk_Handle_Cue& InCueEntity)
    -> TSubclassOf<UCk_CueBase_EntityScript>
{
    return InCueEntity.Get<TSubclassOf<UCk_CueBase_EntityScript>>();
}

// --------------------------------------------------------------------------------------------------------------------
