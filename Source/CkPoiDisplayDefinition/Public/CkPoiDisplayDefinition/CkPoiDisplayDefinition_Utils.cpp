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
        const FCk_PoiDisplayDefinition_Spec& InParams)
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

    InHandle.Add<ck::FFragment_PoiDisplayDefinition_Params>(
        InParams.Get_Consumer(),
        InParams.Get_Icon(),
        InParams.Get_Priority(),
        InParams.Get_OffscreenPolicy());

    // Current takes the MUTABLE half of the authored seed, and owns it from here on. The icon is
    // immutable post-Add and is served straight from Params, so it is not copied here.
    InHandle.Add<ck::FFragment_PoiDisplayDefinition_Current>();

    auto& Current = InHandle.Get<ck::FFragment_PoiDisplayDefinition_Current>();
    Current._Tint     = InParams.Get_Tint();
    Current._SizeHint = InParams.Get_SizeHint();

    // CkLabel is set-once: on an entity that already carries one this no-ops with a Display log. Only Create's
    // record indexing keys off it, and its child is freshly created, so a direct-attach collision is harmless.
    UCk_Utils_GameplayLabel_UE::Add(InHandle, InParams.Get_Consumer());

    return Cast(InHandle);
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(UCk_Utils_PoiDisplayDefinition_UE, FCk_Handle_PoiDisplayDefinition, ck::FFragment_PoiDisplayDefinition_Params, ck::FFragment_PoiDisplayDefinition_Current);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Create(
        FCk_Handle_Poi& InPoi,
        const FCk_PoiDisplayDefinition_Spec& InParams)
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

    // IgnorePayloadInFlight: the seed below reads ground truth now, so a replayed payload would double-apply
    if (NOT InPoi.Has<ck::FTag_PoiDisplayDefinition_CascadeBound>())
    {
        ck::UUtils_Signal_OnVisibleRange_HiddenChanged::Bind<&UCk_Utils_PoiDisplayDefinition_UE::DoOnOwnerHiddenChanged>(
            InPoi, ECk_Signal_BindingPolicy::IgnorePayloadInFlight, ECk_Signal_PostFireBehavior::DoNothing);

        InPoi.Add<ck::FTag_PoiDisplayDefinition_CascadeBound>();
    }

    // Seed, so a child of an already-hidden owner never flashes visible awaiting a transition that may never come
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
    Get_Icon(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> TSoftObjectPtr<UTexture2D>
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Params>().Get_Icon();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_Tint(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> FLinearColor
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Current>().Get_Tint();
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Get_SizeHint(
        const FCk_Handle_PoiDisplayDefinition& InHandle)
    -> FVector2D
{
    return InHandle.Get<ck::FFragment_PoiDisplayDefinition_Current>().Get_SizeHint();
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

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Request_SetTint(
        FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Request_PoiDisplayDefinition_SetTint& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PoiDisplayDefinition
{
    CK_CALLSTACK_RECORD(ck::FFragment_PoiDisplayDefinition_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_PoiDisplayDefinition_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    Request_SetSizeHint(
        FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Request_PoiDisplayDefinition_SetSizeHint& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate)
    -> FCk_Handle_PoiDisplayDefinition
{
    CK_CALLSTACK_RECORD(ck::FFragment_PoiDisplayDefinition_Requests, InHandle);

    if (InDelegate.IsBound())
    { InRequest.Set_CompletionDelegate(InDelegate); }

    InHandle.AddOrGet<ck::FFragment_PoiDisplayDefinition_Requests>()._Requests.Emplace(InRequest);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    BindTo_OnDisplayChanged(
        FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Delegate_PoiDisplayDefinition_DisplayChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy,
        ECk_Signal_PostFireBehavior InPostFireBehavior)
    -> FCk_Handle_PoiDisplayDefinition
{
    ck::UUtils_Signal_OnPoiDisplayDefinition_DisplayChanged::Bind(InHandle, InDelegate, InBindingPolicy);

    return InHandle;
}

auto
    UCk_Utils_PoiDisplayDefinition_UE::
    UnbindFrom_OnDisplayChanged(
        FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Delegate_PoiDisplayDefinition_DisplayChanged& InDelegate)
    -> FCk_Handle_PoiDisplayDefinition
{
    ck::UUtils_Signal_OnPoiDisplayDefinition_DisplayChanged::Unbind(InHandle, InDelegate);

    return InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
