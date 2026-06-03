#pragma once

#include "Ck2dGridPlacement_Fragment_Data.h"
#include "Ck2dGridPlacement_Result_Data.h"

#include "CkCore/Enums/CkEnums.h"

#include "CkEcs/Handle/CkHandle.h"
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

public:
    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Request Place")
    static FCk_Handle_2dGridPlacement
    Request_Place(
        UPARAM(ref) FCk_Handle_2dGridSystem& InGrid,
        UPARAM(ref) FCk_Handle& InOccupant,
        const FIntPoint& InAnchor,
        ECk_CardinalRotation InRotation);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|2dGridPlacement",
              DisplayName="[Ck][2dGridPlacement] Request Remove")
    static bool
    Request_Remove(
        UPARAM(ref) FCk_Handle_2dGridPlacement& InPlacement);

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
