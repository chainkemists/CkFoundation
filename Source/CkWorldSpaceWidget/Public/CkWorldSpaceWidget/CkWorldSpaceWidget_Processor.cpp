#include "CkWorldSpaceWidget_Processor.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "Blueprint/WidgetLayoutLibrary.h"

#include "Components/CanvasPanelSlot.h"

#include "CollisionQueryParams.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "Kismet/GameplayStatics.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Utils.h"

#include "CkWorldSpaceWidget/CkWorldSpaceWidget_Stats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("UI::WSWidget Projection"), STAT_CkWorldSpaceWidget_Projection, STATGROUP_CkWorldSpaceWidget);
DECLARE_CYCLE_STAT(TEXT("UI::WSWidget Occlusion"),  STAT_CkWorldSpaceWidget_Occlusion,  STATGROUP_CkWorldSpaceWidget);
DECLARE_CYCLE_STAT(TEXT("UI::WSWidget UMGWrite"),   STAT_CkWorldSpaceWidget_UMGWrite,   STATGROUP_CkWorldSpaceWidget);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_UpdateLocation);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_UpdateScaling);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_WorldSpaceWidget_EndPlay);

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
        if (InParams.Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::WorldComponent)
        {
            const auto WidgetComponent = InCurrent.Get_WidgetComponent().Get();

            if (ck::Is_NOT_Valid(WidgetComponent))
            {
                UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
                return;
            }

            auto WorldTransform = InTransform.Get_Transform();
            WorldTransform.AddToTranslation(InParams.Get_LocationInfo().Get_WorldSpaceOffset());
            WidgetComponent->SetWorldTransform(WorldTransform);
            return;
        }

        const auto WrapperWidget = InCurrent.Get_WrapperWidget().Get();

        if (ck::Is_NOT_Valid(WrapperWidget))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        // A disabled wrapper is Collapsed — projection, occlusion tracing and the UMG writes
        // below would all be spent on something that neither lays out nor paints.
        if (NOT InCurrent.Get_Enabled())
        { return; }

        const auto& LocationInfo = InParams.Get_LocationInfo();
        const auto AnchorWorldLocation = InTransform.Get_Transform().GetLocation() + LocationInfo.Get_WorldSpaceOffset();
        const auto PlayerController = InCurrent.Get_WidgetOwningPlayer().Get();

        CK_ENSURE_IF_NOT(ck::IsValid(PlayerController),
            TEXT("Invalid PlayerController. Unable to Project [{}] at World Location [{}]"),
            InParams.Get_Widget(), AnchorWorldLocation)
        { return; }

        const auto CameraManager = PlayerController->PlayerCameraManager;
        const auto CameraLocation = ck::IsValid(CameraManager)
            ? CameraManager->GetCameraLocation()
            : AnchorWorldLocation;
        const auto DistanceToCamera = FVector::Dist(CameraLocation, AnchorWorldLocation);

        const auto& OcclusionInfo = InParams.Get_OcclusionInfo();
        const auto IsOccluded = [&]() -> bool
        {
            SCOPE_CYCLE_COUNTER(STAT_CkWorldSpaceWidget_Occlusion);
            return OcclusionInfo.Get_OcclusionPolicy() == ECk_WorldSpaceWidget_Occlusion_Policy::HideWhenOccluded &&
                   UCk_Utils_WorldSpaceWidget_UE::Get_IsAnchorOccluded(InHandle);
        }();

        const auto& FadingInfo = InParams.Get_FadingInfo();
        const auto FadeFactor = FadingInfo.Get_FadingPolicy() == ECk_WorldSpaceWidget_Fading_Policy::FadeWithDistance
            ? FMath::GetMappedRangeValueClamped(
                UE::Math::TVector2(FadingInfo.Get_FadeFalloff_StartDistance(), FadingInfo.Get_FadeFalloff_EndDistance()),
                UE::Math::TVector2(FadingInfo.Get_MaxOpacity(), FadingInfo.Get_MinOpacity()),
                static_cast<float>(DistanceToCamera))
            : 1.0f;

        auto ProjectedScreenPosition = FVector2D{};
        auto ProjectionSuccess = false;
        auto ViewportSize = FVector2D::ZeroVector;
        const auto HasViewport = ck::IsValid(GEngine) && GEngine->GameViewport != nullptr;
        {
            SCOPE_CYCLE_COUNTER(STAT_CkWorldSpaceWidget_Projection);

            ProjectionSuccess = UGameplayStatics::ProjectWorldToScreen(
                PlayerController,
                AnchorWorldLocation,
                ProjectedScreenPosition);

            if (HasViewport)
            { GEngine->GameViewport->GetViewportSize(ViewportSize); }
        }

        auto AppliedScreenOffset = LocationInfo.Get_ScreenSpaceOffset();
        {
            const auto& OffsetScaling = LocationInfo.Get_ScreenOffsetScaling();

            if (OffsetScaling.Get_AspectScalingPolicy() == ECk_WorldSpaceWidget_AspectScaling_Policy::ScaleXByAspectRatio &&
                HasViewport && ViewportSize.Y > KINDA_SMALL_NUMBER)
            {
                constexpr auto StandardAspect = 16.0f / 9.0f;
                const auto CurrentAspect = static_cast<float>(ViewportSize.X / ViewportSize.Y);
                const auto AspectScaleX = FMath::Clamp(CurrentAspect / StandardAspect,
                    OffsetScaling.Get_AspectRatio_MinScale(), OffsetScaling.Get_AspectRatio_MaxScale());
                AppliedScreenOffset.X *= AspectScaleX;
            }

            if (OffsetScaling.Get_DistanceScalingPolicy() == ECk_WorldSpaceWidget_OffsetDistanceScaling_Policy::ScaleWithDistance)
            {
                const auto FalloffRange = UE::Math::TVector2<float>(
                    OffsetScaling.Get_DistanceFalloff_StartDistance(), OffsetScaling.Get_DistanceFalloff_EndDistance());

                const auto OffsetScaleX = FMath::GetMappedRangeValueClamped(FalloffRange,
                    UE::Math::TVector2<float>(static_cast<float>(OffsetScaling.Get_DistanceScale_Max().X), static_cast<float>(OffsetScaling.Get_DistanceScale_Min().X)),
                    static_cast<float>(DistanceToCamera));
                const auto OffsetScaleY = FMath::GetMappedRangeValueClamped(FalloffRange,
                    UE::Math::TVector2<float>(static_cast<float>(OffsetScaling.Get_DistanceScale_Max().Y), static_cast<float>(OffsetScaling.Get_DistanceScale_Min().Y)),
                    static_cast<float>(DistanceToCamera));

                AppliedScreenOffset.X *= OffsetScaleX;
                AppliedScreenOffset.Y *= OffsetScaleY;
            }
        }

        // ProjectWorldToScreen yields raw viewport pixels, and SetPositionInViewport below divides
        // the whole position back out by the DPI scale — so whatever we add here lands on screen as
        // that same pixel count, DPI-invariant. A DesignSpace offset therefore has to be converted
        // INTO pixels first, using the exact call SetPositionInViewport's divide makes so the two
        // cancel to the authored value with no residue.
        //
        // Guarded on > 0: GetViewportScale returns 0 with no viewport/world, and an unguarded
        // multiply would silently collapse the offset to zero rather than leaving it alone.
        if (LocationInfo.Get_ScreenSpaceOffset_Space() == ECk_WorldSpaceWidget_ScreenOffset_Space::DesignSpace)
        {
            if (const auto DpiScale = UWidgetLayoutLibrary::GetViewportScale(WrapperWidget);
                DpiScale > 0.0f)
            { AppliedScreenOffset *= DpiScale; }
        }

        auto ScreenPosition = ProjectedScreenPosition + AppliedScreenOffset;

        const auto ClampingPolicy = LocationInfo.Get_ClampingPolicy();
        const auto ShouldClamp = ClampingPolicy != ECk_WorldSpaceWidget_Clamping_Policy::None;

        auto ViewportRect = FVector4(0.0, 0.0, ViewportSize.X, ViewportSize.Y);
        if (ClampingPolicy == ECk_WorldSpaceWidget_Clamping_Policy::ClampToViewport_ByBounds)
        {
            if (const auto DesiredSize = WrapperWidget->GetDesiredSize();
                DesiredSize.SizeSquared() > KINDA_SMALL_NUMBER)
            {
                ViewportRect += FVector4(DesiredSize.X, DesiredSize.Y, -DesiredSize.X, -DesiredSize.Y);
            }
        }

        const auto IsOffScreen = HasViewport &&
            (ScreenPosition.X < ViewportRect.X || ScreenPosition.Y < ViewportRect.Y ||
             ScreenPosition.X >= ViewportRect.Z || ScreenPosition.Y >= ViewportRect.W);

        auto TargetOpacity = InCurrent.Get_Enabled() ? FadeFactor : 0.0f;
        if (IsOccluded)
        { TargetOpacity = 0.0f; }
        if (NOT ShouldClamp && (IsOffScreen || NOT ProjectionSuccess))
        { TargetOpacity = 0.0f; }

        {
            SCOPE_CYCLE_COUNTER(STAT_CkWorldSpaceWidget_UMGWrite);

            if (WrapperWidget->GetRenderOpacity() != TargetOpacity)
            { WrapperWidget->SetRenderOpacity(TargetOpacity); }

            if (ShouldClamp)
            {
                ScreenPosition.X = FMath::Clamp(ScreenPosition.X, ViewportRect.X, ViewportRect.Z);
                ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, ViewportRect.Y, ViewportRect.W);
            }

            WrapperWidget->SetPositionInViewport(ScreenPosition);
        }
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

        // Same reasoning as UpdateLocation: nothing downstream is observable while Collapsed.
        if (NOT InCurrent.Get_Enabled())
        { return; }

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
        FProcessor_WorldSpaceWidget_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Requests& InRequests) const
        -> void
    {
        const auto RequestsCopy = InRequests._Requests;
        InRequests._Requests.Reset();

        algo::ForEachRequest(RequestsCopy, ck::Visitor(
        [&](const auto& InRequest) -> void
        {
            // Every DoHandleRequest overload below is void and has no rejection path, so reaching the
            // line after the call IS the success condition.
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

            DoHandleRequest(InHandle, InCurrent, InParams, InRequest);

            Result = ECk_Request_OperationResult::Succeeded;

            if (InRequest.Get_IsRequestHandleValid())
            { InRequest.GetAndDestroyRequestHandle(); }
        }), policy::DontResetContainer{});

        if (InRequests._Requests.IsEmpty())
        { InHandle.Remove<MarkedDirtyBy>(); }
    }

    auto
        FProcessor_WorldSpaceWidget_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetLocationInfo& InRequest)
        -> void
    {
        InParams.Set_LocationInfo(InRequest.Get_LocationInfo());
    }

    auto
        FProcessor_WorldSpaceWidget_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetScalingInfo& InRequest)
        -> void
    {
        InParams.Set_ScalingInfo(InRequest.Get_ScalingInfo());

        const auto ScalingEnabled =
            InParams.Get_RenderMode() == ECk_WorldSpaceWidget_RenderMode::ScreenOverlay &&
            InRequest.Get_ScalingInfo().Get_ScalingPolicy() == ECk_WorldSpaceWidget_Scaling_Policy::ScaleWithDistance;

        if (ScalingEnabled)
        {
            InHandle.AddOrGet<FTag_WorldSpaceWidget_NeedsUpdateScaling>();
            return;
        }

        InHandle.Remove<FTag_WorldSpaceWidget_NeedsUpdateScaling>();

        // Reset the scale box so it does not stay stuck at the last distance-driven scale.
        if (const auto WrapperWidget = InCurrent.Get_WrapperWidget().Get();
            ck::IsValid(WrapperWidget))
        { WrapperWidget->Request_SetWidgetScale(FVector2D{1.0, 1.0}); }
    }

    auto
        FProcessor_WorldSpaceWidget_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetFadingInfo& InRequest)
        -> void
    {
        InParams.Set_FadingInfo(InRequest.Get_FadingInfo());
    }

    auto
        FProcessor_WorldSpaceWidget_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Current& InCurrent,
            FFragment_WorldSpaceWidget_Params& InParams,
            const FCk_Request_WorldSpaceWidget_SetOcclusionInfo& InRequest)
        -> void
    {
        InParams.Set_OcclusionInfo(InRequest.Get_OcclusionInfo());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_WorldSpaceWidget_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_WorldSpaceWidget_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
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
            ck::IsValid(WidgetComponent))
        {
            // unpin before DestroyComponent (destroy garbage-marks the object, failing release validity)
            UCk_Utils_Object_UE::TryReleaseToPool(WidgetComponent);
            WidgetComponent->DestroyComponent();
        }

        // The WRAPPER, not the content widget, is what Request_WrapWidget added to the viewport;
        // removing it takes its content child with it.
        if (const auto WrapperWidget = InCurrent.Get_WrapperWidget().Get();
            ck::IsValid(WrapperWidget))
        {
            WrapperWidget->RemoveFromParent();

            // unpin the subsystem hold (no destroy needed — GC collects once RemoveFromParent drops the viewport ref)
            UCk_Utils_Object_UE::TryReleaseToPool(WrapperWidget);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
