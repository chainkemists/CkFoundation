#include "CkInsightsAnalyzer/Tab/SCkFrameBarChart.h"

#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"

// --------------------------------------------------------------------------------------------------------------------

namespace FrameBarChart
{
    constexpr float MinBarWidth = 1.0f;
    constexpr float MaxBarWidth = 20.0f;
    constexpr float DesiredHeight = 200.0f;
    constexpr float TopPadding = 18.0f;     // room for frame number labels
    constexpr float BottomPadding = 4.0f;
    constexpr float LabelRightMargin = 120.0f;  // room for "100 ms (10 fps)" labels
    constexpr double MinFramesPerPixel = 0.05;   // max zoom in
    constexpr double MaxFramesPerPixel = 500.0;  // max zoom out (allows fitting thousands of frames)

    // Fixed vertical ceiling — ensures threshold lines are always visible
    constexpr double MinDisplayMaxMs = 120.0;

    const FLinearColor ColorGreen  = FLinearColor(0.2f, 0.8f, 0.2f, 0.9f);
    const FLinearColor ColorYellow = FLinearColor(0.9f, 0.8f, 0.1f, 0.9f);
    const FLinearColor ColorRed    = FLinearColor(0.9f, 0.15f, 0.15f, 0.9f);
    const FLinearColor ColorHover  = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
    const FLinearColor ColorSelect = FLinearColor(0.3f, 0.5f, 1.0f, 0.25f);
    const FLinearColor ColorThresholdGreen  = FLinearColor(0.2f, 0.6f, 0.2f, 0.4f);
    const FLinearColor ColorThresholdYellow = FLinearColor(0.7f, 0.6f, 0.1f, 0.4f);
    const FLinearColor ColorThresholdOrange = FLinearColor(0.8f, 0.5f, 0.1f, 0.4f);
    const FLinearColor ColorThresholdRed    = FLinearColor(0.7f, 0.2f, 0.2f, 0.4f);
    const FLinearColor ColorLabel  = FLinearColor(0.6f, 0.6f, 0.6f, 0.7f);
    const FLinearColor ColorBackground = FLinearColor(0.02f, 0.02f, 0.02f, 1.0f);

