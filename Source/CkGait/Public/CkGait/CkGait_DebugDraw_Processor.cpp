#include "CkGait_DebugDraw_Processor.h"

#include "CkGait/Bob/CkBob_Utils.h"
#include "CkGait/Gait/CkGait_Utils.h"
#include "CkGait/Settings/CkGait_DebugSettings.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkCore/Format/CkFormat.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/SceneNode/CkSceneNode_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Gait_DebugDraw);

namespace ck
{
    auto
        FProcessor_Gait_DebugDraw::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_Gait_DebugSettings_UE::Get_DrawBobs())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Gait_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Bob_Tunables& InTunables,
            const FFragment_Bob& InBob)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto SceneNode = UCk_Utils_SceneNode_UE::Cast(InHandle);
        const auto Node = UCk_Utils_Transform_UE::Cast(InHandle);
        if (ck::Is_NOT_Valid(SceneNode) || ck::Is_NOT_Valid(Node))
        { return; }

        const auto Color = InHandle.Has<FTag_Bob_Disabled>()
            ? FLinearColor::Red
            : (InHandle.Has<FTag_Bob_ForeignOffsetReported>() ? FLinearColor::Yellow : FLinearColor::Green);

        const auto RestWorld = InBob.Get_RestOffset() * UCk_Utils_SceneNode_UE::Get_DriverWorldTransform(SceneNode);
        const auto NodeWorld = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(Node);

        UCk_Utils_DebugDraw_UE::DrawDebugCoordinateSystem(World, NodeWorld.GetLocation(), NodeWorld.Rotator(), 10.0f, 0.0f, 1.0f);
        UCk_Utils_DebugDraw_UE::DrawDebugLine(World, RestWorld.GetLocation(), NodeWorld.GetLocation(), Color, 0.0f, 1.5f);
        UCk_Utils_DebugDraw_UE::DrawDebugPoint(World, NodeWorld.GetLocation(), 3.0f, Color, 0.0f);

        const auto& Gait = InTunables.Get_Gait();
        const auto GaitText = (ck::IsValid(Gait) && UCk_Utils_Gait_UE::Has(Gait))
            ? ck::Format_UE(TEXT("Gait phase {:.2f} A {:.2f}"), UCk_Utils_Gait_UE::Get_Phase(Gait), UCk_Utils_Gait_UE::Get_Amount(Gait))
            : FString{TEXT("gait -")};

        const auto BobOffset = UCk_Utils_Bob_UE::Get_BobOffset(InHandle);
        const auto Location = BobOffset.GetLocation();
        const auto Rotation = BobOffset.Rotator();

        const auto Text = ck::Format_UE(
            TEXT("{} | Bob loc ({:.2f},{:.2f},{:.2f}) rot ({:.1f},{:.1f},{:.1f}) spring {:.2f} land#{}"),
            GaitText,
            Location.X, Location.Y, Location.Z,
            Rotation.Roll, Rotation.Pitch, Rotation.Yaw,
            UCk_Utils_Bob_UE::Get_SpringOffset(InHandle),
            InBob.Get_ConsumedLandingCount());
        UCk_Utils_DebugDraw_UE::DrawDebugString(World, NodeWorld.GetLocation(), Text, Color, 0.0f);
    }
}
