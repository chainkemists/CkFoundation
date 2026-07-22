#include "CkPoiDisplayDefinition_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkLabel/CkLabel_Utils.h"

#include "CkVisibleRange/CkVisibleRange_Fragment.h"
#include "CkVisibleRange/CkVisibleRange_Utils.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(Tag_PoiDisplayDefinition_ConsumerRootName, TEXT("Poi.Consumer"))

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Add(
        FCk_Handle& InHandle,
        const FCk_Fragment_PoiDisplayDefinition_ParamsData& InParams)
    -> FCk_Handle_PoiDisplayDefinition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InHandle), TEXT("Invalid Handle supplied to PoiDisplayDefinition Add"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT Has(InHandle),
        TEXT("Handle [{}] already has the PoiDisplayDefinition feature. Use Create for multiple consumer-keyed "
             "definitions on one owner"),
        InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Consumer()),
        TEXT("PoiDisplayDefinition Add on Entity [{}] requires a valid Consumer tag (Poi.Consumer.*)"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_PoiDisplayDefinition_Params>(InParams);

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_PoiDisplayDefinition_UE, FCk_Handle_PoiDisplayDefinition, ck::FFragment_PoiDisplayDefinition_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Create(
        FCk_Handle& InLifetimeOwner,
        const FCk_Fragment_PoiDisplayDefinition_ParamsData& InParams)
    -> FCk_Handle_PoiDisplayDefinition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InLifetimeOwner),
        TEXT("Invalid LifetimeOwner supplied to PoiDisplayDefinition Create"))
    { return {}; }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Consumer()),
        TEXT("PoiDisplayDefinition Create under Entity [{}] requires a valid Consumer tag (Poi.Consumer.*)"),
        InLifetimeOwner)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InLifetimeOwner, [&](FCk_Handle InNewEntity)
    {
        UCk_Utils_GameplayLabel_UE::Add(InNewEntity, InParams.Get_Consumer());

        InNewEntity.Add<ck::FFragment_PoiDisplayDefinition_Params>(InParams);
    });

    auto NewDefinitionEntity = ck::StaticCast<FCk_Handle_PoiDisplayDefinition>(NewEntity);

    RecordOfPoiDisplayDefinitions_Utils::AddIfMissing(InLifetimeOwner, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfPoiDisplayDefinitions_Utils::Request_Connect(InLifetimeOwner, NewDefinitionEntity, ECk_Record_LabelRequirementPolicy::Optional);

    // (a) Bind the parent->child visibility cascade exactly once per owner. IgnorePayloadInFlight: the seed below reads
    // ground truth right now, so a replay of the last hidden-payload would double-apply. The bind is entity-scoped and
    // works whether or not the owner has composed VisibleRange yet — it just never fires until VisibleRange broadcasts.
    if (NOT InLifetimeOwner.Has<ck::FTag_PoiDisplayDefinition_CascadeBound>())
    {
        ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Bind<&UCk_Utils_PoiDisplayDefinition_UE::DoOnOwnerHiddenChanged>(
            InLifetimeOwner, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing);

        InLifetimeOwner.Add<ck::FTag_PoiDisplayDefinition_CascadeBound>();
    }

    // (b) Seed: a child created under an already-hidden owner must not flash visible for a frame before the next
    // hidden transition (which may never come). If the owner is hidden right now, pre-apply the tag to this child.
    if (UCk_Utils_VisibleRange_UE::Has(InLifetimeOwner) &&
        UCk_Utils_VisibleRange_UE::Get_IsHidden(UCk_Utils_VisibleRange_UE::Cast(InLifetimeOwner)))
    {
        NewDefinitionEntity.Add<ck::FTag_PoiDisplayDefinition_ParentHidden>();
    }

    return NewDefinitionEntity;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    DoOnOwnerHiddenChanged(
        FCk_Handle_VisibleRange InOwner,
        bool InIsHidden)
    -> void
{
    // Bind exists but the owner's record may be gone (all children destroyed later). A missing record is a correct
    // silent no-op here — there is nothing to cascade to.
    if (NOT RecordOfPoiDisplayDefinitions_Utils::Has(InOwner))
    { return; }

    RecordOfPoiDisplayDefinitions_Utils::ForEach_ValidEntry(InOwner, [&](FCk_Handle_PoiDisplayDefinition InDefinition)
    {
        if (InIsHidden)
        {
            if (NOT InDefinition.Has<ck::FTag_PoiDisplayDefinition_ParentHidden>())
            { InDefinition.Add<ck::FTag_PoiDisplayDefinition_ParentHidden>(); }
        }
        else
        {
            if (InDefinition.Has<ck::FTag_PoiDisplayDefinition_ParentHidden>())
            { InDefinition.Remove<ck::FTag_PoiDisplayDefinition_ParentHidden>(); }
        }
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_Consumer(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> FGameplayTag
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Consumer();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_Priority(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> int32
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Priority();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_OffscreenPolicy(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> ECk_Poi_OffscreenPolicy
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_OffscreenPolicy();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_DisplayAsset(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> TSoftObjectPtr<UCk_Poi_DisplayDefinition_PDA>
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_DisplayAsset();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_IsParentHidden(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> bool
{
    return InHandle.Has<ck::FTag_PoiDisplayDefinition_ParentHidden>();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_IsEffectivelyHidden(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> bool
{
    if (InHandle.Has<ck::FTag_PoiDisplayDefinition_ParentHidden>())
    { return true; }

    return UCk_Utils_VisibleRange_UE::Has(InHandle) &&
        UCk_Utils_VisibleRange_UE::Get_IsHidden(UCk_Utils_VisibleRange_UE::Cast(InHandle));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    TryGet_PoiDisplayDefinition_ByConsumer(
        const FCk_Handle& InOwner,
        FGameplayTag InConsumer)
    -> FCk_Handle_PoiDisplayDefinition
{
    // Direct-attach definition on the owner itself wins first.
    if (Has(InOwner))
    {
        const auto OwnerDefinition = ck::StaticCast<FCk_Handle_PoiDisplayDefinition>(InOwner);

        if (OwnerDefinition.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Consumer().MatchesTagExact(InConsumer))
        { return OwnerDefinition; }
    }

    auto Result = FCk_Handle_PoiDisplayDefinition{};

    if (RecordOfPoiDisplayDefinitions_Utils::Has(InOwner))
    {
        RecordOfPoiDisplayDefinitions_Utils::ForEach_ValidEntry(InOwner, [&](FCk_Handle_PoiDisplayDefinition InDefinition)
        {
            if (ck::IsValid(Result))
            { return; }

            if (InDefinition.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Consumer().MatchesTagExact(InConsumer))
            { Result = InDefinition; }
        });
    }

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
