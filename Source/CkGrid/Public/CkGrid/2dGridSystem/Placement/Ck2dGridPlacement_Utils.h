#pragma once

#include "Ck2dGridPlacement_Fragment_Data.h"
#include "Ck2dGridPlacement_Result_Data.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkGrid/2dGridSystem/Grid/Ck2dGridSystem_Fragment_Data.h"
#include "CkGrid/2dGridSystem/Object/Ck2dGridObject_Fragment_Data.h"

#include "Ck2dGridPlacement_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Unifying placement layer: composes GridObject (footprint/tag-gating), GridSystem (bounds/disabled
// cells) and GridOccupancy (overlap + stamping) into a single validated placement API. This is the
// layer gameplay calls — the raw occupancy layer underneath performs NO validation.
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_2dGridSystem"))
class CKGRID_API UCk_Utils_2dGridPlacement_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_2dGridPlacement_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Get Can Place")
    static FCk_2dGridPlacement_Result
    Get_CanPlace(
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle_2dGridObject& InObject,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation,
        ECk_GridConnectivity InConnectivity);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Get First Available Position")
    static FIntPoint
    Get_FirstAvailablePosition(
        const FCk_Handle_2dGridSystem& InGrid,
        const FCk_Handle_2dGridObject& InObject,
        ECk_CardinalRotation InRotation);

    // READ-BACK of what a live placement was made with. Nothing new is stored: Request_AddPlacement already
    // writes the grid, anchor and rotation onto the entry, and the occupancy persistence handler already
    // round-trips them (Ck2dGridOccupancy_Fragment.cpp re-registers from Entry.Get_Anchor()/Get_Rotation()).
    // Without a reader, a consumer whose own DERIVED state is Session — a navmesh cut, a footprint outline —
    // has no way to re-derive it after a load except by re-running the spatial resolve that produced the
    // anchor in the first place, which is a second implementation of placement math and drifts from this one.
    //
    // Pair with UCk_Utils_2dGridOccupancy_UE::Get_PlacementForOccupant to go occupant -> placement -> args.
    //
    // CONSUMER CONTRACT — branch on Get_Grid (or on the placement handle's own validity) FIRST.
    // Get_Anchor and Get_Rotation answer (0,0) and None for an invalid placement, and those are
    // legitimate values for a real placement at the grid origin: the two cases are indistinguishable
    // at the call site. There is deliberately no TryGet variant — an out-param pair would be a second
    // way to ask one question, and the grid read already separates "no placement" from every anchor.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Get Grid")
    static FCk_Handle_2dGridSystem
    Get_Grid(
        const FCk_Handle_2dGridPlacement& InPlacement);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Get Anchor")
    static FIntPoint
    Get_Anchor(
        const FCk_Handle_2dGridPlacement& InPlacement);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Get Rotation")
    static ECk_CardinalRotation
    Get_Rotation(
        const FCk_Handle_2dGridPlacement& InPlacement);

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Request Place",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridPlacement
    Request_Place(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        UPARAM(ref) FCk_Handle& InOccupant,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Request Remove",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static bool
    Request_Remove(
        UPARAM(ref) FCk_Handle_2dGridPlacement& InPlacement,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName = "[2dGridPlacement] Bind To OnObjectPlaced",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridSystem
    BindTo_OnObjectPlaced(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectPlaced& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName = "[2dGridPlacement] Unbind From OnObjectPlaced",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridSystem
    UnbindFrom_OnObjectPlaced(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectPlaced& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName = "[2dGridPlacement] Bind To OnObjectRemoved",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridSystem
    BindTo_OnObjectRemoved(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectRemoved& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName = "[2dGridPlacement] Unbind From OnObjectRemoved",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_2dGridSystem
    UnbindFrom_OnObjectRemoved(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        const FCk_Delegate_2dGridPlacement_ObjectRemoved& InDelegate);
};

// --------------------------------------------------------------------------------------------------------------------
