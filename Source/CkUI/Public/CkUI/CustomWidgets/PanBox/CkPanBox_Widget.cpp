#include "CkPanBox_Widget.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Components/PanelSlot.h>
#include <Widgets/SNullWidget.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_PanBoxWidget_UE::
    UCk_PanBoxWidget_UE()
{
    SetClipping(EWidgetClipping::ClipToBounds);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PanBoxWidget_UE::
    Request_SetView(
        const FCk_PanBox_View& InView)
    -> void
{
    const auto ZoomIsValid = InView.Get_Zoom() > 0.0f;
    CK_ENSURE_IF_NOT(ZoomIsValid, TEXT("PanBox Request_SetView rejected: Zoom [{}] must be > 0"), InView.Get_Zoom())
    { return; }

    const auto NewView = DoGet_ClampedView(InView);

    const auto ViewChanged = NOT NewView.Get_Offset().Equals(_View.Get_Offset()) ||
        NOT FMath::IsNearlyEqual(NewView.Get_Zoom(), _View.Get_Zoom());

    _View = NewView;

    if (NOT ViewChanged)
    { return; }

    DoInvalidateLayout();
    OnViewChanged.Broadcast(_View);
}

auto
    UCk_PanBoxWidget_UE::
    Get_View() const
    -> FCk_PanBox_View
{
    return _View;
}

auto
    UCk_PanBoxWidget_UE::
    StartPan()
    -> FCk_PanBox_View
{
    _PanAnchorView = _View;
    return _PanAnchorView;
}

auto
    UCk_PanBoxWidget_UE::
    Get_ViewForPan(
        FVector2D InViewportDelta) const
    -> FCk_PanBox_View
{
    return DoGet_ClampedView(FCk_PanBox_View{_PanAnchorView.Get_Offset() + InViewportDelta, _PanAnchorView.Get_Zoom()});
}

auto
    UCk_PanBoxWidget_UE::
    Get_ViewForZoomAt(
        FVector2D InViewportPoint,
        float InTargetZoom) const
    -> FCk_PanBox_View
{
    const auto TargetZoomIsValid = InTargetZoom > 0.0f;
    CK_ENSURE_IF_NOT(TargetZoomIsValid, TEXT("PanBox Get_ViewForZoomAt rejected: TargetZoom [{}] must be > 0"), InTargetZoom)
    { return _View; }

    // Clamp the target zoom BEFORE deriving the offset — computing the offset for an
    // unclamped zoom and then clamping would make the pivot point drift.
    const auto TargetZoom = DoGet_ClampedZoom(InTargetZoom);
    const auto CurrentZoom = FMath::Max(_View.Get_Zoom(), UE_KINDA_SMALL_NUMBER);
    const auto ZoomRatio = TargetZoom / CurrentZoom;

    const auto NewOffset = InViewportPoint - (InViewportPoint - _View.Get_Offset()) * ZoomRatio;

    return DoGet_ClampedView(FCk_PanBox_View{NewOffset, TargetZoom});
}

auto
    UCk_PanBoxWidget_UE::
    Get_ViewportToSurface(
        FVector2D InViewportPos) const
    -> FVector2D
{
    const auto Zoom = FMath::Max(_View.Get_Zoom(), UE_KINDA_SMALL_NUMBER);
    return (InViewportPos - _View.Get_Offset()) / Zoom;
}

auto
    UCk_PanBoxWidget_UE::
    Get_SurfaceToViewport(
        FVector2D InSurfacePos) const
    -> FVector2D
{
    return InSurfacePos * _View.Get_Zoom() + _View.Get_Offset();
}

auto
    UCk_PanBoxWidget_UE::
    Get_SurfaceSize() const
    -> FVector2D
{
    if (_SurfaceSizeMode == ECk_PanBox_SurfaceSizeMode::Explicit)
    { return _ExplicitSurfaceSize; }

    const auto ContentSlot = GetContentSlot();

    if (ck::Is_NOT_Valid(ContentSlot) || ck::Is_NOT_Valid(ContentSlot->Content))
    { return FVector2D::ZeroVector; }

    return ContentSlot->Content->GetDesiredSize();
}

auto
    UCk_PanBoxWidget_UE::
    Get_ViewportSize() const
    -> FVector2D
{
    if (ck::Is_NOT_Valid(_PanBox))
    { return FVector2D::ZeroVector; }

    return _PanBox->GetTickSpaceGeometry().GetLocalSize();
}

auto
    UCk_PanBoxWidget_UE::
    Request_SetZoomRange(
        const FCk_FloatRange& InZoomRange)
    -> void
{
    const auto ZoomRangeIsValid = InZoomRange.Get_Min() > 0.0 && InZoomRange.Get_Max() >= InZoomRange.Get_Min();
    CK_ENSURE_IF_NOT(ZoomRangeIsValid, TEXT("PanBox Request_SetZoomRange rejected: [{}] must have 0 < Min <= Max"), InZoomRange)
    { return; }

    _ZoomRange = InZoomRange;
    Request_SetView(_View);
}

auto
    UCk_PanBoxWidget_UE::
    Request_SetBoundsPolicy(
        ECk_PanBox_BoundsPolicy InBoundsPolicy)
    -> void
{
    _BoundsPolicy = InBoundsPolicy;
    Request_SetView(_View);
}

auto
    UCk_PanBoxWidget_UE::
    Request_SetSurfaceSizeMode(
        ECk_PanBox_SurfaceSizeMode InSurfaceSizeMode)
    -> void
{
    _SurfaceSizeMode = InSurfaceSizeMode;
    DoInvalidateLayout();
    Request_SetView(_View);
}

auto
    UCk_PanBoxWidget_UE::
    Request_SetExplicitSurfaceSize(
        FVector2D InExplicitSurfaceSize)
    -> void
{
    _ExplicitSurfaceSize = InExplicitSurfaceSize;
    DoInvalidateLayout();
    Request_SetView(_View);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PanBoxWidget_UE::
    SynchronizeProperties()
    -> void
{
    Super::SynchronizeProperties();

    _View = DoGet_ClampedView(_View);
    DoInvalidateLayout();
}

auto
    UCk_PanBoxWidget_UE::
    ReleaseSlateResources(
        bool bReleaseChildren)
    -> void
{
    Super::ReleaseSlateResources(bReleaseChildren);

    _PanBox.Reset();
}

#if WITH_EDITOR
auto
    UCk_PanBoxWidget_UE::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

auto
    UCk_PanBoxWidget_UE::
    RebuildWidget()
    -> TSharedRef<SWidget>
{
    _PanBox = SNew(SCk_PanBox)
        .View_UObject(this, &UCk_PanBoxWidget_UE::Get_View)
        .ExplicitSurfaceSize_UObject(this, &UCk_PanBoxWidget_UE::DoGet_ExplicitSurfaceSizeForSlate);

    if (GetChildrenCount() > 0)
    {
        const auto ContentSlot = GetContentSlot();
        _PanBox->Set_Content(ck::IsValid(ContentSlot->Content) ? ContentSlot->Content->TakeWidget() : SNullWidget::NullWidget);
    }

    return _PanBox.ToSharedRef();
}

auto
    UCk_PanBoxWidget_UE::
    OnSlotAdded(
        UPanelSlot* InSlot)
    -> void
{
    if (ck::Is_NOT_Valid(_PanBox))
    { return; }

    _PanBox->Set_Content(ck::IsValid(InSlot->Content) ? InSlot->Content->TakeWidget() : SNullWidget::NullWidget);
}

auto
    UCk_PanBoxWidget_UE::
    OnSlotRemoved(
        UPanelSlot* InSlot)
    -> void
{
    if (ck::Is_NOT_Valid(_PanBox))
    { return; }

    _PanBox->Set_Content(SNullWidget::NullWidget);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_PanBoxWidget_UE::
    DoGet_ClampedView(
        const FCk_PanBox_View& InView) const
    -> FCk_PanBox_View
{
    const auto ClampedZoom = DoGet_ClampedZoom(InView.Get_Zoom());

    auto ClampedOffset = InView.Get_Offset();

    if (_BoundsPolicy == ECk_PanBox_BoundsPolicy::ClampViewToSurface)
    {
        const auto ViewportSize = Get_ViewportSize();
        const auto OffsetRange = ViewportSize - Get_SurfaceSize() * ClampedZoom;

        ClampedOffset.X = FMath::Clamp(ClampedOffset.X, FMath::Min(0.0, OffsetRange.X), FMath::Max(0.0, OffsetRange.X));
        ClampedOffset.Y = FMath::Clamp(ClampedOffset.Y, FMath::Min(0.0, OffsetRange.Y), FMath::Max(0.0, OffsetRange.Y));
    }

    return FCk_PanBox_View{ClampedOffset, ClampedZoom};
}

auto
    UCk_PanBoxWidget_UE::
    DoGet_ClampedZoom(
        float InZoom) const
    -> float
{
    const auto MinZoom = FMath::Max(static_cast<float>(_ZoomRange.Get_Min()), UE_KINDA_SMALL_NUMBER);
    const auto MaxZoom = FMath::Max(static_cast<float>(_ZoomRange.Get_Max()), MinZoom);

    return FMath::Clamp(InZoom, MinZoom, MaxZoom);
}

auto
    UCk_PanBoxWidget_UE::
    DoGet_ExplicitSurfaceSizeForSlate() const
    -> TOptional<FVector2D>
{
    if (_SurfaceSizeMode == ECk_PanBox_SurfaceSizeMode::Explicit)
    { return _ExplicitSurfaceSize; }

    return {};
}

auto
    UCk_PanBoxWidget_UE::
    DoInvalidateLayout()
    -> void
{
    if (ck::Is_NOT_Valid(_PanBox))
    { return; }

    _PanBox->Invalidate(EInvalidateWidgetReason::Layout);
}

// --------------------------------------------------------------------------------------------------------------------
