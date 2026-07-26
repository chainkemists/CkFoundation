#include "CkPathNetwork_Actor.h"

#include "CkPathNetwork/CkPathNetwork_Log.h"
#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Subsystem/CkEcsWorld_Subsystem.h"

#include <Components/SceneComponent.h>
#include <Engine/World.h>

#if WITH_EDITOR
#include <DrawDebugHelpers.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

ACk_PathNetwork_UE::
    ACk_PathNetwork_UE()
{
    // AInfo has no root component; the relative-ribbon storage (+ MakeEditWidget point widgets) needs one for its frame.
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent->SetMobility(EComponentMobility::Static);

#if WITH_EDITOR
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    BeginPlay()
    -> void
{
    Super::BeginPlay();

    // Server-only, mirroring the routing model: clients never plan and never need the graph.
    if (NOT HasAuthority())
    { return; }

    auto TransientEntity = UCk_Utils_EcsWorld_Subsystem_UE::Get_TransientEntity(GetWorld());

    CK_ENSURE_IF_NOT(ck::IsValid(TransientEntity),
        TEXT("PathNetwork actor [{}] could not resolve the TransientEntity for the current world"), this)
    { return; }

    auto Params = FCk_Fragment_PathNetwork_ParamsData{Get_WorldRibbons()};
    Params.Set_BuildParams(_BuildParams);

    _NetworkHandle = UCk_Utils_PathNetwork_UE::Add(TransientEntity, Params);

    if (_AutoDetectOnBeginPlay == ECk_EnableDisable::Enable && ck::IsValid(_Detector))
    {
        UCk_Utils_PathNetwork_UE::Request_RebuildFromDetector(
            _NetworkHandle, _Detector, Get_DetectionBounds(), _VectorizeParams);
    }

    ck::pathnetwork::Display(TEXT("PathNetwork actor [{}] constructed network entity [{}] ([{}] ribbons, auto-detect [{}])"),
        this, _NetworkHandle, _Ribbons.Num(), _AutoDetectOnBeginPlay);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    EndPlay(
        const EEndPlayReason::Type EndPlayReason)
    -> void
{
    if (ck::IsValid(_NetworkHandle))
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(_NetworkHandle);
        _NetworkHandle = {};
    }

    Super::EndPlay(EndPlayReason);
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    ACk_PathNetwork_UE::
    Tick(
        float InDeltaSeconds)
    -> void
{
    Super::Tick(InDeltaSeconds);

    // Draws the AUTHORED ribbons; the BUILT graph is drawn in game worlds by ck.PathNetwork.DebugDraw.
    const auto* World = GetWorld();

    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}) || World->IsGameWorld())
    { return; }

    if (NOT IsSelectedInEditor())
    { return; }

    for (const auto& Ribbon : Get_WorldRibbons())
    {
        const auto& Points = Ribbon.Get_Points();
        const auto Color = Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored
            ? FColor::Green
            : FColor::Orange;

        for (auto PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
        {
            const auto& Point = Points[PointIndex];
            DrawDebugPoint(World, Point.Get_Location(), 12.0f, Color, false, -1.0f);

            if (PointIndex == 0)
            { continue; }

            const auto& Prev = Points[PointIndex - 1];
            DrawDebugLine(World, Prev.Get_Location(), Point.Get_Location(), Color, false, -1.0f, SDPG_World, 3.0f);

            const auto Tangent = (Point.Get_Location() - Prev.Get_Location()).GetSafeNormal2D();

            if (Tangent.IsNearlyZero())
            { continue; }

            const auto Right = FVector::CrossProduct(FVector::UpVector, Tangent);
            const auto EdgeColor = Color.WithAlpha(96);

            DrawDebugLine(World,
                Prev.Get_Location() + Right * Prev.Get_HalfWidth(),
                Point.Get_Location() + Right * Point.Get_HalfWidth(),
                EdgeColor, false, -1.0f, SDPG_World, 1.0f);
            DrawDebugLine(World,
                Prev.Get_Location() - Right * Prev.Get_HalfWidth(),
                Point.Get_Location() - Right * Point.Get_HalfWidth(),
                EdgeColor, false, -1.0f, SDPG_World, 1.0f);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    ShouldTickIfViewportsOnly() const
    -> bool
{
    return true;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    Get_NetworkHandle() const
    -> FCk_Handle_PathNetwork
{
    return _NetworkHandle;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    Get_DetectionBounds() const
    -> FBox
{
    const auto Center = GetActorLocation();
    return FBox{Center - _DetectionExtents, Center + _DetectionExtents};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    Get_WorldRibbons() const
    -> TArray<FCk_PathNetwork_Ribbon>
{
    const auto& ActorTransform = GetActorTransform();

    auto WorldRibbons = _Ribbons;
    for (auto& Ribbon : WorldRibbons)
    {
        for (auto& Point : Ribbon.Get_Points())
        { Point.Set_Location(ActorTransform.TransformPosition(Point.Get_Location())); }
    }

    return WorldRibbons;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_PathNetwork_UE::
    Convert_WorldRibbonToRelative(
        const FCk_PathNetwork_Ribbon& InWorldRibbon) const
    -> FCk_PathNetwork_Ribbon
{
    const auto& ActorTransform = GetActorTransform();

    auto RelativeRibbon = InWorldRibbon;
    for (auto& Point : RelativeRibbon.Get_Points())
    { Point.Set_Location(ActorTransform.InverseTransformPosition(Point.Get_Location())); }

    return RelativeRibbon;
}

// --------------------------------------------------------------------------------------------------------------------
