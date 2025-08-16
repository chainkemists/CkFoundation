#include "CkCue_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/EntityScript/CkEntityScript_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_Cue_UE, FCk_Handle_Cue, TSubclassOf<UCk_CueBase_EntityScript>);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Cue_UE::
    Add(
        FCk_Handle& InHandle,
        const FGameplayTag& InCueName,
        TSubclassOf<UCk_CueBase_EntityScript> InCueClass)
    -> FCk_Handle_Cue
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCueName),
        TEXT("CueName is invalid when trying to add Cue to Entity [{}]"), InHandle)
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InCueClass),
        TEXT("CueClass is invalid when trying to add Cue [{}] to Entity [{}]"), InCueName, InHandle)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InCueName);
        InNewEntity.Add<TSubclassOf<UCk_CueBase_EntityScript>>(InCueClass);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
        UCk_Utils_Handle_UE::Set_DebugName(InNewEntity, *ck::Format_UE(TEXT("Cue: [{}]"), InCueName));
#endif
    });

    auto NewCueEntity = ck::StaticCast<FCk_Handle_Cue>(NewEntity);

    RecordOfCues_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfCues_Utils::Request_Connect(InHandle, NewCueEntity, ECk_Record_LabelRequirementPolicy::Required);

    return NewCueEntity;
}

auto
    UCk_Utils_Cue_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfCues_Utils::Has(InHandle);
}

auto
    UCk_Utils_Cue_UE::
    TryGet_Cue(
        const FCk_Handle& InCueOwnerEntity,
        FGameplayTag InCueName)
    -> FCk_Handle_Cue
{
    return RecordOfCues_Utils::Get_ValidEntry_ByTag(InCueOwnerEntity, InCueName);
}

auto
    UCk_Utils_Cue_UE::
    Request_Execute(
        const FCk_Handle& InContextEntity,
        const FGameplayTag& InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InContextEntity);
    RequestEntity.Add<ck::FFragment_Cue_ExecuteRequest>(InCueName, InSpawnParams);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(RequestEntity, *ck::Format_UE(TEXT("Cue Execute Request [{}]"), InCueName));
#endif
}

auto
    UCk_Utils_Cue_UE::
    Request_Execute_Local(
        const FCk_Handle& InContextEntity,
        const FGameplayTag& InCueName,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    auto RequestEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InContextEntity);
    RequestEntity.Add<ck::FFragment_Cue_ExecuteRequestLocal>(InCueName, InSpawnParams);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    UCk_Utils_Handle_UE::Set_DebugName(RequestEntity, *ck::Format_UE(TEXT("Cue Execute Local Request [{}]"), InCueName));
#endif
}

auto
    UCk_Utils_Cue_UE::
    Request_Execute_Local(
        const FCk_Handle_Cue& InCueEntity,
        const FInstancedStruct& InSpawnParams)
    -> void
{
    const auto CueClass = InCueEntity.Get<TSubclassOf<UCk_CueBase_EntityScript>>();

    const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InCueEntity);
    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity_TransientOwner(
        World->GetSubsystem<UCk_EcsWorld_Subsystem_UE>());

    UCk_Utils_EntityScript_UE::Add(NewEntity, CueClass, InSpawnParams);

#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
    const auto CueName = Get_Name(InCueEntity);
    UCk_Utils_Handle_UE::Set_DebugName(NewEntity, *ck::Format_UE(TEXT("Direct Cue Execute [{}]"), CueName));
#endif
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

auto
    UCk_Utils_Cue_UE::
    ForEach_Cue(
        const FCk_Handle& InCueOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Cue>
{
    auto Cues = TArray<FCk_Handle_Cue>{};

    ForEach_Cue(InCueOwnerEntity, [&](FCk_Handle_Cue InCue)
    {
        if (InDelegate.IsBound())
        { InDelegate.Execute(InCue, InOptionalPayload); }
        else
        { Cues.Emplace(InCue); }
    });

    return Cues;
}

auto
    UCk_Utils_Cue_UE::
    ForEach_Cue(
        const FCk_Handle& InCueOwnerEntity,
        const TFunction<void(FCk_Handle_Cue)>& InFunc)
    -> void
{
    RecordOfCues_Utils::ForEach_ValidEntry(InCueOwnerEntity, InFunc);
}

// --------------------------------------------------------------------------------------------------------------------
