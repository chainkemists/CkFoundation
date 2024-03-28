#include "CkSfx_Utils.h"

#include "Sfx/CkSfx_Fragment.h"

#include "CkFx/CkFx_Log.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sfx_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_Sfx_ParamsData& InParams)
    -> FCk_Handle_Sfx
{
     auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InNewEntity)
     {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Name());

        InNewEntity.Add<ck::FFragment_Sfx_Params>(InParams);
        InNewEntity.Add<ck::FFragment_Sfx_Current>();
    });

    auto NewSfxEntity = Cast(NewEntity);

    RecordOfSfx_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfSfx_Utils::Request_Connect(InHandle, NewSfxEntity);

    return NewSfxEntity;
}

auto
    UCk_Utils_Sfx_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleSfx_ParamsData& InParams)
    -> TArray<FCk_Handle_Sfx>
{
    return ck::algo::Transform<TArray<FCk_Handle_Sfx>>(InParams.Get_SfxParams(),
    [&](const FCk_Fragment_Sfx_ParamsData& InSfxParams)
    {
        return Add(InHandle, InSfxParams);
    });
}

auto
    UCk_Utils_Sfx_UE::
    Has_Any(
        const FCk_Handle& InHandle)
    -> bool
{
    return RecordOfSfx_Utils::Has(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Sfx, UCk_Utils_Sfx_UE, FCk_Handle_Sfx, ck::FFragment_Sfx_Current, ck::FFragment_Sfx_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Sfx_UE::
    TryGet_Sfx(
        const FCk_Handle& InSfxOwnerEntity,
        FGameplayTag InSfxName)
    -> FCk_Handle_Sfx
{
    return RecordOfSfx_Utils::Get_ValidEntry_If(InSfxOwnerEntity, ck::algo::MatchesGameplayLabelExact{InSfxName});
}

auto
    UCk_Utils_Sfx_UE::
    ForEach_Sfx(
        FCk_Handle& InSfxOwnerEntity,
        const FInstancedStruct& InOptionalPayload,
        const FCk_Lambda_InHandle& InDelegate)
    -> TArray<FCk_Handle_Sfx>
{
    auto Sfx = TArray<FCk_Handle_Sfx>{};

    ForEach_Sfx(InSfxOwnerEntity, [&](FCk_Handle_Sfx InSfx)
    {
        Sfx.Emplace(InSfx);

        std::ignore = InDelegate.ExecuteIfBound(InSfx, InOptionalPayload);
    });

    return Sfx;
}

auto
    UCk_Utils_Sfx_UE::
    ForEach_Sfx(
        FCk_Handle& InSfxOwnerEntity,
        const TFunction<void(FCk_Handle_Sfx)>& InFunc)
    -> void
{
    RecordOfSfx_Utils::ForEach_ValidEntry
    (
        InSfxOwnerEntity,
        InFunc,
        ECk_Record_ForEach_Policy::IgnoreRecordMissing
    );
}

// --------------------------------------------------------------------------------------------------------------------