    /** Format an integer with comma separators (e.g. 4160 → "4,160"). */
    auto FormatWithCommas(int64 Value) -> FString
    {
        FString Raw = FString::Printf(TEXT("%lld"), Value);
        FString Result;
        const int32 Len = Raw.Len();
        for (int32 i = 0; i < Len; ++i)
        {
            if (i > 0 && (Len - i) % 3 == 0)
            {
                Result.AppendChar(TEXT(','));
            }
            Result.AppendChar(Raw[i]);
        }
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    Construct(const FArguments& InArgs)
    -> void
{
    _TargetFrameMs = InArgs._TargetFrameMs;
    _OnFrameSelectionChanged = InArgs._OnFrameSelectionChanged;

    // Create hover tooltip
    _FrameTooltip = SNew(SToolTip)
        .TextMargin(FMargin(4.0f))
        [
            SAssignNew(_TooltipTextBlock, STextBlock)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
        ];

    SetToolTip(_FrameTooltip);

    // Ensure this widget is hit-testable everywhere (not just over children).
    SetVisibility(EVisibility::Visible);

    // Child fills the entire widget area for reliable hit-testing.
    // SBox defaults to SelfHitTestInvisible, so we must override to Visible.
    ChildSlot
    [
        SNew(SBox)
        .Visibility(EVisibility::Visible)
    ];
}

// --------------------------------------------------------------------------------------------------------------------
// Data
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    SetFrameData(TArray<double>&& InFrameDurationsMs)
    -> void
{
    _FrameDurationsMs = MoveTemp(InFrameDurationsMs);
    _ViewOffset = 0.0;

    // Compute display max using P95 so outlier frames don't squish normal bars.
    // Bars above this value simply clamp to full height.
    if (_FrameDurationsMs.Num() > 0)
    {
        TArray<double> Sorted = _FrameDurationsMs;
        Sorted.Sort();
        const int32 P95Index = FMath::Min(
            FMath::FloorToInt32(static_cast<float>(Sorted.Num()) * 0.95f),
            Sorted.Num() - 1);
        const double P95 = Sorted[P95Index];
        // Use P95 * 1.3 with a floor ensuring the 100ms threshold line is always visible
        _DisplayMaxMs = FMath::Max(P95 * 1.3, FrameBarChart::MinDisplayMaxMs);
    }
    else
    {
        _DisplayMaxMs = FrameBarChart::MinDisplayMaxMs;
    }

    // Compute initial zoom to fit all frames in the widget.
    // Estimate width; when the actual geometry isn't available yet, use 1000px.
    const double EstWidgetWidth = 1000.0;
    const double NumFrames = FMath::Max(1.0, static_cast<double>(_FrameDurationsMs.Num()));
    // Target: NumFrames * (BarWidth + Spacing) <= EstWidgetWidth
    // At MinBarWidth=1 with no spacing: each frame takes 1px.
    // FramesPerPixel = 1/BarWidth, so BarWidth = 1/FramesPerPixel.
    const double DesiredBarStep = EstWidgetWidth / NumFrames;
    const double DesiredBarWidth = FMath::Clamp(
        DesiredBarStep - ((DesiredBarStep >= 3.0) ? 1.0 : 0.0),
        static_cast<double>(FrameBarChart::MinBarWidth),
        static_cast<double>(FrameBarChart::MaxBarWidth));
    _FramesPerPixel = FMath::Clamp(
        1.0 / DesiredBarWidth,
        FrameBarChart::MinFramesPerPixel,
        FrameBarChart::MaxFramesPerPixel);

    ClearSelection();
    Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    SCkFrameBarChart::
    AppendFrameData(const TArray<double>& NewDurationsMs)
    -> void
{
    if (NewDurationsMs.Num() == 0) return;

    _FrameDurationsMs.Append(NewDurationsMs);

    // During progressive loading, keep _DisplayMaxMs at MinDisplayMaxMs (120ms).
    // Outlier frames (startup/shutdown with 10s+ durations) would blow up a max-based
    // heuristic, making normal bars invisible. RecalculateDisplayMax() is called after
    // all data is loaded to apply proper P95-based scaling.

    // Auto-fit zoom: adjust so all loaded frames fill the widget width.
    // This makes bars grow from left to right during loading (like Insights).
    _ViewOffset = 0.0;
    const double EstWidgetWidth = 1000.0;
    const double NumFrames = FMath::Max(1.0, static_cast<double>(_FrameDurationsMs.Num()));
    const double DesiredBarStep = EstWidgetWidth / NumFrames;
    const double DesiredBarWidth = FMath::Clamp(
        DesiredBarStep - ((DesiredBarStep >= 3.0) ? 1.0 : 0.0),
        static_cast<double>(FrameBarChart::MinBarWidth),
        static_cast<double>(FrameBarChart::MaxBarWidth));
    _FramesPerPixel = FMath::Clamp(
        1.0 / DesiredBarWidth,
        FrameBarChart::MinFramesPerPixel,
        FrameBarChart::MaxFramesPerPixel);

    Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    SCkFrameBarChart::
    ClearFrameData()
    -> void
{
    _FrameDurationsMs.Empty();
    _ViewOffset = 0.0;
    _FramesPerPixel = 1.0;
    _DisplayMaxMs = FrameBarChart::MinDisplayMaxMs;
    ClearSelection();
    Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    SCkFrameBarChart::
    RecalculateDisplayMax()
    -> void
{
    if (_FrameDurationsMs.Num() > 0)
    {
        TArray<double> Sorted = _FrameDurationsMs;
        Sorted.Sort();
        const int32 P95Index = FMath::Min(
            FMath::FloorToInt32(static_cast<float>(Sorted.Num()) * 0.95f),
            Sorted.Num() - 1);
        const double P95 = Sorted[P95Index];
        _DisplayMaxMs = FMath::Max(P95 * 1.3, FrameBarChart::MinDisplayMaxMs);
    }
    else
    {
        _DisplayMaxMs = FrameBarChart::MinDisplayMaxMs;
    }
    Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    SCkFrameBarChart::
    SetTargetFrameMs(double InTargetMs)
    -> void
{
    _TargetFrameMs = FMath::Max(1.0, InTargetMs);
    Invalidate(EInvalidateWidgetReason::Paint);
}

// --------------------------------------------------------------------------------------------------------------------
// Selection
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    SetSelection(uint64 StartFrame, uint64 EndFrame)
    -> void
{
    _bHasSelection = true;
    _SelectionStart = FMath::Min(StartFrame, EndFrame);
    _SelectionEnd = FMath::Max(StartFrame, EndFrame);
    Invalidate(EInvalidateWidgetReason::Paint);
}

auto
    SCkFrameBarChart::
    ClearSelection()
    -> void
{
    _bHasSelection = false;
    _SelectionStart = 0;
    _SelectionEnd = 0;
    Invalidate(EInvalidateWidgetReason::Paint);
}

// --------------------------------------------------------------------------------------------------------------------
// SWidget Interface
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    ComputeDesiredSize(float) const
    -> FVector2D
{
    return FVector2D(400.0f, FrameBarChart::DesiredHeight);
}

auto
    SCkFrameBarChart::
    OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
            const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
            int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
    -> int32
{
    const float Width = AllottedGeometry.GetLocalSize().X;
    const float Height = AllottedGeometry.GetLocalSize().Y;
    const float DrawHeight = Height - FrameBarChart::TopPadding - FrameBarChart::BottomPadding;

    if (_FrameDurationsMs.Num() == 0 || Width <= 0.0f || DrawHeight <= 0.0f)
    {
        return LayerId;
    }

    // Background
    FSlateDrawElement::MakeBox(
        OutDrawElements, LayerId,
        AllottedGeometry.ToPaintGeometry(),
        FAppStyle::GetBrush(TEXT("WhiteBrush")),
        ESlateDrawEffect::None,
        FrameBarChart::ColorBackground);

    // Use P95-based display max so outlier frames don't squish normal bars.
    // Bars exceeding this value clamp to full height.
    const double MaxMs = _DisplayMaxMs;

    const float BarW = GetBarWidth(AllottedGeometry);
    const float BarStep = GetBarStep(AllottedGeometry);

    // Draw threshold lines with labels (matching Insights style)
    const FSlateFontInfo LabelFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);
    const float BarAreaRight = Width - FrameBarChart::LabelRightMargin;

    auto DrawThresholdLine = [&](double ThresholdMs, const FLinearColor& LineColor, const FString& Label)
    {
        const float Y = FrameBarChart::TopPadding + DrawHeight * (1.0f - static_cast<float>(ThresholdMs / MaxMs));
        if (Y >= FrameBarChart::TopPadding && Y <= Height - FrameBarChart::BottomPadding)
        {
            TArray<FVector2D> LinePoints;
            LinePoints.Add(FVector2D(0.0f, Y));
            LinePoints.Add(FVector2D(BarAreaRight, Y));
            FSlateDrawElement::MakeLines(
                OutDrawElements, LayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                LinePoints, ESlateDrawEffect::None, LineColor, true, 1.0f);

            // Label on the right: "16.7 ms (60 fps)"
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 1,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(FrameBarChart::LabelRightMargin, 14.0f),
                    FSlateLayoutTransform(FVector2D(BarAreaRight + 4.0f, Y - 7.0f))),
                Label, LabelFont,
                ESlateDrawEffect::None, FrameBarChart::ColorLabel);
        }
    };

    DrawThresholdLine(16.67,  FrameBarChart::ColorThresholdGreen,  TEXT("16.7 ms (60 fps)"));
    DrawThresholdLine(33.33,  FrameBarChart::ColorThresholdYellow, TEXT("33.3 ms (30 fps)"));
    DrawThresholdLine(50.0,   FrameBarChart::ColorThresholdOrange, TEXT("50 ms (20 fps)"));
    DrawThresholdLine(66.67,  FrameBarChart::ColorThresholdRed,    TEXT("66.7 ms (15 fps)"));
    DrawThresholdLine(100.0,  FrameBarChart::ColorThresholdRed,    TEXT("100 ms (10 fps)"));

    // Draw frame number axis along the top with vertical grid lines
    {
        const FSlateFontInfo AxisFont = FCoreStyle::GetDefaultFontStyle("Regular", 8);
        const FLinearColor GridLineColor = FLinearColor(0.3f, 0.3f, 0.3f, 0.2f);

        // Compute nice label interval so labels don't overlap (1-2-5 sequence)
        constexpr float MinLabelSpacing = 80.0f;
        const int32 FramesPerLabel = FMath::Max(1, FMath::CeilToInt32(MinLabelSpacing / BarStep));

        int32 NiceInterval = 1;
        const int32 NiceSteps[] = { 1, 2, 5 };
        int32 Magnitude = 1;
        while (NiceInterval < FramesPerLabel)
        {
            for (int32 Step : NiceSteps)
            {
                NiceInterval = Step * Magnitude;
                if (NiceInterval >= FramesPerLabel) break;
            }
            if (NiceInterval < FramesPerLabel) Magnitude *= 10;
        }

        const int32 LabelFirstFrame = FMath::Max(0,
            (FMath::FloorToInt32(static_cast<float>(_ViewOffset)) / NiceInterval) * NiceInterval);

        for (int32 i = LabelFirstFrame; i < _FrameDurationsMs.Num(); i += NiceInterval)
        {
            const float LabelX = (static_cast<float>(i) - static_cast<float>(_ViewOffset)) * BarStep;
            if (LabelX < -30.0f) continue;
            if (LabelX > BarAreaRight) break;

            // Vertical grid line from top of chart area to bottom
            TArray<FVector2D> GridPoints;
            GridPoints.Add(FVector2D(LabelX, FrameBarChart::TopPadding));
            GridPoints.Add(FVector2D(LabelX, Height - FrameBarChart::BottomPadding));
            FSlateDrawElement::MakeLines(
                OutDrawElements, LayerId + 1,
                AllottedGeometry.ToPaintGeometry(),
                GridPoints, ESlateDrawEffect::None, GridLineColor, true, 1.0f);

            // Frame number label at the top
            FSlateDrawElement::MakeText(
                OutDrawElements, LayerId + 1,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(60.0f, 14.0f),
                    FSlateLayoutTransform(FVector2D(LabelX + 2.0f, 1.0f))),
                FrameBarChart::FormatWithCommas(i),
                AxisFont,
                ESlateDrawEffect::None,
                FrameBarChart::ColorLabel);
        }
    }

    // Draw bars
    const int32 FirstFrame = FMath::Max(0, FMath::FloorToInt32(_ViewOffset));
    const int32 VisibleBars = FMath::CeilToInt32(Width / BarStep) + 2;
    const int32 LastFrame = FMath::Min(FirstFrame + VisibleBars, _FrameDurationsMs.Num());

    for (int32 i = FirstFrame; i < LastFrame; ++i)
    {
        const float X = (static_cast<float>(i) - static_cast<float>(_ViewOffset)) * BarStep;
        if (X + BarW < 0.0f || X > Width) continue;

        const double Ms = _FrameDurationsMs[i];
        const float BarHeight = FMath::Clamp(
            DrawHeight * static_cast<float>(Ms / MaxMs), 1.0f, DrawHeight);
        const float BarY = FrameBarChart::TopPadding + DrawHeight - BarHeight;

        const FLinearColor Color = GetBarColor(Ms);

        FSlateDrawElement::MakeBox(
            OutDrawElements, LayerId + 2,
            AllottedGeometry.ToPaintGeometry(
                FVector2D(BarW, BarHeight),
                FSlateLayoutTransform(FVector2D(X, BarY))),
            FAppStyle::GetBrush(TEXT("WhiteBrush")),
            ESlateDrawEffect::None, Color);
    }

    // Draw selection highlight
    if (_bHasSelection)
    {
        const float SelX1 = (static_cast<float>(_SelectionStart) - static_cast<float>(_ViewOffset)) * BarStep;
        const float SelX2 = (static_cast<float>(_SelectionEnd + 1) - static_cast<float>(_ViewOffset)) * BarStep;
        const float ClampedX1 = FMath::Max(0.0f, SelX1);
        const float ClampedX2 = FMath::Min(Width, SelX2);

        if (ClampedX2 > ClampedX1)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 3,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(ClampedX2 - ClampedX1, Height),
                    FSlateLayoutTransform(FVector2D(ClampedX1, 0.0f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")),
                ESlateDrawEffect::None, FrameBarChart::ColorSelect);
        }
    }

    // Draw hover highlight
    if (_HoveredFrame >= 0 && _HoveredFrame < _FrameDurationsMs.Num())
    {
        const float HoverX = (static_cast<float>(_HoveredFrame) - static_cast<float>(_ViewOffset)) * BarStep;
        if (HoverX >= 0.0f && HoverX + BarW <= Width)
        {
            FSlateDrawElement::MakeBox(
                OutDrawElements, LayerId + 4,
                AllottedGeometry.ToPaintGeometry(
                    FVector2D(BarW, Height),
                    FSlateLayoutTransform(FVector2D(HoverX, 0.0f))),
                FAppStyle::GetBrush(TEXT("WhiteBrush")),
                ESlateDrawEffect::None, FrameBarChart::ColorHover);
        }
    }

    return LayerId + 5;
}

// --------------------------------------------------------------------------------------------------------------------
// Mouse Interaction
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    -> FReply
{
    if (_FrameDurationsMs.Num() == 0) return FReply::Unhandled();

    auto Self = SharedThis(const_cast<SCkFrameBarChart*>(this));

    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        _bIsDragging = true;
        _DragStartFrame = XToFrame(MyGeometry, MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X);
        return FReply::Handled().CaptureMouse(Self).SetUserFocus(Self, EFocusCause::Mouse);
    }

    if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton ||
        MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        _bIsPanning = true;
        _PanStartX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
        _PanStartOffset = _ViewOffset;
        return FReply::Handled().CaptureMouse(Self).SetUserFocus(Self, EFocusCause::Mouse);
    }

