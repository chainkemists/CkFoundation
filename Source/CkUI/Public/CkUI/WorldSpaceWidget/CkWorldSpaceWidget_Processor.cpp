#include "CkWorldSpaceWidget_Processor.h"

#include "Blueprint/WidgetLayoutLibrary.h"

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Math/Geometry/CkGeometry_Utils.h"

#include "Components/CanvasPanelSlot.h"

#include "Engine/GameViewportClient.h"

#include "Kismet/GameplayStatics.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_WorldSpaceWidget_UpdateLocation::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Transform InTransform,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) const
        -> void
    {
        const auto& Widget = InParams.Get_Widget().Get();

        if (ck::Is_NOT_Valid(Widget))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        const auto& LocationInfo = InParams.Get_LocationInfo();

        const auto& ProjectionWorldLocation = InTransform.Get_Transform().GetLocation() + LocationInfo.Get_WorldSpaceOffset();
        const auto& PlayerController = InCurrent.Get_WidgetOwningPlayer().Get();

        if (ck::Is_NOT_Valid(PlayerController))
        { return; }

        auto ProjectedScreenPosition = FVector2D{};
        const auto& ProjectionSuccess = UGameplayStatics::ProjectWorldToScreen(
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
            FFragment_Transform InTransform,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) const
        -> void
    {
        const auto& Widget = InParams.Get_Widget().Get();

        if (ck::Is_NOT_Valid(Widget))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        const auto& PlayerController = InCurrent.Get_WidgetOwningPlayer().Get();

        if (ck::Is_NOT_Valid(PlayerController))
        { return; }

        const auto& CameraManager = PlayerController->PlayerCameraManager;

        if (ck::Is_NOT_Valid(CameraManager))
        { return; }

        const auto& WidgetWorldLocation = InTransform.Get_Transform().GetLocation();
        const auto& WidgetWorldLocation_WithOffset = WidgetWorldLocation + InParams.Get_LocationInfo().Get_WorldSpaceOffset();

        const auto& DistanceFromTargetToViewport = FVector::Dist(CameraManager->GetCameraLocation(), WidgetWorldLocation_WithOffset);

        const auto& ScalingInfo = InParams.Get_ScalingInfo();

        const auto& WidgetScale = FMath::GetMappedRangeValueClamped(
            UE::Math::TVector2(ScalingInfo.Get_ScaleFalloff_StartDistance(), ScalingInfo.Get_ScaleFalloff_EndDistance()),
            UE::Math::TVector2(ScalingInfo.Get_MaxScale(), ScalingInfo.Get_MinScale()),
            DistanceFromTargetToViewport);

        InCurrent.Get_WrapperWidget()->Request_SetWidgetScale(FVector2D{WidgetScale, WidgetScale});
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_WorldSpaceWidget_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_WorldSpaceWidget_Params& InParams,
            FFragment_WorldSpaceWidget_Current& InCurrent) const
        -> void
    {
        if (const auto& Widget = InParams.Get_Widget();
            ck::IsValid(Widget))
        {
            Widget->RemoveFromParent();
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
