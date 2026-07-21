#pragma once
#include "CkEcsExt/CkEcsExt_Utils.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkMinimap/CkFogOfWar_Fragment.h"
#include "CkMinimap/CkFogOfWar_Fragment_Data.h"

#include "CkFogOfWar_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_FogOfWar"))
class CKMINIMAP_API UCk_Utils_FogOfWar_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_FogOfWar_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_FogOfWar);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Compose a FogOfWar exploration grid DIRECTLY onto InHandle (no child entity). An entity hosts at most ONE
    // grid — cover multiple map areas with one fog entity each. Exploration is GAMEPLAY state: it persists via
    // the v3 save pipeline (only when the entity is recipe-rebuildable) and is projector-agnostic — the Minimap
    // culls unexplored POIs through Request_SetFogOfWar, painters seed from Get_ExploredData + CellsRevealed.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Add Feature")
    static FCk_Handle_FogOfWar
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_FogOfWar_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|FogOfWar",
        DisplayName="[Ck][FogOfWar] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_FogOfWar
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|FogOfWar",
        DisplayName="[Ck][FogOfWar] Handle -> FogOfWar Handle",
        meta = (CompactNodeTitle = "<AsFogOfWar>", BlueprintAutocast))
    static FCk_Handle_FogOfWar
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid FogOfWar Handle",
        Category = "Ck|Utils|FogOfWar",
        meta = (CompactNodeTitle = "INVALID_FogOfWarHandle", Keywords = "make"))
    static FCk_Handle_FogOfWar
    Get_InvalidHandle() { return {}; };

public:
    // Locations OUTSIDE the grid's bounds always report explored (unfogged): fog gates only what it covers, and
    // mis-sized bounds then fail VISIBLE (POIs show) instead of silently hiding them forever.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Get Is Location Explored")
    static bool
    Get_IsLocationExplored(
        const FCk_Handle_FogOfWar& InFogOfWar,
        FVector InWorldLocation);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Get Explored Fraction")
    static float
    Get_ExploredFraction(
        const FCk_Handle_FogOfWar& InFogOfWar);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Get Cell Counts")
    static FIntPoint
    Get_CellCounts(
        const FCk_Handle_FogOfWar& InFogOfWar);

    // The PULL-SEED for fog painters (and the save payload): the full packed grid. Seed from this, then apply
    // CellsRevealed deltas — the signal replays at most its LAST payload to late binders.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Get Explored Data")
    static FCk_RepData_FogOfWar
    Get_ExploredData(
        const FCk_Handle_FogOfWar& InFogOfWar);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Add Revealer")
    static FCk_Handle_FogOfWar
    Request_AddRevealer(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Handle& InRevealer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Remove Revealer")
    static FCk_Handle_FogOfWar
    Request_RemoveRevealer(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Handle& InRevealer);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Reveal Location")
    static FCk_Handle_FogOfWar
    Request_RevealLocation(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_RevealLocation& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Reveal All")
    static FCk_Handle_FogOfWar
    Request_RevealAll(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Reset")
    static FCk_Handle_FogOfWar
    Request_Reset(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName="[Ck][FogOfWar] Request Set Explored")
    static FCk_Handle_FogOfWar
    Request_SetExplored(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Request_FogOfWar_SetExplored& InRequest);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName = "[Ck][FogOfWar] Bind To OnCellsRevealed")
    static FCk_Handle_FogOfWar
    BindTo_OnCellsRevealed(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_CellsRevealed& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName = "[Ck][FogOfWar] Bind To OnReset")
    static FCk_Handle_FogOfWar
    BindTo_OnReset(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_Reset& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName = "[Ck][FogOfWar] Unbind From OnCellsRevealed")
    static FCk_Handle_FogOfWar
    UnbindFrom_OnCellsRevealed(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_CellsRevealed& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|FogOfWar",
              DisplayName = "[Ck][FogOfWar] Unbind From OnReset")
    static FCk_Handle_FogOfWar
    UnbindFrom_OnReset(
        UPARAM(ref) FCk_Handle_FogOfWar& InFogOfWar,
        const FCk_Delegate_FogOfWar_Reset& InDelegate);

public:
    // Pure fog-grid math — no entities, no world (unit-tested in CkFogOfWar_Utils.spec.cpp).

    // ceil(extent / cell size) per axis, min 1x1, anchored at the bounds min corner. The last row/column
    // may overshoot the bounds (partial cells).
    static auto
    Get_CellCounts(
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FIntPoint;

    // Row-major cell index (CellY * Counts.X + CellX), or INDEX_NONE outside the bounds. Inclusive on
    // BOTH edges; in-bounds coords are clamped-floor into [0, Counts-1].
    static auto
    Get_CellIndex(
        const FVector& InWorldPos,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize,
        const FIntPoint& InCellCounts) -> int32;

    // World-XY center of a cell (reveal stamps test THIS point against the reveal radius).
    static auto
    Get_CellCenter(
        int32 InCellX,
        int32 InCellY,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FVector2D;

    // UV rect for painters, normalized over the BOUNDS extent and clamped — partial edge cells never seam
    // past the texture. UV = (Frame + 1) / 2 of Get_BoundsToFrame (U right = world +Y, V down = world -X).
    static auto
    Get_CellUVRect(
        int32 InCellIndex,
        const FCk_Minimap_WorldBounds& InBounds,
        float InCellSize) -> FBox2D;
};

// --------------------------------------------------------------------------------------------------------------------
