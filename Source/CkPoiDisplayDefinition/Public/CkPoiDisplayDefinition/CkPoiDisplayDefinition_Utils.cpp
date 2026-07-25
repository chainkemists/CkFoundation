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
             "definitions on one Poi"),
        InHandle)
    { return Cast(InHandle); }

    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Consumer()),
        TEXT("PoiDisplayDefinition Add on Handle [{}] requires a valid Consumer tag (Poi.Consumer.*)"), InHandle)
    { return {}; }

    InHandle.Add<ck::FFragment_PoiDisplayDefinition_Params>(InParams);

    // CkLabel is set-once: on an entity that already carries one this no-ops with a Display log. Only Create's
    // record indexing keys off it, and its child is freshly created, so a direct-attach collision is harmless.
    UCk_Utils_GameplayLabel_UE::Add(InHandle, InParams.Get_Consumer());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_PoiDisplayDefinition_UE, FCk_Handle_PoiDisplayDefinition, ck::FFragment_PoiDisplayDefinition_Params);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Create(
        FCk_Handle_Poi& InPoi,
        const FCk_Fragment_PoiDisplayDefinition_ParamsData& InParams)
    -> FCk_Handle_PoiDisplayDefinition
{
    CK_ENSURE_IF_NOT(ck::IsValid(InPoi),
        TEXT("Invalid Poi Handle supplied to PoiDisplayDefinition Create"))
    { return {}; }

    // Add re-checks the consumer, but this has to reject BEFORE the child exists — otherwise a bad consumer
    // orphans a created entity under the Poi.
    CK_ENSURE_IF_NOT(ck::IsValid(InParams.Get_Consumer()),
        TEXT("PoiDisplayDefinition Create under Poi [{}] requires a valid Consumer tag (Poi.Consumer.*)"),
        InPoi)
    { return {}; }

    auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InPoi);

    auto NewDefinitionEntity = Add(NewEntity, InParams);

    RecordOfPoiDisplayDefinitions_Utils::AddIfMissing(InPoi, ECk_Record_EntryHandlingPolicy::Default);
    RecordOfPoiDisplayDefinitions_Utils::Request_Connect(InPoi, NewDefinitionEntity, ECk_Record_LabelRequirementPolicy::Optional);

    // (a) Bind the parent->child visibility cascade exactly once per owner. IgnorePayloadInFlight: the seed below reads
    // ground truth right now, so a replay of the last hidden-payload would double-apply. The bind is entity-scoped and
    // works whether or not the owner has composed VisibleRange yet — it just never fires until VisibleRange broadcasts.
    if (NOT InPoi.Has<ck::FTag_PoiDisplayDefinition_CascadeBound>())
    {
        ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Bind<&UCk_Utils_PoiDisplayDefinition_UE::DoOnOwnerHiddenChanged>(
            InPoi, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing);

        InPoi.Add<ck::FTag_PoiDisplayDefinition_CascadeBound>();
    }

    // (b) Seed: a child created under an already-hidden owner must not flash visible for a frame before the next
    // hidden transition (which may never come). If the owner is hidden right now, pre-apply the tag to this child.
    if (UCk_Utils_VisibleRange_UE::Has(InPoi) &&
        UCk_Utils_VisibleRange_UE::Get_IsHidden(UCk_Utils_VisibleRange_UE::Cast(InPoi)))
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
        const FCk_Handle_Poi& InPoi,
        FGameplayTag InConsumer)
    -> FCk_Handle_PoiDisplayDefinition
{
    // Direct-attach definition on the POI itself wins first.
    if (Has(InPoi))
    {
        const auto OwnerDefinition = CastChecked(InPoi);

        if (OwnerDefinition.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Consumer().MatchesTagExact(InConsumer))
        { return OwnerDefinition; }
    }

    auto Result = FCk_Handle_PoiDisplayDefinition{};

    if (RecordOfPoiDisplayDefinitions_Utils::Has(InPoi))
    {
        RecordOfPoiDisplayDefinitions_Utils::ForEach_ValidEntry(InPoi, [&](FCk_Handle_PoiDisplayDefinition InDefinition)
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
