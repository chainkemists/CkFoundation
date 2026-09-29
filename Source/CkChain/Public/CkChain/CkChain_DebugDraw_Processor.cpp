#include "CkChain_DebugDraw_Processor.h"

#include "CkChain/CkChainLink_Utils.h"
#include "CkChain/Settings/CkChain_DebugSettings.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_Chain_DebugDraw);

namespace ck
{
    auto
        FProcessor_Chain_DebugDraw::
        DoTick(FCk_Time InDeltaT)
        -> void
    {
        if (NOT UCk_Utils_Chain_DebugSettings_UE::Get_DrawHistory() &&
            NOT UCk_Utils_Chain_DebugSettings_UE::Get_DrawLinkTargets())
        {
            _LastVisitedCount = 0;
            return;
        }

        TProcessor::DoTick(InDeltaT);
    }

    auto
        FProcessor_Chain_DebugDraw::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Chain_Params& InParams,
            const FFragment_Chain& InCurrent)
        -> void
    {
        auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& History = InCurrent.Get_History();
        if (UCk_Utils_Chain_DebugSettings_UE::Get_DrawHistory())
        {
            for (auto Index = 0; Index < History.Get_NumSamples(); ++Index)
            {
                const auto Location = History.Get_Sample(Index).Get_Location();
                UCk_Utils_DebugDraw_UE::DrawDebugPoint(World, Location, 5.0f, FLinearColor{0.0f, 0.8f, 0.8f}, 0.0f);
                if (Index > 0)
                {
                    UCk_Utils_DebugDraw_UE::DrawDebugLine(World,
                        History.Get_Sample(Index - 1).Get_Location(), Location,
                        FLinearColor{0.0f, 0.8f, 0.8f}, 0.0f, 1.5f);
                }
            }
        }

        if (NOT UCk_Utils_Chain_DebugSettings_UE::Get_DrawLinkTargets())
        { return; }

        for (const auto& Link : InCurrent.Get_Links())
        {
            if (ck::Is_NOT_Valid(Link))
            { continue; }

            const auto& LinkState = Link.Get<FFragment_ChainLink>();
            const auto Held = UCk_Utils_ChainLink_UE::Get_IsHeld(Link);
            const auto Location = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(
                UCk_Utils_Transform_UE::CastChecked(Link));
            if (Held || NOT LinkState.Get_HasTargetPose())
            {
                UCk_Utils_DebugDraw_UE::DrawDebugPoint(World, Location, 12.0f, FLinearColor::Gray, 0.0f);
                continue;
            }

            const auto Target = UCk_Utils_ChainLink_UE::Get_TargetPose(Link).GetLocation();
            const auto Color = FVector::Dist(Location, Target) < 1.0 ? FLinearColor::Green : FLinearColor{1.0f, 0.65f, 0.0f};
            UCk_Utils_DebugDraw_UE::DrawDebugArrow(World, Location, Target, 15.0f, Color, 0.0f, 2.0f);
        }
    }
}
