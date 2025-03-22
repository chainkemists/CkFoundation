#include "CkWorldSpaceWidget_Processor.h"

#include "CkUI/CkScreen_Utils.h"

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

        const auto& ProjectionWorldLocation = InTransform.Get_Transform().GetLocation() + InParams.Get_WorldSpaceOffset();
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

        const auto ScreenPosition = ProjectedScreenPosition + InParams.Get_ScreenSpaceOffset();

        Widget->SetPositionInViewport(ScreenPosition);
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
