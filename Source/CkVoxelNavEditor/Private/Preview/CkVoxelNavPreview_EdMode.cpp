#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EdMode.h"

#include "CkVoxelNavEditor/Preview/CkVoxelNavPreview_EditorSubsystem.h"

#include "CkVoxelNav/Debug/CkVoxelNav_DebugSnapshot.h"

#include <Editor.h>
#include <EditorModeManager.h>
#include <Engine/Canvas.h>
#include <Engine/Engine.h>
#include <CanvasItem.h>
#include <PrimitiveDrawInterface.h>
#include <SceneManagement.h>
#include <Textures/SlateIcon.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "Ck_VoxelNavPreview_EdMode"

// --------------------------------------------------------------------------------------------------------------------

const FEditorModeID UCk_VoxelNavPreview_EdMode::EM_CkVoxelNavPreviewModeId = TEXT("Ck.VoxelNav.Preview");

// --------------------------------------------------------------------------------------------------------------------

namespace ck_voxelnav_preview_edmode
{
    constexpr auto MaxImmediateBoxesPerLayer = 10000;

    auto Get_StatusRank(ck::voxelnav::EDebugSnapshotStatus InStatus) -> int32
    {
        using ck::voxelnav::EDebugSnapshotStatus;
        switch (InStatus)
        {
        case EDebugSnapshotStatus::Failed: return 6;
        case EDebugSnapshotStatus::MissingCook: return 5;
        case EDebugSnapshotStatus::StaleCook: return 4;
        case EDebugSnapshotStatus::Building: return 3;
        case EDebugSnapshotStatus::RuntimeOnly: return 2;
        case EDebugSnapshotStatus::Current: return 1;
        default: return 0;
        }
    }

    auto Get_StatusLabel(ck::voxelnav::EDebugSnapshotStatus InStatus) -> const TCHAR*
    {
        using ck::voxelnav::EDebugSnapshotStatus;
        switch (InStatus)
        {
        case EDebugSnapshotStatus::MissingCook: return TEXT("MISSING COOK");
        case EDebugSnapshotStatus::StaleCook: return TEXT("STALE COOK");
        case EDebugSnapshotStatus::Building: return TEXT("BUILDING");
        case EDebugSnapshotStatus::Current: return TEXT("CURRENT");
        case EDebugSnapshotStatus::Failed: return TEXT("FAILED");
        case EDebugSnapshotStatus::RuntimeOnly: return TEXT("RUNTIME ONLY");
        default: return TEXT("UNKNOWN");
        }
    }

    auto Get_StatusColor(ck::voxelnav::EDebugSnapshotStatus InStatus) -> FLinearColor
    {
        using ck::voxelnav::EDebugSnapshotStatus;
        switch (InStatus)
        {
        case EDebugSnapshotStatus::Current: return FLinearColor(0.20f, 0.85f, 1.00f, 0.90f);
        case EDebugSnapshotStatus::Building: return FLinearColor(0.70f, 0.35f, 1.00f, 0.85f);
        case EDebugSnapshotStatus::StaleCook: return FLinearColor(1.00f, 0.55f, 0.05f, 0.85f);
        case EDebugSnapshotStatus::MissingCook:
        case EDebugSnapshotStatus::Failed: return FLinearColor(1.00f, 0.10f, 0.10f, 0.90f);
        default: return FLinearColor(0.55f, 0.55f, 0.55f, 0.80f);
        }
    }

    auto Get_CanDrawRetainedGeometry(ck::voxelnav::EDebugSnapshotStatus InStatus) -> bool
    {
        using ck::voxelnav::EDebugSnapshotStatus;
        return InStatus == EDebugSnapshotStatus::Current ||
            InStatus == EDebugSnapshotStatus::Building ||
            InStatus == EDebugSnapshotStatus::StaleCook;
    }

    auto
    DrawLine(
        FPrimitiveDrawInterface& InPdi,
        const FVector& InFrom,
        const FVector& InTo,
        const FLinearColor& InColor,
        float InThickness) -> void
    {
        InPdi.DrawLine(InFrom, InTo, InColor, SDPG_World, InThickness);
    }

