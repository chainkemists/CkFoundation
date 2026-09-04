#include "CkCrowdAgent_DiagDraw_Processor.h"

#include "CkCrowd/Agent/CkCrowdAgent_Utils.h"
#include "CkCrowd/CkCrowd_Stats.h"
#include "CkCrowd/Settings/CkCrowd_DebugSettings.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkPmg/CkPmg_Utils_DebugLines.h"
#include "CkPmg/CkPmg_Utils_DebugShapes.h"

#include "HAL/IConsoleManager.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_CrowdAgent_DiagDrawBreadcrumb);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Crowd::DiagBreadcrumbRetained"), STAT_CkCrowd_DiagDrawBreadcrumbProc, STATGROUP_CkCrowd);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_crowd_agent_diag_draw_processor
{
    static TAutoConsoleVariable<int32> CVarSelectedEntityId(
        TEXT("ck.Crowd.SelectedEntityId"),
        -1,
        TEXT("Hash id of the crowd agent currently selected in the Crowd Debugger. -1 = none.\n")
        TEXT("Draw processors render this agent regardless of the per-overlay CVars."),
        ECVF_Default);

    constexpr auto BreadcrumbLiftZ = 96.0f;
    constexpr auto Breadcrumb_Thickness = 3.0f;
    constexpr auto Breadcrumb_Thickness_Selected = 6.0f;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CrowdAgent_DiagDrawBreadcrumb::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_CrowdAgent_DiagRecorder& InRecorder,
            FFragment_CrowdAgent_DiagBreadcrumb& InBreadcrumb)
        -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_CkCrowd_DiagDrawBreadcrumbProc);

        const auto ResetGeometry = [&InBreadcrumb]()
        {
            for (auto& Chunk : InBreadcrumb._Chunks)
            {
                if (ck::IsValid(Chunk))
                { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Chunk); }
            }
            InBreadcrumb._Chunks.Reset();
            InBreadcrumb._History = crowd_diag_breadcrumb::FHistoryState{};
            InBreadcrumb._HasAppliedVisibility = false;
        };

        auto HasInvalidChunk = false;
        for (const auto& Chunk : InBreadcrumb._Chunks)
        {
            if (ck::Is_NOT_Valid(Chunk))
            {
                HasInvalidChunk = true;
                break;
            }
        }
        if (HasInvalidChunk)
        { ResetGeometry(); }

        auto UpdateRange = crowd_diag_breadcrumb::PrepareUpdate(
            InRecorder.Get_Samples().Num(),
            InRecorder._TrackGeneration,
            InRecorder._RetainedHistoryStartPos + FVector{0.0f, 0.0f, ck_crowd_agent_diag_draw_processor::BreadcrumbLiftZ},
            InBreadcrumb._History);
        if (UpdateRange._NeedsGeometryReset)
        {
            for (auto& Chunk : InBreadcrumb._Chunks)
            {
                if (ck::IsValid(Chunk))
                { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(Chunk); }
            }
            InBreadcrumb._Chunks.Reset();
            InBreadcrumb._HasAppliedVisibility = false;
        }

        const auto DrawAll = UCk_Utils_Crowd_DebugSettings_UE::Get_DrawBreadcrumbs();
        const auto SelectedHash = ck_crowd_agent_diag_draw_processor::CVarSelectedEntityId.GetValueOnGameThread();
        const auto IsSelected = SelectedHash >= 0 &&
            static_cast<int32>(GetTypeHash(InHandle)) == SelectedHash;
        const auto ShouldBeVisible = DrawAll || IsSelected;
        const auto Color = UCk_Utils_CrowdAgent_UE::Get_DebugColor(InHandle);
        const auto Thickness = IsSelected
            ? ck_crowd_agent_diag_draw_processor::Breadcrumb_Thickness_Selected
            : ck_crowd_agent_diag_draw_processor::Breadcrumb_Thickness;

        const auto VisibilityChanged = NOT InBreadcrumb._HasAppliedVisibility ||
            InBreadcrumb._LastAppliedVisibility != ShouldBeVisible;
        const auto SelectionChanged = NOT InBreadcrumb._HasAppliedVisibility ||
            InBreadcrumb._LastAppliedSelection != IsSelected;
        const auto ColorChanged = NOT InBreadcrumb._HasAppliedVisibility ||
            NOT InBreadcrumb._LastAppliedColor.Equals(Color);

        if (VisibilityChanged || SelectionChanged || ColorChanged)
        {
            for (auto& Chunk : InBreadcrumb._Chunks)
            {
                if (ck::Is_NOT_Valid(Chunk))
                { continue; }

                if (VisibilityChanged)
                { UCk_Utils_Pmg_DebugShape_UE::Request_SetVisible(Chunk, ShouldBeVisible, {}); }
                if (SelectionChanged)
                {
                    UCk_Utils_Pmg_DebugShape_UE::Request_SetLineThickness(
                        Chunk,
                        FCk_Request_Pmg_DebugShape_SetLineThickness{Thickness},
                        {});
                }
                if (ColorChanged)
                {
                    UCk_Utils_Pmg_DebugShape_UE::Request_SetColor(
                        Chunk,
                        FCk_Request_Pmg_DebugShape_SetColor{Color},
                        {});
                }
            }
        }

        InBreadcrumb._HasAppliedVisibility = true;
        InBreadcrumb._LastAppliedVisibility = ShouldBeVisible;
        InBreadcrumb._LastAppliedSelection = IsSelected;
        InBreadcrumb._LastAppliedColor = Color;

        if (NOT ShouldBeVisible)
        { return; }

        const auto Lift = FVector{0.0f, 0.0f, ck_crowd_agent_diag_draw_processor::BreadcrumbLiftZ};
        const auto& Samples = InRecorder.Get_Samples();
        for (auto SampleIndex = UpdateRange._BeginSampleIndex;
             SampleIndex < UpdateRange._EndSampleIndex;
             ++SampleIndex)
        {
            const auto Plan = crowd_diag_breadcrumb::PlanSample(
                Samples[SampleIndex].Get_Pos() + Lift,
                InBreadcrumb._History);
            if (NOT Plan._ShouldAppend)
            {
                InBreadcrumb._History = Plan._NextState;
                continue;
            }

            if (Plan._ShouldStartChunk)
            {
                auto Owner = static_cast<FCk_Handle>(InHandle);
                auto Chunk = pmg::Create_DebugLineSet(
                    Owner,
                    FTransform{FRotator::ZeroRotator, Plan._Start, FVector::OneVector},
                    Color,
                    Thickness,
                    ECk_Pmg_RenderMode::DoubleSided);
                const auto ChunkIsValid = ck::IsValid(Chunk);
                CK_ENSURE_IF_NOT(ChunkIsValid,
                    TEXT("Cannot create retained CrowdDiag breadcrumb chunk for agent [{}]"),
                    InHandle)
                { return; }

                InBreadcrumb._Chunks.Add(Chunk);
                if (Plan._ShouldEvictOldestChunk)
                {
                    auto& OldestChunk = InBreadcrumb._Chunks[0];
                    if (ck::IsValid(OldestChunk))
                    { UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(OldestChunk); }
                    InBreadcrumb._Chunks.RemoveAt(0);
                }
            }

            const auto HasActiveChunk = NOT InBreadcrumb._Chunks.IsEmpty() &&
                                        ck::IsValid(InBreadcrumb._Chunks.Last());
            CK_ENSURE_IF_NOT(HasActiveChunk,
                TEXT("Cannot append CrowdDiag breadcrumb for agent [{}] without a live PMG chunk"),
                InHandle)
            { return; }

            pmg::Append_DebugLine_World(
                InBreadcrumb._Chunks.Last(),
                Plan._Start,
                Plan._End,
                Color,
                Thickness);
            InBreadcrumb._History = Plan._NextState;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
