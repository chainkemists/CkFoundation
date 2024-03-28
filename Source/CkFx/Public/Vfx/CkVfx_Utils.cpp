#include "CkVfx_Utils.h"

#include "Vfx/CkVfx_Fragment.h"

#include "CkFx/CkFx_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vfx_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Vfx_ParamsData& InParams)
    -> FCk_Handle_Vfx
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Vfx_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Vfx_Current>();
    });

    auto NewVfxEntity = Cast(NewEntity);

    RecordOfVfx_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfVfx_Utils::Request_Connect(InHandle, NewVfxEntity);

    return NewVfxEntity;
}

auto
    UCk_Utils_Vfx_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleVfx_ParamsData& InParams)
    -> TArray<FCk_Handle_Vfx>
{
    return ck::algo::Transform<TArray<FCk_Handle_Vfx>>(InParams.Get_VfxParams(),
    [&](const FCk_Fragment_Vfx_ParamsData& InVfxParams)
    {
        return Add(InHandle, InVfxParams);
    });
}

auto
    UCk_Utils_Vfx_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfVfx_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Vfx, UCk_Utils_Vfx_UE, FCk_Handle_Vfx, ck::FFragment_Vfx_Current, ck::FFragment_Vfx_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Vfx_UE::
    TryGet_Vfx(
        const FCk_Handle& InVfxOwnerEntity,
        FGameplayTag InVfxName)
    -> FCk_Handle_Vfx
{
    return RecordOfVfx_Utils::Get_ValidEntry_If(InVfxOwnerEntity, ck::algo::MatchesGameplayLabelExact{InVfxName});
}

auto
    UCk_Utils_Vfx_UE::
    ForEach_Vfx(
        FCk_Handle& InVfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Vfx>
{
    auto Vfx = TArray<FCk_Handle_Vfx>{};

    ForEach_Vfx(InVfxOwnerEntity, [&](FCk_Handle_Vfx InVfx)
    {
        Vfx.Emplace(InVfx);

        std::ignore = InDelegate.ExecuteIfBound(InVfx, InOptionalPayload);
    });

    return Vfx;
}

auto
    UCk_Utils_Vfx_UE::
    ForEach_Vfx(
        FCk_Handle& InVfxOwnerEntity,
        const TFunction<void(FCk_Handle_Vfx)>& InFunc)
    -> void
{
    RecordOfVfx_Utils::ForEach_ValidEntry
    (
        InVfxOwnerEntity,
        InFunc,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

// --------------------------------------------------------------------------------------------------------------------
