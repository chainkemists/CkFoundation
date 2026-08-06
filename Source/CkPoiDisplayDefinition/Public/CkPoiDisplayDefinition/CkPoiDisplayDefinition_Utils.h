#pragma once

#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkRecord/Record/CkRecord_Utils.h"

#include "CkVisibleRange/CkVisibleRange_Fragment_Data.h"

#include "CkPoi/CkPoi_Fragment_Data.h"

#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment.h"
#include "CkPoiDisplayDefinition/CkPoiDisplayDefinition_Fragment_Data.h"

#include "CkPoiDisplayDefinition_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_PoiDisplayDefinition"))
class CKPOIDISPLAYDEFINITION_API UCk_Utils_PoiDisplayDefinition_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_PoiDisplayDefinition_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_PoiDisplayDefinition);

private:
    struct RecordOfPoiDisplayDefinitions_Utils : public ck::TUtils_RecordOfEntities<ck::FFragment_RecordOfPoiDisplayDefinitions> {};

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Direct-attach: composes ONE display definition onto InHandle itself (no child entity) plus a CkLabel carrying
    // the _Consumer tag. Use when an entity needs exactly one presentation config; for several consumer-keyed
    // definitions on one POI, use Create instead.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Add New Display Definition")
    static FCk_Handle_PoiDisplayDefinition
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_PoiDisplayDefinition_Spec& InParams);

    // Creates a display-definition CHILD entity under InPoi and composes it via Add, so the child is keyed by its
    // _Consumer tag (fragment + label) and connected to the POI's RecordOfPoiDisplayDefinitions. Multiple Creates
    // with distinct consumers compose several definitions on one POI. Also wires the POI's VisibleRange->child
    // ParentHidden cascade (bound once per POI).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Create New Display Definition")
    static FCk_Handle_PoiDisplayDefinition
    Create(
        UPARAM(ref) FCk_Handle_Poi& InPoi,
        const FCk_PoiDisplayDefinition_Spec& InParams);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|PoiDisplayDefinition",
        DisplayName="[Ck][PoiDisplayDefinition] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_PoiDisplayDefinition
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|PoiDisplayDefinition",
        DisplayName="[Ck][PoiDisplayDefinition] Handle -> PoiDisplayDefinition Handle",
        meta = (CompactNodeTitle = "<AsPoiDisplayDefinition>", BlueprintAutocast))
    static FCk_Handle_PoiDisplayDefinition
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid PoiDisplayDefinition Handle",
        Category = "Ck|Utils|PoiDisplayDefinition",
        meta = (CompactNodeTitle = "INVALID_PoiDisplayDefinitionHandle", Keywords = "make"))
    static FCk_Handle_PoiDisplayDefinition
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Consumer")
    static FGameplayTag
    Get_Consumer(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Priority")
    static int32
    Get_Priority(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Offscreen Policy")
    static ECk_Poi_OffscreenPolicy
    Get_OffscreenPolicy(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    // Authored on Params and immutable for the definition's lifetime — there is no Request to change it.
    // Still a soft reference: the caller decides when (and whether) to load the texture.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Icon")
    static TSoftObjectPtr<UTexture2D>
    Get_Icon(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    // The LIVE display state (Current), not the authored Params seed — this is what a projector or marker
    // widget draws.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Tint")
    static FLinearColor
    Get_Tint(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Size Hint")
    static FVector2D
    Get_SizeHint(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Is Parent Hidden")
    static bool
    Get_IsParentHidden(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

    // Parent-cascade hidden OR the definition's own VisibleRange (if composed) reporting hidden. The read a
    // projector uses to decide whether to draw this definition at all.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Get Is Effectively Hidden")
    static bool
    Get_IsEffectivelyHidden(
        const FCk_Handle_PoiDisplayDefinition& InHandle);

public:
    // Resolves the display definition on InPoi for InConsumer: the POI's own direct-attach definition first, then
    // the first exact-match child in its record. Returns an invalid handle when none matches (house TryGet_* contract).
    // Matching is MatchesTagExact — a consumer tag never resolves a definition keyed with a DESCENDANT of it.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Try Get Display Definition By Consumer")
    static FCk_Handle_PoiDisplayDefinition
    TryGet_PoiDisplayDefinition_ByConsumer(
        const FCk_Handle_Poi& InPoi,
        FGameplayTag InConsumer);

public:
    // Per-instance display overrides. Both are deferred (drained by
    // FProcessor_PoiDisplayDefinition_HandleRequests) and collapse into ONE OnDisplayChanged broadcast per
    // frame; re-asserting a value already in Current broadcasts nothing.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Request Set Tint",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PoiDisplayDefinition
    Request_SetTint(
        UPARAM(ref) FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Request_PoiDisplayDefinition_SetTint& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName="[Ck][PoiDisplayDefinition] Request Set Size Hint",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_PoiDisplayDefinition
    Request_SetSizeHint(
        UPARAM(ref) FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Request_PoiDisplayDefinition_SetSizeHint& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // Default binding policy is FireIfPayloadInFlight: a marker widget that binds in the same frame a
    // display change was requested must still get it — unlike the projector membership signals, the payload
    // here is a one-shot "re-read Current" nudge that is cheap to replay.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName = "[Ck][PoiDisplayDefinition] Bind To OnDisplayChanged")
    static FCk_Handle_PoiDisplayDefinition
    BindTo_OnDisplayChanged(
        UPARAM(ref) FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Delegate_PoiDisplayDefinition_DisplayChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|PoiDisplayDefinition",
              DisplayName = "[Ck][PoiDisplayDefinition] Unbind From OnDisplayChanged")
    static FCk_Handle_PoiDisplayDefinition
    UnbindFrom_OnDisplayChanged(
        UPARAM(ref) FCk_Handle_PoiDisplayDefinition& InHandle,
        const FCk_Delegate_PoiDisplayDefinition_DisplayChanged& InDelegate);

private:
    // Cascade handler bound once per owner to OnVisibleRange_HiddenChanged (signature = the signal payload). Walks the
    // owner's display-definition children and adds/removes FTag_PoiDisplayDefinition_ParentHidden to match InIsHidden.
    static auto
    DoOnOwnerHiddenChanged(
        FCk_Handle_VisibleRange InOwner,
        bool InIsHidden) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
