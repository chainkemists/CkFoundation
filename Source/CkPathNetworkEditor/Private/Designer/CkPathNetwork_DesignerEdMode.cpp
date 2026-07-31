#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerEdMode.h"

#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerSession.h"
#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerToolkit.h"

#include "CkPathNetwork/Actor/CkPathNetwork_Actor.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Editor.h>
#include <Engine/World.h>
#include <LevelEditor.h>
#include <Modules/ModuleManager.h>
#include <PrimitiveDrawInterface.h>
#include <PrimitiveDrawingUtils.h>
#include <SceneInterface.h>
#include <SceneView.h>
#include <Textures/SlateIcon.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "Ck_PathNetworkDesigner_EdMode"

// --------------------------------------------------------------------------------------------------------------------

const FEditorModeID UCk_PathNetworkDesigner_EdMode::EM_CkPathNetworkDesignerModeId =
    TEXT("Ck.PathNetwork.Designer");

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer::draw
{
    auto
    Draw_Ribbon(
        FPrimitiveDrawInterface& InPDI,
        const FCk_PathNetwork_Ribbon& InRibbon,
        const FLinearColor& InColor,
        const float InCenterThickness,
        const float InEdgeThickness) -> void
    {
        const auto& Points = InRibbon.Get_Points();
        for (auto PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
        {
            const auto& Point = Points[PointIndex];
            InPDI.DrawPoint(
                Point.Get_Location(),
                InColor,
                9.0f,
                SDPG_Foreground);
            if (PointIndex == 0)
            { continue; }

            const auto& Previous = Points[PointIndex - 1];
            InPDI.DrawLine(
                Previous.Get_Location(),
                Point.Get_Location(),
                InColor,
                SDPG_Foreground,
                InCenterThickness,
                0.0f,
                true);

            const auto Tangent =
                (Point.Get_Location() - Previous.Get_Location()).GetSafeNormal2D();
            if (Tangent.IsNearlyZero())
            { continue; }

            const auto Right = FVector::CrossProduct(FVector::UpVector, Tangent);
            auto EdgeColor = InColor;
            EdgeColor.A = 0.65f;
            InPDI.DrawTranslucentLine(
                Previous.Get_Location() + Right * Previous.Get_HalfWidth(),
                Point.Get_Location() + Right * Point.Get_HalfWidth(),
                EdgeColor,
                SDPG_World,
                InEdgeThickness,
                0.0f,
                true);
            InPDI.DrawTranslucentLine(
                Previous.Get_Location() - Right * Previous.Get_HalfWidth(),
                Point.Get_Location() - Right * Point.Get_HalfWidth(),
                EdgeColor,
                SDPG_World,
                InEdgeThickness,
                0.0f,
                true);
        }
    }

    auto
    Draw_Mask(
        FPrimitiveDrawInterface& InPDI,
        const TArray<FVector>& InMaskDrawPoints) -> void
    {
        for (const auto& Point : InMaskDrawPoints)
        {
            InPDI.DrawPoint(
                Point,
                FLinearColor{0.95f, 0.72f, 0.12f, 0.72f},
                4.0f,
                SDPG_World);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCk_PathNetworkDesigner_EdMode::
    UCk_PathNetworkDesigner_EdMode()
{
    Info = FEditorModeInfo(
        EM_CkPathNetworkDesignerModeId,
        LOCTEXT("ModeName", "Ck Path Network"),
        FSlateIcon{},
        true);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    Enter()
    -> void
{
    Super::Enter();

    auto* Session = GetOrCreate_Session();
    auto* EditorWorld = ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{})
        ? GEditor->GetEditorWorldContext().World()
        : nullptr;
    Session->Initialize(EditorWorld);

    auto& LevelEditor =
        FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
    if (NOT _MapChangedHandle.IsValid())
    {
        _MapChangedHandle =
            LevelEditor.OnMapChanged().AddUObject(
                this, &UCk_PathNetworkDesigner_EdMode::OnMapChanged);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    Exit()
    -> void
{
    if (_MapChangedHandle.IsValid()
        && FModuleManager::Get().IsModuleLoaded(TEXT("LevelEditor")))
    {
        auto& LevelEditor =
            FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
        LevelEditor.OnMapChanged().Remove(_MapChangedHandle);
        _MapChangedHandle.Reset();
    }

    if (ck::IsValid(_Session))
    { _Session->Clear_Preview(); }
    _Session = nullptr;

    Super::Exit();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    CreateToolkit()
    -> void
{
    GetOrCreate_Session();
    Toolkit = MakeShared<FCk_PathNetworkDesigner_Toolkit>();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    Render(
        const FSceneView* InView,
        FViewport* InViewport,
        FPrimitiveDrawInterface* InPDI)
    -> void
{
    Super::Render(InView, InViewport, InPDI);

    const bool InputsAreValid =
        ck::IsValid(_Session)
        && InView != nullptr
        && InView->Family != nullptr
        && InView->Family->Scene != nullptr
        && InPDI != nullptr;
    if (NOT InputsAreValid)
    { return; }

    auto* SessionWorld = _Session->GetWorld();
    auto* ViewWorld = InView->Family->Scene->GetWorld();
    if (ck::Is_NOT_Valid(SessionWorld)
        || SessionWorld->WorldType != EWorldType::Editor
        || ViewWorld != SessionWorld)
    { return; }

    if (_Session->Get_DrawBounds())
    {
        DrawWireBox(
            InPDI,
            _Session->Get_DetectionBounds(),
            FLinearColor{0.25f, 0.75f, 1.0f, 1.0f},
            SDPG_Foreground,
            2.0f,
            0.0f,
            true);
    }

    const auto& Preview = _Session->Get_Preview();
    if (Preview._Succeeded)
    {
        if (_Session->Get_DrawMask())
        {
            ck::pathnetwork_editor::designer::draw::Draw_Mask(
                *InPDI,
                _Session->Get_MaskDrawPoints());
        }

        if (_Session->Get_DrawPreviewRibbons())
        {
            for (const auto& Ribbon : Preview._GeneratedWorldRibbons)
            {
                ck::pathnetwork_editor::designer::draw::Draw_Ribbon(
                    *InPDI,
                    Ribbon,
                    FLinearColor{0.1f, 0.9f, 1.0f, 1.0f},
                    4.0f,
                    1.5f);
            }
        }
    }

    if (_Session->Get_DrawComponentTransfers())
    {
        for (const auto& Transfer : _Session->Get_RoutePreview()._ComponentTransferSegments)
        {
            InPDI->DrawLine(
                Transfer._Start + FVector{0.0, 0.0, 13.0},
                Transfer._End + FVector{0.0, 0.0, 13.0},
                FLinearColor{1.0f, 0.62f, 0.08f, 0.65f},
                SDPG_Foreground,
                2.0f,
                0.0f,
                true);
        }
    }

    if (_Session->Get_DrawRoutePreview())
    {
        for (const auto& Watch :
             _Session->Get_RouteWatchPreviews())
        {
            const auto WatchStart =
                Watch._Start + FVector{0.0, 0.0, 8.0};
            const auto WatchGoal =
                Watch._Goal + FVector{0.0, 0.0, 8.0};
            InPDI->DrawPoint(
                WatchStart,
                FLinearColor{0.15f, 1.0f, 0.25f, 0.55f},
                10.0f,
                SDPG_Foreground);
            InPDI->DrawPoint(
                WatchGoal,
                FLinearColor{1.0f, 0.15f, 0.12f, 0.55f},
                10.0f,
                SDPG_Foreground);

            if (NOT Watch._Preview._Succeeded)
            { continue; }

            const auto WatchColor = Watch._Preview._UsesNetwork
                ? FLinearColor{0.95f, 0.15f, 1.0f, 0.42f}
                : FLinearColor{1.0f, 0.48f, 0.08f, 0.42f};
            for (auto PointIndex = 1;
                 PointIndex < Watch._Preview._CompiledWaypoints.Num();
                 ++PointIndex)
            {
                InPDI->DrawLine(
                    Watch._Preview._CompiledWaypoints[PointIndex - 1]
                        + FVector{0.0, 0.0, 8.0},
                    Watch._Preview._CompiledWaypoints[PointIndex]
                        + FVector{0.0, 0.0, 8.0},
                    WatchColor,
                    SDPG_Foreground,
                    2.0f,
                    0.0f,
                    true);
            }
        }

        const auto Start = _Session->Get_RoutePreviewStart()
            + FVector{0.0, 0.0, 10.0};
        const auto Goal = _Session->Get_RoutePreviewGoal()
            + FVector{0.0, 0.0, 10.0};
        if (FVector::Dist(Start, Goal) > 1.0)
        {
            InPDI->DrawPoint(
                Start,
                FLinearColor{0.15f, 1.0f, 0.25f, 1.0f},
                18.0f,
                SDPG_Foreground);
            InPDI->DrawPoint(
                Goal,
                FLinearColor{1.0f, 0.15f, 0.12f, 1.0f},
                18.0f,
                SDPG_Foreground);
        }

        const auto& RoutePreview = _Session->Get_RoutePreview();
        if (RoutePreview._Succeeded)
        {
            const auto IsNetworkDecision =
                RoutePreview._Decision
                    == ck::pathnetwork_editor::designer::
                        ERoutePreviewDecision::NetworkSelected
                || RoutePreview._Decision
                    == ck::pathnetwork_editor::designer::
                        ERoutePreviewDecision::
                            NetworkPreferredByMinimumSavings;
            const auto IsDirectFallback = NOT IsNetworkDecision;
            const auto RouteColor = IsDirectFallback
                ? FLinearColor{1.0f, 0.48f, 0.08f, 1.0f}
                : FLinearColor{0.95f, 0.15f, 1.0f, 1.0f};
            for (auto Index = 0;
                 Index < RoutePreview._CompiledWaypoints.Num();
                 ++Index)
            {
                const auto Point = RoutePreview._CompiledWaypoints[Index]
                    + FVector{0.0, 0.0, 10.0};
                InPDI->DrawPoint(
                    Point,
                    RouteColor,
                    8.0f,
                    SDPG_Foreground);
                if (Index == 0)
                { continue; }

                InPDI->DrawLine(
                    RoutePreview._CompiledWaypoints[Index - 1]
                        + FVector{0.0, 0.0, 10.0},
                    Point,
                    RouteColor,
                    SDPG_Foreground,
                    5.0f,
                    0.0f,
                    true);
            }

            constexpr auto ShortcutDashCount = 12;
            for (const auto& Transfer : RoutePreview._SelectedComponentTransferSegments)
            {
                for (auto DashIndex = 0;
                     DashIndex < ShortcutDashCount;
                     DashIndex += 2)
                {
                    const auto DashStartAlpha =
                        static_cast<double>(DashIndex)
                        / ShortcutDashCount;
                    const auto DashEndAlpha =
                        static_cast<double>(DashIndex + 1)
                        / ShortcutDashCount;
                    InPDI->DrawLine(
                        FMath::Lerp(
                            Transfer._Start,
                            Transfer._End,
                            DashStartAlpha)
                            + FVector{0.0, 0.0, 16.0},
                        FMath::Lerp(
                            Transfer._Start,
                            Transfer._End,
                            DashEndAlpha)
                            + FVector{0.0, 0.0, 16.0},
                        FLinearColor{1.0f, 0.16f, 0.04f, 1.0f},
                        SDPG_Foreground,
                        8.0f,
                        0.0f,
                        true);
                }
            }

            for (const auto& Shortcut :
                 RoutePreview._LocalNetworkShortcutSegments)
            {
                for (auto DashIndex = 0;
                     DashIndex < ShortcutDashCount;
                     DashIndex += 2)
                {
                    const auto DashStartAlpha =
                        static_cast<double>(DashIndex)
                        / ShortcutDashCount;
                    const auto DashEndAlpha =
                        static_cast<double>(DashIndex + 1)
                        / ShortcutDashCount;
                    InPDI->DrawLine(
                        FMath::Lerp(
                            Shortcut._Start,
                            Shortcut._End,
                            DashStartAlpha)
                            + FVector{0.0, 0.0, 14.0},
                        FMath::Lerp(
                            Shortcut._Start,
                            Shortcut._End,
                            DashEndAlpha)
                            + FVector{0.0, 0.0, 14.0},
                        FLinearColor{0.08f, 0.92f, 1.0f, 1.0f},
                        SDPG_Foreground,
                        7.0f,
                        0.0f,
                        true);
                }
            }

            for (const auto& JoinPoint : RoutePreview._JoinPoints)
            {
                InPDI->DrawPoint(
                    JoinPoint + FVector{0.0, 0.0, 12.0},
                    FLinearColor{1.0f, 0.78f, 0.08f, 1.0f},
                    14.0f,
                    SDPG_Foreground);
            }
        }
    }

    const auto* Actor = _Session->Get_VisualizedActor();
    if (ck::Is_NOT_Valid(Actor) || Actor->GetWorld() != SessionWorld)
    { return; }

    for (const auto& Ribbon : Actor->Get_WorldRibbons())
    {
        const bool IsAuthored =
            Ribbon.Get_Source() == ECk_PathNetwork_RibbonSource::Authored;
        if ((IsAuthored && NOT _Session->Get_DrawAuthoredRibbons())
            || (NOT IsAuthored && NOT _Session->Get_DrawGeneratedRibbons()))
        { continue; }

        ck::pathnetwork_editor::designer::draw::Draw_Ribbon(
            *InPDI,
            Ribbon,
            IsAuthored
                ? FLinearColor{0.25f, 0.95f, 0.45f, 1.0f}
                : FLinearColor{1.0f, 0.48f, 0.1f, 1.0f},
            3.0f,
            1.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    Get_Session() const
    -> UCk_PathNetworkDesigner_Session_UE*
{
    return _Session;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    GetOrCreate_Session()
    -> UCk_PathNetworkDesigner_Session_UE*
{
    if (ck::Is_NOT_Valid(_Session))
    {
        _Session = NewObject<UCk_PathNetworkDesigner_Session_UE>(
            this, NAME_None, RF_Transient);
    }
    return _Session;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PathNetworkDesigner_EdMode::
    OnMapChanged(
        UWorld* InWorld,
        EMapChangeType InMapChangeType)
    -> void
{
    if (ck::Is_NOT_Valid(_Session))
    { return; }

    _Session->Clear_Preview();
    auto* EditorWorld = ck::IsValid(GEditor, ck::IsValid_Policy_NullptrOnly{})
        ? GEditor->GetEditorWorldContext().World()
        : nullptr;
    _Session->Initialize(EditorWorld);
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE
