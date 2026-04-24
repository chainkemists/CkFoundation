#include "CkTransform_EditorProcessor.h"

#if WITH_EDITOR

#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Transform_Preview_EditorTime);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Transform_Preview_EditorTime::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Transform& InTransform)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        constexpr auto AxisLength = 50.0f;
        constexpr auto AxisThickness = 2.0f;
        constexpr auto DrawAxisCones = true;
        constexpr auto ConeSize = 6.0f;
        constexpr auto Duration = 0.0f;

        UCk_Utils_DebugDraw_UE::DrawDebugTransformGizmo(
            World,
            InTransform.Get_Transform(),
            AxisLength,
            AxisThickness,
            DrawAxisCones,
            ConeSize,
            Duration);
    }
}

#endif

// --------------------------------------------------------------------------------------------------------------------