    return FReply::Unhandled();
}

auto
    SCkFrameBarChart::
    OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    -> FReply
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && _bIsDragging)
    {
        _bIsDragging = false;

        const int64 EndFrame = XToFrame(MyGeometry, MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X);
        const uint64 Start = static_cast<uint64>(FMath::Min(_DragStartFrame, EndFrame));
        const uint64 End = static_cast<uint64>(FMath::Max(_DragStartFrame, EndFrame));

        SetSelection(Start, End);
        _OnFrameSelectionChanged.ExecuteIfBound(Start, End);

        return FReply::Handled().ReleaseMouseCapture();
    }

    if ((MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton ||
         MouseEvent.GetEffectingButton() == EKeys::RightMouseButton) && _bIsPanning)
    {
        _bIsPanning = false;
        return FReply::Handled().ReleaseMouseCapture();
    }

    return FReply::Unhandled();
}

auto
    SCkFrameBarChart::
    OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    -> FReply
{
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());

    if (_bIsDragging)
    {
        // Update visual selection during drag
        const int64 CurrentFrame = XToFrame(MyGeometry, LocalPos.X);
        const uint64 Start = static_cast<uint64>(FMath::Min(_DragStartFrame, CurrentFrame));
        const uint64 End = static_cast<uint64>(FMath::Max(_DragStartFrame, CurrentFrame));
        SetSelection(Start, End);
        return FReply::Handled();
    }

    if (_bIsPanning)
    {
        const float BarStep = GetBarStep(MyGeometry);
        const float DeltaX = LocalPos.X - _PanStartX;
        _ViewOffset = _PanStartOffset - static_cast<double>(DeltaX) / static_cast<double>(BarStep);
        _ViewOffset = FMath::Clamp(_ViewOffset, 0.0,
            FMath::Max(0.0, static_cast<double>(_FrameDurationsMs.Num()) - 10.0));
        Invalidate(EInvalidateWidgetReason::Paint);
        return FReply::Handled();
    }

    // Hover tracking
    const int64 NewHoveredFrame = XToFrame(MyGeometry, LocalPos.X);
    const_cast<SCkFrameBarChart*>(this)->_HoveredFrame = NewHoveredFrame;

    // Update tooltip text
    if (NewHoveredFrame >= 0 && NewHoveredFrame < _FrameDurationsMs.Num() && _TooltipTextBlock.IsValid())
    {
        const double Ms = _FrameDurationsMs[NewHoveredFrame];
        const double BudgetDelta = Ms - _TargetFrameMs;

        FString BudgetText;
        if (BudgetDelta > 0.1)
        {
            BudgetText = FString::Printf(TEXT("  (+%.1fms over %.1fms budget)"), BudgetDelta, _TargetFrameMs);
        }
        else
        {
            BudgetText = TEXT("  (within budget)");
        }

        _TooltipTextBlock->SetText(FText::FromString(
            FString::Printf(TEXT("Frame #%s  —  %.2fms%s"),
                *FrameBarChart::FormatWithCommas(NewHoveredFrame), Ms, *BudgetText)));
    }

    Invalidate(EInvalidateWidgetReason::Paint);

    // Return Unhandled for hover-only so Slate continues to deliver OnMouseWheel events.
    // Returning Handled here can prevent mouse wheel routing in some UE5 Slate configurations.
    return FReply::Unhandled();
}

