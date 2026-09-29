#include "CkSway_DebugDraw_Processor.h"

#include "CkSway/CkSway_Utils.h"
#include "CkSway/Settings/CkSway_DebugSettings.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Sway_DebugDraw);

namespace ck
{
    auto
        FProcessor_Sway_DebugDraw::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_Sway_DebugSettings_UE::Get_DrawOffsets())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Sway_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Sway_Tunables& InTunables,
            const FFragment_Sway& InSway)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto SceneNode = UCk_Utils_SceneNode_UE::Cast(InHandle);
        const auto Node = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SceneNode) || ck::Is_NOT_Valid(Node))
        { return; }

        const auto Color = InHandle.Has<FTag_Sway_Disabled>()
            ? FLinearColor::Red
            : (InHandle.Has<FTag_Sway_ForeignOffsetReported>() ? FLinearColor::Yellow : FLinearColor::Green);

        const auto DriverWorld = UCk_Utils_SceneNode_UE::Get_DriverWorldTransform(SceneNode);
        const auto NodeLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Node);

        UCk_Utils_DebugDraw_UE::DrawDebugCoordinateSystem(World, DriverWorld.GetLocation(), DriverWorld.Rotator(), 10.0f, 0.0f, 1.0f);
        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, DriverWorld.GetLocation(), NodeLocation, Color, 0.0f, 1.5f);
        UCk_Utils_DebugDraw_UE::DrawDebugPoint(World, NodeLocation, 3.0f, Color, 0.0f);

        const auto Location = UCk_Utils_Sway_UE::Get_LocationOffset(InHandle);
        const auto Rotation = UCk_Utils_Sway_UE::Get_RotationOffset(InHandle);
        const auto Stimulus = UCk_Utils_Sway_UE::Get_LastStimulus(InHandle);
        const auto& AngularVelocity = Stimulus.Get_AngularVelocityDeg();
        const auto& LinearVelocity = Stimulus.Get_LinearVelocity();

        const auto Text = ck::Format_UE(
            TEXT("Loc ({:.2f},{:.2f},{:.2f}) Rot ({:.2f},{:.2f},{:.2f}) | w ({:.1f},{:.1f},{:.1f}) v ({:.1f},{:.1f},{:.1f})"),
            Location.X, Location.Y, Location.Z,
            Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
            AngularVelocity.X, AngularVelocity.Y, AngularVelocity.Z,
            LinearVelocity.X, LinearVelocity.Y, LinearVelocity.Z);
        UCk_Utils_DebugDraw_UE::DrawDebugString(World, NodeLocation, Text, Color, 0.0f);
    }
}
