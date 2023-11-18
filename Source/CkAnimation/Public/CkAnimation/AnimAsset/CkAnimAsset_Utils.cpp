#include "CkAnimAsset_Utils.h"

#include "CkEcsBasics/EntityHolder/CkEntityHolder_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_AnimAsset_UE::
    Add(
        FCk_Handle InHandle,
        const FCk_Fragment_AnimAsset_ParamsData& InParams)
    -> void
{
    const auto NewAnimAssetEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InHandle, [&](FCk_Handle InAnimAssetEntity)
    {
        InAnimAssetEntity.Add<ck::FFragment_AnimAsset_Params>(InParams);

        ck::UCk_Utils_OwningEntity::Add(InAnimAssetEntity, InHandle);
        UCk_Utils_GameplayLabel_UE::Add(InAnimAssetEntity, InParams.Get_AnimationAsset().Get_ID());
    });

    RecordOfAnimAssets_Utils::AddIfMissing(InHandle, ECk_Record_EntryHandlingPolicy::DisallowDuplicateNames);
    RecordOfAnimAssets_Utils::Request_Connect(InHandle, NewAnimAssetEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    AddMultiple(
        FCk_Handle InHandle,
        const FCk_Fragment_MultipleAnimAsset_ParamsData& InParams)
    -> void
{
    for (const auto& params : InParams.Get_AnimAssetParams())
    {
        Add(InHandle, params);
    }
}

auto
    UCk_Utils_AnimAsset_UE::
    Has(
        FCk_Handle InAnimAssetOwnerEntity,
        FGameplayTag InAnimAssetName)
    -> bool
{
    const auto& AnimAssetEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel<
        UCk_Utils_AnimAsset_UE,
        RecordOfAnimAssets_Utils>(InAnimAssetOwnerEntity, InAnimAssetName);

    return ck::IsValid(AnimAssetEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    Has_Any(
        FCk_Handle InAnimAssetOwnerEntity)
    -> bool
{
    return RecordOfAnimAssets_Utils::Has(InAnimAssetOwnerEntity);
}

auto
    UCk_Utils_AnimAsset_UE::
    Ensure(
        FCk_Handle InAnimAssetOwnerEntity,
        FGameplayTag InAnimAssetName)
    -> bool
{
    CK_ENSURE_IF_NOT(Has(InAnimAssetOwnerEntity, InAnimAssetName), TEXT("Handle [{}] does NOT have AnimAsset [{}]"), InAnimAssetOwnerEntity, InAnimAssetName)
    { return false; }

    return true;
}

auto
    UCk_Utils_AnimAsset_UE::
    Ensure_Any(
        FCk_Handle InAnimAssetOwnerEntity)
    -> bool
{
    CK_ENSURE_IF_NOT(Has_Any(InAnimAssetOwnerEntity), TEXT("Handle [{}] does NOT have any AnimAsset [{}]"), InAnimAssetOwnerEntity)
    { return false; }

    return true;
}

auto
    UCk_Utils_AnimAsset_UE::
    Get_All(
        FCk_Handle InAnimAssetOwnerEntity)
    -> TArray<FGameplayTag>
{
    if (NOT Has_Any(InAnimAssetOwnerEntity))
    { return {}; }

    const auto& AnimAssetEntities = RecordOfAnimAssets_Utils::Get_AllRecordEntries(InAnimAssetOwnerEntity);

    return ck::algo::Transform<TArray<FGameplayTag>>(AnimAssetEntities, [&](FCk_Handle InAnimAssetEntity)
    {
        return UCk_Utils_GameplayLabel_UE::Get_Label(InAnimAssetEntity);
    });
}

auto
    UCk_Utils_AnimAsset_UE::
    Get_Animation(
        FCk_Handle   InAnimAssetOwnerEntity,
        FGameplayTag InAnimName)
    -> FCk_AnimAsset_Animation
{
    if (NOT Ensure(InAnimAssetOwnerEntity, InAnimName))
    { return {}; }

    const auto& AnimAssetEntity = Get_EntityOrRecordEntry_WithFragmentAndLabel
        <UCk_Utils_AnimAsset_UE, RecordOfAnimAssets_Utils>(InAnimAssetOwnerEntity, InAnimName);

    const auto& AnimAssetParams = AnimAssetEntity.Get<ck::FFragment_AnimAsset_Params>().Get_Params();

    return AnimAssetParams.Get_AnimationAsset();
}

auto
    UCk_Utils_AnimAsset_UE::
    Has(
        FCk_Handle InHandle)
    -> bool
{
    return InHandle.Has_All<ck::FFragment_AnimAsset_Params>();
}

// --------------------------------------------------------------------------------------------------------------------