auto
    SCkFrameBarChart::
    OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    -> FReply
{
    if (_FrameDurationsMs.Num() == 0) return FReply::Unhandled();

    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
    ApplyZoom(MyGeometry, LocalPos.X, MouseEvent.GetWheelDelta());
    return FReply::Handled();
}

auto
    SCkFrameBarChart::
    OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
    -> FReply
{
    if (_FrameDurationsMs.Num() == 0) return FReply::Unhandled();

    const FKey Key = InKeyEvent.GetKey();

    // +/= to zoom in, -/_ to zoom out (numpad and regular keys)
    if (Key == EKeys::Add || Key == EKeys::Equals)
    {
        // Zoom centered on widget midpoint
        ApplyZoom(MyGeometry, MyGeometry.GetLocalSize().X * 0.5f, 1.0f);
        return FReply::Handled();
    }
    if (Key == EKeys::Subtract || Key == EKeys::Hyphen)
    {
        ApplyZoom(MyGeometry, MyGeometry.GetLocalSize().X * 0.5f, -1.0f);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

auto
    SCkFrameBarChart::
    OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
    -> void
{
    SCompoundWidget::OnMouseEnter(MyGeometry, MouseEvent);

    // Take focus on hover so scroll wheel zoom works without clicking first
    if (!HasKeyboardFocus())
    {
        FSlateApplication::Get().SetKeyboardFocus(SharedThis(this), EFocusCause::Mouse);
    }
}

auto
    SCkFrameBarChart::
    OnMouseLeave(const FPointerEvent& MouseEvent)
    -> void
{
    _HoveredFrame = -1;
    Invalidate(EInvalidateWidgetReason::Paint);
    SCompoundWidget::OnMouseLeave(MouseEvent);
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    SCkFrameBarChart::
    XToFrame(const FGeometry& Geometry, float LocalX) const
    -> int64
{
    const float BarStep = GetBarStep(Geometry);
    const int64 Frame = static_cast<int64>(_ViewOffset + static_cast<double>(LocalX) / static_cast<double>(BarStep));
    return FMath::Clamp(Frame, static_cast<int64>(0), static_cast<int64>(_FrameDurationsMs.Num() - 1));
}

auto
    SCkFrameBarChart::
    FrameToX(const FGeometry& Geometry, int64 FrameIndex) const
    -> float
{
    const float BarStep = GetBarStep(Geometry);
    return (static_cast<float>(FrameIndex) - static_cast<float>(_ViewOffset)) * BarStep;
}

auto
    SCkFrameBarChart::
    GetBarColor(double DurationMs) const
    -> FLinearColor
{
    if (DurationMs > _TargetFrameMs * 2.0)
    {
        return FrameBarChart::ColorRed;
    }
    if (DurationMs > _TargetFrameMs)
    {
        return FrameBarChart::ColorYellow;
    }
    return FrameBarChart::ColorGreen;
}

auto
    SCkFrameBarChart::
    GetBarWidth(const FGeometry& Geometry) const
    -> float
{
    // Bar width inversely proportional to frames-per-pixel
    const float Width = static_cast<float>(1.0 / _FramesPerPixel);
    return FMath::Clamp(Width, FrameBarChart::MinBarWidth, FrameBarChart::MaxBarWidth);
}

auto
    SCkFrameBarChart::
    GetBarStep(const FGeometry& Geometry) const
    -> float
{
    const float BarWidth = GetBarWidth(Geometry);
    // Drop spacing when bars are very thin (zoomed out) to fit more frames
    const float Spacing = (BarWidth >= 2.0f) ? 1.0f : 0.0f;
    return FMath::Max(BarWidth + Spacing, 0.5f); // minimum step to avoid division by zero
}

auto
    SCkFrameBarChart::
    ApplyZoom(const FGeometry& Geometry, float LocalX, float Delta)
    -> void
{
    // Frame under cursor before zoom
    const float BarStep = GetBarStep(Geometry);
    const double FrameUnderCursor = _ViewOffset + static_cast<double>(LocalX) / static_cast<double>(BarStep);

    // Adjust zoom
    const double ZoomFactor = (Delta > 0) ? 0.8 : 1.25;
    _FramesPerPixel = FMath::Clamp(
        _FramesPerPixel * ZoomFactor,
        FrameBarChart::MinFramesPerPixel,
        FrameBarChart::MaxFramesPerPixel);

    // Recalculate offset to keep the frame under cursor in place
    const float NewBarStep = GetBarStep(Geometry);
    _ViewOffset = FrameUnderCursor - static_cast<double>(LocalX) / static_cast<double>(NewBarStep);
    _ViewOffset = FMath::Max(0.0, _ViewOffset);

    Invalidate(EInvalidateWidgetReason::Paint);
}

// --------------------------------------------------------------------------------------------------------------------
