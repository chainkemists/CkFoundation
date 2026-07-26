#include "CkMinimapFrame_Widget.h"

#include "CkMinimap/CkMinimap_Log.h"
#include "CkMinimap/CkMinimap_Utils.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_minimap_frame_widget
{
    const TCHAR* ObserverArrowGlyph = TEXT("▲");
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MinimapFrame_Widget::
    Set_Minimap(
        FCk_Handle_Minimap& InMinimap)
    -> void
{
    auto AppearedDelegate = FCk_Delegate_Minimap_EntryAppeared{};
    AppearedDelegate.BindDynamic(this, &UCk_MinimapFrame_Widget::HandleEntryAppeared);

    auto DisappearedDelegate = FCk_Delegate_Minimap_EntryDisappeared{};
    DisappearedDelegate.BindDynamic(this, &UCk_MinimapFrame_Widget::HandleEntryDisappeared);

    if (ck::IsValid(_Minimap))
    {
        UCk_Utils_Minimap_UE::UnbindFrom_OnEntryAppeared(_Minimap, AppearedDelegate);
        UCk_Utils_Minimap_UE::UnbindFrom_OnEntryDisappeared(_Minimap, DisappearedDelegate);
    }

    _Minimap = InMinimap;

    CK_ENSURE_IF_NOT(ck::IsValid(_Minimap), TEXT("Invalid Minimap Handle passed to MinimapFrame Widget [{}]"), this)
    { return; }

    // Seed-then-delta: signal replay stashes at most the LAST payload, so seeding must come from the pull API
    for (const auto& Entry : UCk_Utils_Minimap_UE::Get_Entries(_Minimap))
    {
        OnMinimapEntryAppeared(Entry);
    }

    UCk_Utils_Minimap_UE::BindTo_OnEntryAppeared(_Minimap, AppearedDelegate,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing);

    UCk_Utils_Minimap_UE::BindTo_OnEntryDisappeared(_Minimap, DisappearedDelegate,
        ECk_Signal_BindingPolicy::IgnorePayloadInFlight,
        ECk_Signal_PostFireBehavior::DoNothing);
}

auto
    UCk_MinimapFrame_Widget::
    Get_Minimap() const
    -> FCk_Handle_Minimap
{
    return _Minimap;
}

auto
    UCk_MinimapFrame_Widget::
    Set_MapTexture(
        UTexture2D* InTexture,
        const FCk_Minimap_WorldBounds& InBounds)
    -> void
{
    _MapTexture = InTexture;
    _MapBounds = InBounds;

    if (ck::IsValid(_MapImage) && ck::IsValid(_MapTexture))
    {
        _MapImage->SetBrushFromTexture(_MapTexture);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MinimapFrame_Widget::
    NativeOnInitialized()
    -> void
{
    Super::NativeOnInitialized();

    DoBuildWidgetTree();
}

auto
    UCk_MinimapFrame_Widget::
    NativeDestruct()
    -> void
{
    if (ck::IsValid(_Minimap))
    {
        auto AppearedDelegate = FCk_Delegate_Minimap_EntryAppeared{};
        AppearedDelegate.BindDynamic(this, &UCk_MinimapFrame_Widget::HandleEntryAppeared);

        auto DisappearedDelegate = FCk_Delegate_Minimap_EntryDisappeared{};
        DisappearedDelegate.BindDynamic(this, &UCk_MinimapFrame_Widget::HandleEntryDisappeared);

        UCk_Utils_Minimap_UE::UnbindFrom_OnEntryAppeared(_Minimap, AppearedDelegate);
        UCk_Utils_Minimap_UE::UnbindFrom_OnEntryDisappeared(_Minimap, DisappearedDelegate);

        _Minimap = {};
    }

    Super::NativeDestruct();
}

auto
    UCk_MinimapFrame_Widget::
    NativeTick(
        const FGeometry& MyGeometry,
        float InDeltaTime)
    -> void
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    DoRefreshLayout(MyGeometry);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MinimapFrame_Widget::
    HandleEntryAppeared(
        FCk_Handle_Minimap InMinimap,
        FCk_Minimap_Entry InEntry)
    -> void
{
    OnMinimapEntryAppeared(InEntry);
}

auto
    UCk_MinimapFrame_Widget::
    HandleEntryDisappeared(
        FCk_Handle_Minimap InMinimap,
        FCk_Handle_Poi InPoi)
    -> void
{
    OnMinimapEntryDisappeared(InPoi);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_MinimapFrame_Widget::
    DoBuildWidgetTree()
    -> void
{
    using namespace ck_minimap_frame_widget;

    const auto HasDesignerAuthoredTree = ck::IsValid(WidgetTree->RootWidget);

    if (HasDesignerAuthoredTree)
    { return; }

    _RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapFrame_RootCanvas"));
    WidgetTree->RootWidget = _RootCanvas;

    _MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapFrame_Map"));

    auto MapSlot = _RootCanvas->AddChildToCanvas(_MapImage);
    MapSlot->SetAutoSize(false);
    MapSlot->SetAlignment(FVector2D{0.5, 0.5});

    if (ck::IsValid(_MapTexture))
    {
        _MapImage->SetBrushFromTexture(_MapTexture);
    }

    if (ck::IsValid(_FrameMaterial))
    {
        _FrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapFrame_Frame"));
        _FrameMID = UMaterialInstanceDynamic::Create(_FrameMaterial, this);
        _FrameImage->SetBrushFromMaterial(_FrameMID);

        auto FrameSlot = _RootCanvas->AddChildToCanvas(_FrameImage);
        FrameSlot->SetAutoSize(false);
        FrameSlot->SetAlignment(FVector2D{0.5, 0.5});
    }

    _ObserverArrow = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MinimapFrame_Observer"));
    _ObserverArrow->SetText(FText::FromString(ObserverArrowGlyph));

    auto ArrowSlot = _RootCanvas->AddChildToCanvas(_ObserverArrow);
    ArrowSlot->SetAutoSize(true);
    ArrowSlot->SetAlignment(FVector2D{0.5, 0.5});
}

auto
    UCk_MinimapFrame_Widget::
    DoRefreshLayout(
        const FGeometry& InGeometry)
    -> void
{
    const auto OwnsBuiltInTree = ck::IsValid(_RootCanvas);

    if (NOT OwnsBuiltInTree)
    { return; }

    const auto MinimapIsValid = ck::IsValid(_Minimap);

    if (NOT MinimapIsValid)
    {
        if (ck::IsValid(_MapImage))
        { _MapImage->SetVisibility(ESlateVisibility::Hidden); }

        if (ck::IsValid(_ObserverArrow))
        { _ObserverArrow->SetVisibility(ESlateVisibility::Hidden); }

        for (const auto& Blip : _BlipPool)
        { Blip->SetVisibility(ESlateVisibility::Hidden); }

        return;
    }

    const auto PanelSize = InGeometry.GetLocalSize();
    const auto PanelCenter = PanelSize * 0.5;
    const auto FrameSize = static_cast<float>(FMath::Min(PanelSize.X, PanelSize.Y));
    const auto HalfFrame = FrameSize * 0.5f;

    const auto ProjectionMode = UCk_Utils_Minimap_UE::Get_ProjectionMode(_Minimap);
    const auto RotationMode = UCk_Utils_Minimap_UE::Get_RotationMode(_Minimap);
    const auto ViewYaw = UCk_Utils_Minimap_UE::Get_ViewYawDegrees(_Minimap);

    const auto RotateWithObserver =
        ProjectionMode == ECk_Minimap_ProjectionMode::ObserverCentric &&
        RotationMode == ECk_Minimap_RotationMode::RotateWithObserver;

    if (ck::IsValid(_FrameMID))
    {
        _FrameMID->SetScalarParameterValue(_ViewYawParameterName, ViewYaw);
    }

    if (ck::IsValid(_FrameImage))
    {
        const auto FrameSlot = Cast<UCanvasPanelSlot>(_FrameImage->Slot);
        if (ck::IsValid(FrameSlot, ck::IsValid_Policy_NullptrOnly{}))
        {
            FrameSlot->SetPosition(PanelCenter);
            FrameSlot->SetSize(FVector2D{FrameSize, FrameSize});
        }
    }

    DoRefreshBackground(PanelCenter, FrameSize, ViewYaw, RotateWithObserver);

    if (ck::IsValid(_ObserverArrow))
    {
        _ObserverArrow->SetVisibility(ESlateVisibility::HitTestInvisible);
        _ObserverArrow->SetRenderTransformAngle(RotateWithObserver ? 0.0f : ViewYaw);
        DoPositionChildAt(_ObserverArrow, PanelCenter);
    }

    const auto Entries = UCk_Utils_Minimap_UE::Get_Entries(_Minimap);

    for (auto Index = _BlipPool.Num(); Index < Entries.Num(); ++Index)
    {
        auto Blip = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
        Blip->SetDesiredSizeOverride(_BlipSize);

        auto BlipSlot = _RootCanvas->AddChildToCanvas(Blip);
        BlipSlot->SetAutoSize(true);
        BlipSlot->SetAlignment(FVector2D{0.5, 0.5});

        _BlipPool.Emplace(Blip);
    }

    for (auto Index = 0; Index < _BlipPool.Num(); ++Index)
    {
        const auto& Blip = _BlipPool[Index];

        if (Index >= Entries.Num())
        {
            Blip->SetVisibility(ESlateVisibility::Hidden);
            continue;
        }

        const auto& Entry = Entries[Index];
        const auto IsClamped = Entry.Get_EdgeState() == ECk_Minimap_EntryEdgeState::ClampedToEdge;

        auto Tint = _BlipTint;
        Tint.A *= IsClamped ? _ClampedBlipOpacity : 1.0f;

        const auto BlipAngle = RotateWithObserver
            ? Entry.Get_WorldYawDegrees() - ViewYaw
            : Entry.Get_WorldYawDegrees();

        Blip->SetVisibility(ESlateVisibility::HitTestInvisible);
        Blip->SetColorAndOpacity(Tint);
        Blip->SetRenderTransformAngle(BlipAngle);
        DoPositionChildAt(Blip, PanelCenter + Entry.Get_MapPosition() * HalfFrame);
    }
}

auto
    UCk_MinimapFrame_Widget::
    DoRefreshBackground(
        const FVector2D& InPanelCenter,
        float InFrameSize,
        float InViewYaw,
        bool InRotateWithObserver)
    -> void
{
    if (ck::Is_NOT_Valid(_MapImage))
    { return; }

    const auto HasImagery = ck::IsValid(_MapTexture) && ck::IsValid(_MapBounds);

    if (NOT HasImagery)
    {
        _MapImage->SetVisibility(ESlateVisibility::Hidden);
        return;
    }

    _MapImage->SetVisibility(ESlateVisibility::HitTestInvisible);

    const auto MapSlot = Cast<UCanvasPanelSlot>(_MapImage->Slot);

    if (ck::Is_NOT_Valid(MapSlot, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    const auto ProjectionMode = UCk_Utils_Minimap_UE::Get_ProjectionMode(_Minimap);

    if (ProjectionMode == ECk_Minimap_ProjectionMode::FixedBounds)
    {
        // The whole bounds fill the frame per-axis — matching Get_BoundsToFrame's per-axis normalize, so
        // blips and imagery agree (non-square bounds render aspect-distorted; a real HUD picks square bounds
        // or letterboxes in its own subclass)
        MapSlot->SetPosition(InPanelCenter);
        MapSlot->SetSize(FVector2D{InFrameSize, InFrameSize});
        _MapImage->SetRenderTransformAngle(0.0f);
        return;
    }

    // ObserverCentric: uniform world-to-pixel scale; the image is laid out at the BOUNDS aspect ratio.
    // Screen X covers world Y (East) and screen Y covers world X (North) — hence the axis swap in the
    // image dimensions.
    const auto ViewExtent = UCk_Utils_Minimap_UE::Get_ViewExtent(_Minimap);

    CK_ENSURE_IF_NOT(ViewExtent > 0.0f, TEXT("MinimapFrame Widget [{}] has a non-positive ViewExtent"), this)
    { return; }

    const auto HalfExtents = _MapBounds.Get_HalfExtents();
    const auto ImageSize = FVector2D
    {
        InFrameSize * HalfExtents.Y / ViewExtent,
        InFrameSize * HalfExtents.X / ViewExtent
    };

    const auto ViewOrigin = UCk_Utils_Minimap_UE::Get_ViewOrigin(_Minimap);
    const auto ObserverInBoundsFrame = UCk_Utils_Minimap_UE::Get_BoundsToFrame(ViewOrigin, _MapBounds);
    const auto ObserverPx = FVector2D
    {
        ObserverInBoundsFrame.X * ImageSize.X * 0.5,
        ObserverInBoundsFrame.Y * ImageSize.Y * 0.5
    };

    const auto Angle = InRotateWithObserver ? -InViewYaw : 0.0f;

    // Rotation happens about the image's own center (render-transform pivot), so the translation that pins
    // the observer at the frame center must itself be rotated: Translate = R_angle(-ObserverPx)
    const auto Translate = DoRotate2D(-ObserverPx, Angle);

    MapSlot->SetPosition(InPanelCenter + Translate);
    MapSlot->SetSize(ImageSize);
    _MapImage->SetRenderTransformAngle(Angle);
}

auto
    UCk_MinimapFrame_Widget::
    DoPositionChildAt(
        UWidget* InChild,
        const FVector2D& InPixelPosition) const
    -> void
{
    const auto CanvasSlot = Cast<UCanvasPanelSlot>(InChild->Slot);

    if (ck::Is_NOT_Valid(CanvasSlot, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    CanvasSlot->SetPosition(InPixelPosition);
}

auto
    UCk_MinimapFrame_Widget::
    DoRotate2D(
        const FVector2D& InVector,
        float InAngleDegrees)
    -> FVector2D
{
    const auto Radians = FMath::DegreesToRadians(InAngleDegrees);
    const auto Cos = FMath::Cos(Radians);
    const auto Sin = FMath::Sin(Radians);

    return FVector2D
    {
        InVector.X * Cos - InVector.Y * Sin,
        InVector.X * Sin + InVector.Y * Cos
    };
}

// --------------------------------------------------------------------------------------------------------------------
