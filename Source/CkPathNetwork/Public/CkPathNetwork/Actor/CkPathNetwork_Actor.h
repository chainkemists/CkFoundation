#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <GameFramework/Info.h>

#include "CkPathNetwork_Actor.generated.h"

class UCk_PathNetwork_Detector_UE;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKPATHNETWORK_API FCk_PathNetwork_RepresentativeRoute
{
    GENERATED_BODY()

public:
    FCk_PathNetwork_RepresentativeRoute() = default;

    FCk_PathNetwork_RepresentativeRoute(
        const FName InName,
        const FVector& InStart,
        const FVector& InGoal)
        : _Name{InName}
        , _Start{InStart}
        , _Goal{InGoal}
    {}

    auto Get_Name() const -> FName { return _Name; }
    auto Get_Start() const -> const FVector& { return _Start; }
    auto Get_Goal() const -> const FVector& { return _Goal; }

    auto Set_Name(const FName InName) -> FCk_PathNetwork_RepresentativeRoute&
    {
        _Name = InName;
        return *this;
    }

    auto Set_Start(const FVector& InStart) -> FCk_PathNetwork_RepresentativeRoute&
    {
        _Start = InStart;
        return *this;
    }

    auto Set_Goal(const FVector& InGoal) -> FCk_PathNetwork_RepresentativeRoute&
    {
        _Goal = InGoal;
        return *this;
    }

private:
    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Representative Routes")
    FName _Name;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Representative Routes")
    FVector _Start = FVector::ZeroVector;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Representative Routes")
    FVector _Goal = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------

// Level-placed home of a path network: authoring + lifetime plumbing only, all runtime behavior
// lives on the entity it constructs at BeginPlay (authority only). Ribbons are stored
// actor-RELATIVE (MakeEditWidget's space; moves the network as one unit) — see Get_WorldRibbons.
UCLASS(Blueprintable, BlueprintType,
    DisplayName = "Ck Path Network",
    HideCategories = (Replication, Physics, Networking, Actor, Rendering, Collision, Input, LOD, HLOD, WorldPartition, DataLayers, Cooking, "Level Instance", Advanced, Tags, ComponentReplication, ComponentTick, Events),
    meta = (DisplayName = "Ck Path Network"))
class CKPATHNETWORK_API ACk_PathNetwork_UE : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_PathNetwork_UE);

    ACk_PathNetwork_UE();

protected:
    auto
    BeginPlay() -> void override;

    auto
    EndPlay(
        const EEndPlayReason::Type EndPlayReason) -> void override;

#if WITH_EDITOR
public:
    auto
    Tick(
        float InDeltaSeconds) -> void override;

    auto
    ShouldTickIfViewportsOnly() const -> bool override;

    // Editor authoring services own validation, transaction boundaries, and detector duplication.
    // InOwnedDetector must already be an instanced child of this actor.
    auto
    Set_EditorAuthoringConfiguration(
        const FCk_PathNetwork_BuildParams& InBuildParams,
        const FCk_PathNetwork_VectorizeParams& InVectorizeParams,
        UCk_PathNetwork_Detector_UE* InOwnedDetector,
        const FVector& InDetectionExtents,
        ECk_EnableDisable InAutoDetectOnBeginPlay) -> bool;

    auto
    Set_EditorAuthoringConfiguration(
        const FCk_PathNetwork_BuildParams& InBuildParams,
        const FCk_PathNetwork_VectorizeParams& InVectorizeParams,
        UCk_PathNetwork_Detector_UE* InOwnedDetector,
        const FVector& InDetectionExtents,
        ECk_EnableDisable InAutoDetectOnBeginPlay,
        ECk_EnableDisable InUseRecommendedFollowerTuning,
        const FCk_PathNetworkFollower_Tuning& InRecommendedFollowerTuning) -> bool;

    auto
    Get_RepresentativeRoutes() const
        -> const TArray<FCk_PathNetwork_RepresentativeRoute>&;

    auto
    Get_WorldRepresentativeRoutes() const
        -> TArray<FCk_PathNetwork_RepresentativeRoute>;

    auto
    Convert_WorldRepresentativeRouteToRelative(
        const FCk_PathNetwork_RepresentativeRoute& InWorldRoute) const
        -> FCk_PathNetwork_RepresentativeRoute;

    auto
    Set_RepresentativeRoutes(
        const TArray<FCk_PathNetwork_RepresentativeRoute>& InRoutes) -> void;
#endif

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|PathNetwork",
              DisplayName = "[Ck][PathNetwork] Get Network Handle")
    FCk_Handle_PathNetwork
    Get_NetworkHandle() const;

    // World-space detection bounds for the assigned detector: the actor's location +/- extents.
    UFUNCTION(BlueprintPure,
              Category = "Ck|PathNetwork",
              DisplayName = "[Ck][PathNetwork] Get Detection Bounds")
    FBox
    Get_DetectionBounds() const;

    // The authored (actor-relative) ribbons in world space — what the network entity is built from.
    UFUNCTION(BlueprintPure,
              Category = "Ck|PathNetwork",
              DisplayName = "[Ck][PathNetwork] Get World Ribbons")
    TArray<FCk_PathNetwork_Ribbon>
    Get_WorldRibbons() const;

    // Inverse of Get_WorldRibbons for a single ribbon (the editor tool stores detector output with it).
    UFUNCTION(BlueprintPure,
              Category = "Ck|PathNetwork",
              DisplayName = "[Ck][PathNetwork] Convert World Ribbon To Relative")
    FCk_PathNetwork_Ribbon
    Convert_WorldRibbonToRelative(
        const FCk_PathNetwork_Ribbon& InWorldRibbon) const;

private:
    // Actor-RELATIVE. Edit via the viewport point widgets (MakeEditWidget) or the details array.
    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork",
        meta = (AllowPrivateAccess = true))
    TArray<FCk_PathNetwork_Ribbon> _Ribbons;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork",
        meta = (AllowPrivateAccess = true))
    FCk_PathNetwork_BuildParams _BuildParams;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Detection",
        meta = (AllowPrivateAccess = true))
    FCk_PathNetwork_VectorizeParams _VectorizeParams;

    // Per-game detection (e.g. sample landscape paint weights). Authored inline; optional.
    UPROPERTY(EditAnywhere, Instanced,
        Category = "Ck|PathNetwork|Detection",
        meta = (AllowPrivateAccess = true))
    TObjectPtr<UCk_PathNetwork_Detector_UE> _Detector;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Detection",
        meta = (AllowPrivateAccess = true, ClampMin = "100.0"))
    FVector _DetectionExtents = FVector{50000.0, 50000.0, 10000.0};

    // Re-runs the detector at BeginPlay (after the saved ribbons are constructed) — the "always
    // fresh" mode for games whose levels change between editor bakes.
    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Detection",
        meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _AutoDetectOnBeginPlay = ECk_EnableDisable::Disable;

    // Optional per-map routing profile. Existing actors remain opt-out until a
    // designer explicitly enables and applies a recommendation.
    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Route Preferences",
        meta = (
            AllowPrivateAccess = true,
            DisplayName = "Use Map Route Preferences",
            ToolTip = "Publish this actor's route preference profile for game-specific followers to consume."))
    ECk_EnableDisable _UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;

    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Route Preferences",
        meta = (
            AllowPrivateAccess = true,
            ShowOnlyInnerProperties,
            DisplayName = "Recommended Follower Tuning"))
    FCk_PathNetworkFollower_Tuning _RecommendedFollowerTuning;

#if WITH_EDITORONLY_DATA
    UPROPERTY(EditAnywhere,
        Category = "Ck|PathNetwork|Representative Routes",
        meta = (
            AllowPrivateAccess = true,
            TitleProperty = "_Name",
            DisplayName = "Representative Route Watchlist"))
    TArray<FCk_PathNetwork_RepresentativeRoute> _RepresentativeRoutes;
#endif

public:
    CK_PROPERTY(_Ribbons);
    CK_PROPERTY_GET(_BuildParams);
    CK_PROPERTY_GET(_VectorizeParams);
    CK_PROPERTY_GET(_Detector);
    CK_PROPERTY_GET(_DetectionExtents);
    CK_PROPERTY_GET(_AutoDetectOnBeginPlay);
    CK_PROPERTY_GET(_UseRecommendedFollowerTuning);
    CK_PROPERTY_GET(_RecommendedFollowerTuning);

private:
    FCk_Handle_PathNetwork _NetworkHandle;
};

// --------------------------------------------------------------------------------------------------------------------
