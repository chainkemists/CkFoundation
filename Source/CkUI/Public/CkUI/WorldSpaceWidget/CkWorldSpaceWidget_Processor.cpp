#include "CkWorldSpaceWidget_Processor.h"

#include "Components/CanvasPanelSlot.h"

#include "CollisionQueryParams.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Kismet/GameplayStatics.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_UpdateLocation);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_UpdateScaling);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_EndPlay);

// --------------------------------------------------------------------------------------------------------------------

// Mirrors the legacy WorldSpaceWidgets plugin: trace camera -> widget anchor on
// the configured channel (player pawn ignored); a blocking hit means occluded.
static auto
DoIsAnchorOccluded(
    const APlayerController* InPlayerController,
    const FVector& InAnchorWorldLocation,
    const FCk_WorldSpaceWidget_OcclusionInfo& InOcclusionInfo)
    -> bool
{
    if (ck::Is_NOT_Valid(InPlayerController, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    const auto CameraManager = InPlayerController->PlayerCameraManager;
    if (ck::Is_NOT_Valid(CameraManager))
    { return false; }

    const auto World = InPlayerController->GetWorld();
    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return false; }

    auto QueryParams = FCollisionQueryParams{FName{TEXT("CkWorldSpaceWidgetOcclusion")}, true};
    if (const auto PlayerPawn = InPlayerController->GetPawn();
        ck::IsValid(PlayerPawn))
    { QueryParams.AddIgnoredActor(PlayerPawn); }

    auto Hit = FHitResult{};
    World->LineTraceSingleByChannel(
        Hit,
        CameraManager->GetCameraLocation(),
        InAnchorWorldLocation,
        InOcclusionInfo.Get_TraceChannel().GetValue(),
        QueryParams);

    return Hit.IsValidBlockingHit();
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_WorldSpaceWidget_UpdateLocation::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent)
        -> void
    {
        // WorldComponent mode: the widget IS a 3D component — drive its world
        // transform from the entity (no screen projection / billboarding).
        if (InParams.Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::WorldComponent)
        {
            const auto WidgetComponent = InCurrent.Get_WidgetComponent().Get();

            if (ck::Is_NOT_Valid(WidgetComponent, ck::IsValid_Policy_NullptrOnly{}))
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
                return;
            }

            auto WorldTransform = InTransform.Get_Transform();
            WorldTransform.AddToTranslation(InParams.Get_LocationInfo().Get_WorldSpaceOffset());
            WidgetComponent->SetWorldTransform(WorldTransform);
            return;
        }

        auto Widget = InCurrent.Get_WrapperWidget().Get();

        if (ck::Is_NOT_Valid(Widget))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        const auto& LocationInfo = InParams.Get_LocationInfo();
        const auto ProjectionWorldLocation = InTransform.Get_Transform().GetLocation() + LocationInfo.Get_WorldSpaceOffset();
        auto PlayerController = InCurrent.Get_WidgetOwningPlayer().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(PlayerController),
            TEXT("Invalid PlayerController. Unable to Project [{}] at World Location [{}]"),
            InParams.Get_Widget(), ProjectionWorldLocation)
        { return; }

        // Visibility rides RenderOpacity: enabled-state, then the per-frame
        // occlusion test forces it to zero when the anchor is blocked.
        auto TargetOpacity = InCurrent.Get_Enabled() ? 1.0f : 0.0f;

        if (TargetOpacity > 0.0f &&
            InParams.Get_OcclusionInfo().Get_OcclusionPolicy() == ECk_WorldSpaceWidget_Occlusion_Policy::HideWhenOccluded &&
            DoIsAnchorOccluded(PlayerController, ProjectionWorldLocation, InParams.Get_OcclusionInfo()))
        {
            TargetOpacity = 0.0f;
        }

        if (Widget->GetRenderOpacity() != TargetOpacity)
        { Widget->SetRenderOpacity(TargetOpacity); }

        auto ProjectedScreenPosition = FVector2D{};
        const auto ProjectionSuccess = UGameplayStatics::ProjectWorldToScreen(
            PlayerController,
            ProjectionWorldLocation,
            ProjectedScreenPosition);

        if (NOT ProjectionSuccess)
        { return; }

        auto ScreenPosition = ProjectedScreenPosition + LocationInfo.Get_ScreenSpaceOffset();

        if (LocationInfo.Get_ClampingPolicy() == ECk_WorldSpaceWidget_Clamping_Policy::ClampToViewport)
        {
            auto ViewportSize = FVector2D{};
            GEngine->GameViewport->GetViewportSize(ViewportSize);

            const auto ViewportRect = FVector4(0, 0, ViewportSize.X, ViewportSize.Y);

            ScreenPosition.X = FMath::Clamp(ScreenPosition.X, ViewportRect.X, ViewportRect.Z);
            ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, ViewportRect.Y, ViewportRect.W);
        }

        Widget->SetPositionInViewport(ScreenPosition);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_WorldSpaceWidget_UpdateScaling::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform,
            const FFragment_WorldSpaceWidget_Params& InParams,
            const FFragment_WorldSpaceWidget_Current& InCurrent)
        -> void
    {
        // WorldComponent widgets are 3D objects — perspective scaling is native.
        if (InParams.Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::WorldComponent)
        { return; }

        if (const auto Widget = InParams.Get_Widget().Get();
            ck::Is_NOT_Valid(Widget))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        auto PlayerController = InCurrent.Get_WidgetOwningPlayer().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(PlayerController), TEXT("Invalid PlayerController"))
        { return; }

        auto CameraManager = PlayerController->PlayerCameraManager;
        CK_ENSURE_IF_NOT(ck::IsValid(CameraManager), TEXT("Invalid CameraManager"))
        { return; }

        const auto WidgetWorldLocation = InTransform.Get_Transform().GetLocation();
        const auto WidgetWorldLocation_WithOffset = WidgetWorldLocation + InParams.Get_LocationInfo().Get_WorldSpaceOffset();

        const auto DistanceFromTargetToViewport = FVector::Dist(CameraManager->GetCameraLocation(), WidgetWorldLocation_WithOffset);

        const auto ScalingInfo = InParams.Get_ScalingInfo();

        const auto WidgetScale = FMath::GetMappedRangeValueClamped(
            UE::Math::TVector2(ScalingInfo.Get_ScaleFalloff_StartDistance(), ScalingInfo.Get_ScaleFalloff_EndDistance()),
            UE::Math::TVector2(ScalingInfo.Get_MaxScale(), ScalingInfo.Get_MinScale()),
            DistanceFromTargetToViewport);

        InCurrent.Get_WrapperWidget()->Request_SetWidgetScale(FVector2D{WidgetScale, WidgetScale});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_WorldSpaceWidget_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent)
        -> void
    {
        if (const auto WidgetComponent = InCurrent.Get_WidgetComponent().Get();
            ck::IsValid(WidgetComponent, ck::IsValid_Policy_NullptrOnly{}))
        {
            WidgetComponent->DestroyComponent();
        }

        if (auto Widget = InParams.Get_Widget();
            ck::IsValid(Widget))
        {
            Widget->RemoveFromParent();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