    auto
    DrawBounds(
        FPrimitiveDrawInterface& InPdi,
        const FBox& InBounds,
        const FLinearColor& InColor,
        float InThickness) -> void
    {
        if (InBounds.IsValid == 0)
        { return; }

        const FVector Corners[8] =
        {
            {InBounds.Min.X, InBounds.Min.Y, InBounds.Min.Z}, {InBounds.Max.X, InBounds.Min.Y, InBounds.Min.Z},
            {InBounds.Max.X, InBounds.Max.Y, InBounds.Min.Z}, {InBounds.Min.X, InBounds.Max.Y, InBounds.Min.Z},
            {InBounds.Min.X, InBounds.Min.Y, InBounds.Max.Z}, {InBounds.Max.X, InBounds.Min.Y, InBounds.Max.Z},
            {InBounds.Max.X, InBounds.Max.Y, InBounds.Max.Z}, {InBounds.Min.X, InBounds.Max.Y, InBounds.Max.Z}
        };
        constexpr int32 Edges[12][2] =
        {
            {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6},
            {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
        };
        for (const auto& Edge : Edges)
        { DrawLine(InPdi, Corners[Edge[0]], Corners[Edge[1]], InColor, InThickness); }
    }

    auto
    DrawLayer(
        FPrimitiveDrawInterface& InPdi,
        const ck::voxelnav::FDebugSnapshotLayerOutput& InLayer,
        const FLinearColor& InColor,
        int32& InOutRemainingBoxes) -> void
    {
        const auto Count = FMath::Min3(InLayer._Cells.Num(), InOutRemainingBoxes, MaxImmediateBoxesPerLayer);
        for (auto Index = 0; Index < Count; ++Index)
        { DrawBounds(InPdi, InLayer._Cells[Index]._Bounds, InColor, 1.0f); }
        InOutRemainingBoxes -= Count;
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCk_VoxelNavPreview_EdMode::UCk_VoxelNavPreview_EdMode()
{
    Info = FEditorModeInfo(
        EM_CkVoxelNavPreviewModeId,
        LOCTEXT("CkVoxelNavPreviewMode", "VoxelNav Preview"),
        FSlateIcon(),
        /*bVisibleInUI*/ false);
}

auto
    UCk_VoxelNavPreview_EdMode::
    Enter()
    -> void
{
    Super::Enter();

    if (auto* PreviewSubsystem = UCk_VoxelNavPreview_EditorSubsystem_UE::Get())
    { PreviewSubsystem->Request_RebuildAll(); }
}

auto
    UCk_VoxelNavPreview_EdMode::
    Render(
        const FSceneView* InView,
        FViewport* InViewport,
        FPrimitiveDrawInterface* InPdi)
    -> void
{
    Super::Render(InView, InViewport, InPdi);
    if (InPdi == nullptr)
    { return; }

    if (GEditor != nullptr && GEditor->PlayWorld != nullptr)
    { return; }

    const auto* PreviewSubsystem = UCk_VoxelNavPreview_EditorSubsystem_UE::Get();
    if (PreviewSubsystem == nullptr)
    { return; }

    const auto Snapshots = PreviewSubsystem->Get_RenderSnapshots();
    if (NOT Snapshots.IsValid())
    { return; }

    auto RemainingMerged = ck_voxelnav_preview_edmode::MaxImmediateBoxesPerLayer;
    auto RemainingRaw = ck_voxelnav_preview_edmode::MaxImmediateBoxesPerLayer;
    auto RemainingOccupied = ck_voxelnav_preview_edmode::MaxImmediateBoxesPerLayer;
    for (const auto& Snapshot : *Snapshots)
    {
        const auto StatusColor = ck_voxelnav_preview_edmode::Get_StatusColor(Snapshot._Status);
        ck_voxelnav_preview_edmode::DrawBounds(
            *InPdi, Snapshot._AuthoredBounds, StatusColor, 2.0f);

        if (NOT ck_voxelnav_preview_edmode::Get_CanDrawRetainedGeometry(Snapshot._Status))
        { continue; }

        ck_voxelnav_preview_edmode::DrawBounds(
            *InPdi, Snapshot._PendingDirtyBounds, FLinearColor(1.0f, 0.50f, 0.10f, 0.95f), 2.0f);
        ck_voxelnav_preview_edmode::DrawBounds(
            *InPdi, Snapshot._ActiveDirtyBounds, FLinearColor(1.0f, 0.10f, 0.10f, 0.95f), 2.0f);

        for (const auto& Chunk : Snapshot._Chunks)
        {
            ck_voxelnav_preview_edmode::DrawBounds(
                *InPdi, Chunk._Bounds, FLinearColor(0.70f, 0.35f, 1.00f, 0.85f), 1.5f);
        }

        ck_voxelnav_preview_edmode::DrawLayer(
            *InPdi, Snapshot._MergedFree,
            Snapshot._Status == ck::voxelnav::EDebugSnapshotStatus::Current
                ? FLinearColor(0.20f, 0.85f, 1.00f, 0.90f) : StatusColor,
            RemainingMerged);
        ck_voxelnav_preview_edmode::DrawLayer(
            *InPdi, Snapshot._RawFree,
            Snapshot._Status == ck::voxelnav::EDebugSnapshotStatus::Current
                ? FLinearColor(0.20f, 1.00f, 0.45f, 0.45f) : StatusColor,
            RemainingRaw);
        ck_voxelnav_preview_edmode::DrawLayer(
            *InPdi, Snapshot._Occupied,
            Snapshot._Status == ck::voxelnav::EDebugSnapshotStatus::Current
                ? FLinearColor(1.00f, 0.20f, 0.15f, 0.45f) : StatusColor,
            RemainingOccupied);

        for (const auto& Portal : Snapshot._Portals)
        {
            ck_voxelnav_preview_edmode::DrawLine(
                *InPdi, Portal._From, Portal._ConnectionPoint, FLinearColor(0.90f, 0.45f, 1.00f), 2.0f);
            ck_voxelnav_preview_edmode::DrawLine(
                *InPdi, Portal._ConnectionPoint, Portal._To, FLinearColor(0.90f, 0.45f, 1.00f), 2.0f);
        }
    }
}

auto
    UCk_VoxelNavPreview_EdMode::
    DrawHUD(
        FEditorViewportClient* InViewportClient,
        FViewport* InViewport,
        const FSceneView* InView,
        FCanvas* InCanvas)
    -> void
{
    Super::DrawHUD(InViewportClient, InViewport, InView, InCanvas);
    if (InCanvas == nullptr || GEditor == nullptr || GEditor->PlayWorld != nullptr)
    { return; }

    const auto* PreviewSubsystem = UCk_VoxelNavPreview_EditorSubsystem_UE::Get();
    const auto Snapshots = PreviewSubsystem != nullptr ? PreviewSubsystem->Get_RenderSnapshots() : nullptr;

    auto Status = ck::voxelnav::EDebugSnapshotStatus::RuntimeOnly;
    auto Message = FString{TEXT("place a Ck Voxel Nav Volume and cook the current map")};
    if (Snapshots.IsValid() && NOT Snapshots->IsEmpty())
    {
        Status = (*Snapshots)[0]._Status;
        Message = (*Snapshots)[0]._StatusMessage;
        for (const auto& Snapshot : *Snapshots)
        {
            if (ck_voxelnav_preview_edmode::Get_StatusRank(Snapshot._Status) >
                ck_voxelnav_preview_edmode::Get_StatusRank(Status))
            {
                Status = Snapshot._Status;
                Message = Snapshot._StatusMessage;
            }
        }
    }

    const auto Label = FString::Printf(
        TEXT("VoxelNav Editor Overlay: %s%s%s"),
        ck_voxelnav_preview_edmode::Get_StatusLabel(Status),
        Message.IsEmpty() ? TEXT("") : TEXT(" - "),
        *Message);
    auto TextItem = FCanvasTextItem{
        FVector2D{16.0, 48.0}, FText::FromString(Label), GEngine->GetSmallFont(),
        ck_voxelnav_preview_edmode::Get_StatusColor(Status)};
    TextItem.EnableShadow(FLinearColor::Black);
    InCanvas->DrawItem(TextItem);
}

auto
    UCk_VoxelNavPreview_EdMode::
    Set_LevelOverlayEnabled(
        bool InEnabled)
    -> bool
{
    if (GEditor == nullptr || GEditor->PlayWorld != nullptr)
    { return false; }

    auto& ModeTools = GLevelEditorModeTools();
    if (InEnabled)
    { ModeTools.ActivateMode(EM_CkVoxelNavPreviewModeId); }
    else
    { ModeTools.DeactivateMode(EM_CkVoxelNavPreviewModeId); }

    GEditor->RedrawLevelEditingViewports();
    return ModeTools.IsModeActive(EM_CkVoxelNavPreviewModeId) == InEnabled;
}

auto
    UCk_VoxelNavPreview_EdMode::
    Get_IsLevelOverlayEnabled()
    -> bool
{
    return GEditor != nullptr && GLevelEditorModeTools().IsModeActive(EM_CkVoxelNavPreviewModeId);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
