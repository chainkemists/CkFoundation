#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SToolTip.h"
#include "Widgets/Text/STextBlock.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE_TwoParams(FOnFrameSelectionChanged, uint64 /*StartFrame*/, uint64 /*EndFrame*/);

// --------------------------------------------------------------------------------------------------------------------

/**
 * Lightweight frame time bar chart widget.
 *
 * Draws vertical bars for each frame, colored by severity:
 * - Green: under budget
 * - Yellow: 1x-2x over budget
 * - Red: > 2x over budget
 *
 * Supports click-to-select, drag-to-select-range, scroll-to-zoom, and middle-drag-to-pan.
 * Fires FOnFrameSelectionChanged when the user selects frame(s).
 */
class CKINSIGHTSANALYZER_API SCkFrameBarChart : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCkFrameBarChart)
        : _TargetFrameMs(16.67)
    {}
        SLATE_ARGUMENT(double, TargetFrameMs)
        SLATE_EVENT(FOnFrameSelectionChanged, OnFrameSelectionChanged)
    SLATE_END_ARGS()

    auto Construct(const FArguments& InArgs) -> void;

    // ---- Data ----

    /** Set frame data. Resets viewport and selection. */
    auto SetFrameData(TArray<double>&& InFrameDurationsMs) -> void;

    /** Append frame data incrementally (for progressive loading). Does not reset viewport. */
    auto AppendFrameData(const TArray<double>& NewDurationsMs) -> void;

    /** Clear all frame data. */
    auto ClearFrameData() -> void;

    /** Recalculate display max using P95 (call after progressive loading completes). */
    auto RecalculateDisplayMax() -> void;

    /** Get total frame count. */
    auto GetFrameCount() const -> uint64 { return static_cast<uint64>(_FrameDurationsMs.Num()); }

    /** Set the target frame budget in ms. */
    auto SetTargetFrameMs(double InTargetMs) -> void;

    // ---- Selection ----

    /** Get the current selection range (inclusive). */
    auto GetSelectionStart() const -> uint64 { return _SelectionStart; }
    auto GetSelectionEnd() const -> uint64 { return _SelectionEnd; }
    auto HasSelection() const -> bool { return _bHasSelection; }

    /** Programmatically select a frame range. */
    auto SetSelection(uint64 StartFrame, uint64 EndFrame) -> void;

    /** Clear selection. */
    auto ClearSelection() -> void;

    // ---- SWidget Interface ----

    virtual auto SupportsKeyboardFocus() const -> bool override { return true; }

    virtual auto ComputeDesiredSize(float LayoutScaleMultiplier) const -> FVector2D override;
    virtual auto OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
                         const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
                         int32 LayerId, const FWidgetStyle& InWidgetStyle,
                         bool bParentEnabled) const -> int32 override;

    virtual auto OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    virtual auto OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    virtual auto OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    virtual auto OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> FReply override;
    virtual auto OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) -> void override;
    virtual auto OnMouseLeave(const FPointerEvent& MouseEvent) -> void override;
    virtual auto OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) -> FReply override;

private:
    /** Convert a local X coordinate to a frame index. Clamps to valid range. */
    auto XToFrame(const FGeometry& Geometry, float LocalX) const -> int64;

    /** Convert a frame index to a local X coordinate. */
    auto FrameToX(const FGeometry& Geometry, int64 FrameIndex) const -> float;

    /** Get bar color for a given frame duration. */
    auto GetBarColor(double DurationMs) const -> FLinearColor;

    /** Compute bar width for current zoom level. */
    auto GetBarWidth(const FGeometry& Geometry) const -> float;

    /** Compute bar step (bar width + adaptive spacing). */
    auto GetBarStep(const FGeometry& Geometry) const -> float;

    /** Apply zoom centered on a given local X position. Delta > 0 = zoom in, < 0 = zoom out. */
    auto ApplyZoom(const FGeometry& Geometry, float LocalX, float Delta) -> void;

private:
    TArray<double> _FrameDurationsMs;
    double _TargetFrameMs = 16.67;

    // Viewport state
    double _ViewOffset = 0.0;       // First visible frame (fractional for smooth scrolling)
    double _FramesPerPixel = 1.0;   // Zoom level: how many frames fit in one pixel

    // Selection
    bool   _bHasSelection = false;
    uint64 _SelectionStart = 0;
    uint64 _SelectionEnd = 0;

    // Interaction state
    bool   _bIsDragging = false;
    bool   _bIsPanning = false;
    int64  _DragStartFrame = 0;
    float  _PanStartX = 0.0f;
    double _PanStartOffset = 0.0;
    int64  _HoveredFrame = -1;

    FOnFrameSelectionChanged _OnFrameSelectionChanged;

    /** Cached display max (P95-based) so outlier frames don't squish normal bars. */
    double _DisplayMaxMs = 0.0;

    // Tooltip
    TSharedPtr<SToolTip>   _FrameTooltip;
    TSharedPtr<STextBlock> _TooltipTextBlock;
};

// --------------------------------------------------------------------------------------------------------------------
