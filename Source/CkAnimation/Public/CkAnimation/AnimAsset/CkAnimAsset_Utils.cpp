#include "CkAnimAsset_Utils.h"

#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimAsset_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_AnimAsset_ParamsData& InParams)
    -> FCk_Handle_AnimAsset
{
    auto NewAnimAssetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InAnimAssetEntity)
    {
        InAnimAssetEntity.Add<ck::FFragment_AnimAsset_Params>(InParams);
        UCk_Utils_GameplayLabel_UE::Add(InAnimAssetEntity, InParams.Get_AnimationAsset().Get_ID());
    });

    RecordOfAnimAssets_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfAnimAssets_Utils::Request_Connect(InHandle, NewAnimAssetEntity);

    return Conv_HandleToAnimAsset(NewAnimAssetEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    AddMultiple(
        FCk_Handle& InHandle,
        const FCk_Fragment_MultipleAnimAsset_ParamsData& InParams)
    -> FCk_Handle_AnimAsset
{
    for (const auto& params : InParams.Get_AnimAssetParams())
    {
        Add(InHandle, params);
    }

    return Conv_HandleToAnimAsset(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(AnimAsset, UCk_Utils_AnimAsset_UE, FCk_Handle_AnimAsset, ck::FFragment_AnimAsset_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimAsset_UE::
    TryGet_AnimAsset(
        const FCk_Handle& InAnimAssetOwnerEntity,
        FGameplayTag      InAnimAssetName)
    -> FCk_Handle_AnimAsset
{
    const auto FoundEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AnimAsset_UE,
        RecordOfAnimAssets_Utils>(InAnimAssetOwnerEntity, InAnimAssetName);

    if (ck::Is_NOT_Valid(FoundEntity))
    { return {}; }

    return Conv_HandleToAnimAsset(FoundEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    Get_Name(
        const FCk_Handle_AnimAsset& InAnimAssetEntity)
    -> FGameplayTag
{
    return UCk_Utils_GameplayLabel_UE::Get_Label(InAnimAssetEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    Get_All(
        const FCk_Handle& InAnimAssetOwnerEntity)
    -> TArray<FCk_Handle_AnimAsset>
{
    auto AllAnimAssets = TArray<FCk_Handle_AnimAsset>{};

    RecordOfAnimAssets_Utils::ForEach_ValidEntry(InAnimAssetOwnerEntity, [&](const auto& InAnimAssetEntity)
    {
        AllAnimAssets.Add(InAnimAssetEntity);
    });

    return AllAnimAssets;
}

auto
    UCk_Utils_AnimAsset_UE::
    Get_Animation(
        const FCk_Handle_AnimAsset& InAnimAssetEntity)
    -> FCk_AnimAsset_Animation
{
    const auto& AnimAssetParams = InAnimAssetEntity.Get<ck::FFragment_AnimAsset_Params>().Get_Params();

    return AnimAssetParams.Get_AnimationAsset();
}

// --------------------------------------------------------------------------------------------------------------------
